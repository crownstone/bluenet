/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 7, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "cs_ipc_bootloader_status.h"

#ifdef BUILD_P2P_DFU

static const uint32_t ipc_page_start     = 0x69000;
static const uint32_t validation_pre = 0x90909090;
static const uint32_t validation_post = 0xAEAEAEAE;

static bool ipc_page_cleared = false;
static bool init_cmd_write_started = false;

uint8_t init_cmd_buffer[INIT_COMMAND_MAX_SIZE] = {0};
uint8_t validationbytes[3][16] = {{48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 69, 70},
							   {70, 69, 68, 67, 66, 65, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48},
							   {48, 70, 49, 69, 50, 68, 51, 67, 52, 66, 53, 65, 54, 57, 55, 56}}; // 1,2,3... in ascii


static void on_clear_ipc_page_complete(void* p_buf);
static void clear_ipc_page();
static void write_init_packet_to_ipc_page();

#endif


void dfu_observer(nrf_dfu_evt_type_t evt_type) {
	switch (evt_type) {
		case NRF_DFU_EVT_DFU_INITIALIZED: {
			cs_dfu_observer_on_dfu_initialized();
			break;
		}
		case NRF_DFU_EVT_TRANSPORT_ACTIVATED: {
			cs_dfu_observer_on_transport_activated();
			break;
		}
		case NRF_DFU_EVT_TRANSPORT_DEACTIVATED: {
			cs_dfu_observer_on_transport_deactivated();
			break;
		}
		case NRF_DFU_EVT_DFU_STARTED: {
			cs_dfu_observer_on_dfu_started();
			break;
		}
		case NRF_DFU_EVT_OBJECT_RECEIVED: {
			cs_dfu_observer_on_object_received();
			break;
		}
		case NRF_DFU_EVT_DFU_FAILED: {
			cs_dfu_observer_on_dfu_failed();
			break;
		}
		case NRF_DFU_EVT_DFU_COMPLETED: {
			cs_dfu_observer_on_dfu_completed();
			break;
		}
		case NRF_DFU_EVT_DFU_ABORTED: {
			cs_dfu_observer_on_dfu_aborted();
			break;
		}
	}
}


void cs_dfu_observer_on_dfu_initialized() {
#ifdef BUILD_P2P_DFU
	clear_ipc_page();
#endif
}

void cs_dfu_observer_on_transport_activated(){

}

void cs_dfu_observer_on_transport_deactivated() {

}

void cs_dfu_observer_on_dfu_started() {

}

void cs_dfu_observer_on_object_received() {
#ifdef BUILD_P2P_DFU
    write_init_packet_to_ipc_page();
#endif
}

void cs_dfu_observer_on_dfu_failed() {

}

void cs_dfu_observer_on_dfu_completed() {

}

void cs_dfu_observer_on_dfu_aborted() {

}



#ifdef BUILD_P2P_DFU

void on_clear_ipc_page_complete(void* p_buf) {
	ipc_page_cleared = true;
}

static void clear_ipc_page() {
	nrf_dfu_flash_erase(ipc_page_start, 1, on_clear_ipc_page_complete);
}

static void write_init_packet_to_ipc_page() {
	if (!ipc_page_cleared) {
		return;
	}

	if(init_cmd_write_started) {
		// only write once.
		return;
	}

	// fill buffer with the offset to increase memrd readibility
	memset(init_cmd_buffer, 1+offset_index, INIT_COMMAND_MAX_SIZE);

	NRF_LOG_HEXDUMP_INFO(s_dfu_settings.init_command, 128);

	// (s_dfu_settings is defined external in nrf_dfu_settings.h)
	// it contains:
	//	.init_command;          /**< Buffer for storing the init command. */
	//	.progress.command_size; /**< The size of the current init command stored in the DFU settings. */

	// copy dfu init page content to single buffer before writing
	memcpy(&init_cmd_buffer[0], &s_dfu_settings.progress.command_size, 4);
	memcpy(&init_cmd_buffer[4], &validation_int, 4);
	memcpy(&init_cmd_buffer[8], &s_dfu_settings.init_command, s_dfu_settings.progress.command_size);
	const uint32_t total_size = INIT_COMMAND_MAX_SIZE + 4 + 4;

	// write the buffer to flash
	ret_code_t flash_write_status = nrf_dfu_flash_store(ipc_page_start, init_cmd_buffer, total_size, NULL);

	init_cmd_write_started = flash_write_status == NRF_SUCCESS;
}

#endif
