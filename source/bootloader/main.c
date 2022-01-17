/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup bootloader_secure_ble main.c
 * @{
 * @ingroup dfu_bootloader_api
 * @brief Bootloader project main file for secure DFU.
 *
 */

#include <stdint.h>
#include <string.h>

#include "cs_GpRegRetConfig.h"
//#include "boards.h"
#include "nrf_mbr.h"
#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_dfu_timers.h"
#include "nrf_dfu.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "cfg/cs_Boards.h"
#include "cfg/cs_DeviceTypes.h"
#include "nrf_nvmc.h"
#include "ipc/cs_IpcRamData.h"
#include "cs_BootloaderConfig.h"

#include "nrf_dfu_settings.h"


static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();

#if NRF_MODULE_ENABLED(NRF_LOG_BACKEND_RTT)
    // To allow the buffer to be flushed by the host.
    nrf_delay_ms(100);
#endif
#ifdef NRF_DFU_DEBUG_VERSION
    NRF_BREAKPOINT_COND;
#endif
    NVIC_SystemReset();
}


void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    NRF_LOG_ERROR("%s:%d", p_file_name, line_num);
    on_error();
}


void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}


void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
    on_error();
}

static bool micro_app_page_cleared = false;

void on_clear_micro_app_page_complete(void* p_buf) {
	micro_app_page_cleared = true;
}

static void clear_micro_app_page() {
	const uint32_t microapp_page_start = 0x69000;
////	ret_code_t err_code                =
	nrf_dfu_flash_erase(microapp_page_start, 1, on_clear_micro_app_page_complete);
}

uint8_t init_cmd_buffer[INIT_COMMAND_MAX_SIZE];
uint8_t validationbytes[3][16] = {{48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 69, 70},
							   {70, 69, 68, 67, 66, 65, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48},
							   {48, 70, 49, 69, 50, 68, 51, 67, 52, 66, 53, 65, 54, 57, 55, 56}}; // 1,2,3... in ascii

static void write_init_packet_to_micro_app_page(uint8_t offset_index) {
	if (!micro_app_page_cleared) {
		return;
	}

	NRF_LOG_HEXDUMP_INFO(s_dfu_settings.init_command, 128);

	const uint32_t microapp_page_start     = 0x69000;
	const uint32_t offset_size = 0x60; // INIT_COMMAND_MAX_SIZE; // offset is for debug: every phase gets moved a bit to see where the init packet is complete.
	const uint32_t validation_int = 0xAEAEAEAE;

	// (s_dfu_settings is defined external in nrf_dfu_settings.h)
	// it contains:
	//	.init_command;          /**< Buffer for storing the init command. */
	//	.progress.command_size; /**< The size of the current init command stored in the DFU settings. */
	const uint32_t init_packet_size = s_dfu_settings.progress.command_size;

	// copy dfu init page content to single buffer before writing
	uint32_t offset = 0;
	uint32_t len = 0;

	len = sizeof(init_packet_size);
	memcpy(&init_cmd_buffer[offset], &init_packet_size, len);
	offset += len;

	len = sizeof(validation_int);
	memcpy(&init_cmd_buffer[offset], &validation_int, len);
	offset += len;

	len = init_packet_size;
	memcpy(&init_cmd_buffer[offset], &s_dfu_settings.init_command, len);

	//memcpy(&init_cmd_buffer, validationbytes[offset_index], init_packet_size);

	// write the buffer to flash
	nrf_dfu_flash_store(
			microapp_page_start + offset_index * offset_size , init_cmd_buffer, INIT_COMMAND_MAX_SIZE, NULL);
}

/**
 * @brief Function notifies certain events in DFU process.
 */
static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
	// TODO: print nrf_dfu_validation_init_cmd_present() in the various events
//            bsp_board_init(BSP_INIT_LEDS);
//            bsp_board_led_on(BSP_BOARD_LED_0);
//            bsp_board_led_on(BSP_BOARD_LED_1);
//            bsp_board_led_off(BSP_BOARD_LED_2);
//            bsp_board_led_off(BSP_BOARD_LED_1);
//            bsp_board_led_on(BSP_BOARD_LED_2);

    switch (evt_type)
    {
        case NRF_DFU_EVT_TRANSPORT_ACTIVATED:
        case NRF_DFU_EVT_TRANSPORT_DEACTIVATED:
        case NRF_DFU_EVT_DFU_FAILED:
        case NRF_DFU_EVT_DFU_ABORTED:
            break;
		// ---------------------------------------
        case NRF_DFU_EVT_DFU_INITIALIZED: {
        	clear_micro_app_page();
        	break;
        }
        case NRF_DFU_EVT_DFU_STARTED: {
        	write_init_packet_to_micro_app_page(0);
        	break;
        }
        case NRF_DFU_EVT_OBJECT_RECEIVED: {
        	write_init_packet_to_micro_app_page(1);
        	break;
        }
        case NRF_DFU_EVT_DFU_COMPLETED: {
        	write_init_packet_to_micro_app_page(2);
        	break;
        }
        default:
            break;
    }
}

/**
 * Make sure all GPIO is initialized.
 */
void cs_gpio_init(boards_config_t* board) {
	switch (board->hardwareBoard) {
	case ACR01B10D:
		// Enable NFC pins
		if (NRF_UICR->NFCPINS != 0) {
			nrf_nvmc_write_word((uint32_t)&(NRF_UICR->NFCPINS), 0);
		}
		break;
	default:
		break;
	}
	if (IS_CROWNSTONE(board->deviceType)) {
		// Turn dimmer off.
		nrf_gpio_cfg_output(board->pinGpioPwm);
		if (board->flags.pwmInverted) {
			nrf_gpio_pin_set(board->pinGpioPwm);
		}
		else {
			nrf_gpio_pin_clear(board->pinGpioPwm);
		}
		nrf_gpio_cfg_output(board->pinGpioEnablePwm);
		nrf_gpio_pin_clear(board->pinGpioEnablePwm);
		// Don't do anything with relay.
		if (board->flags.hasRelay) {
			nrf_gpio_cfg_output(board->pinGpioRelayOff);
			nrf_gpio_pin_clear(board->pinGpioRelayOff);
			nrf_gpio_cfg_output(board->pinGpioRelayOn);
			nrf_gpio_pin_clear(board->pinGpioRelayOn);
		}
	}
}

/** See cs_GpRegRet.h */
void cs_check_gpregret() {
	uint32_t gpregret = NRF_POWER->GPREGRET;
	bool start_dfu = false;
	uint32_t counter = gpregret & CS_GPREGRET_COUNTER_MASK;
//	uint32_t flags = gpregret & (~CS_GPREGRET_RESET_COUNTER_MASK);
	NRF_LOG_INFO("GPREGRET=%u counter=%u", gpregret, counter);

	if (gpregret == CS_GPREGRET_LEGACY_DFU_RESET) {
		// Legacy value, set new dfu flag.
		NRF_LOG_WARNING("Legacy value");
		gpregret |= CS_GPREGRET_FLAG_DFU_RESET;
	}

	if (gpregret & CS_GPREGRET_FLAG_DFU_RESET) {
		// App wants to go to DFU mode.
		start_dfu = true;
	}

	if (counter == CS_GPREGRET_COUNTER_MAX) {
		start_dfu = true;
	}
	else {
		// Increase counter, but maintain flags.
//		counter += 1;
//		gpregret = flags | counter;
//		NRF_POWER->GPREGRET = gpregret;
		NRF_POWER->GPREGRET = gpregret + 1;
	}

	if (start_dfu) {
		NRF_LOG_INFO("Start DFU");
		NRF_POWER->GPREGRET = BOOTLOADER_DFU_START;
	}
	gpregret = NRF_POWER->GPREGRET;
	NRF_LOG_INFO("GPREGRET=%u", gpregret);
}

void set_bootloader_info() {
	NRF_LOG_INFO("Set bootloader info");
	NRF_LOG_FLUSH();

	bluenet_ipc_bootloader_data_t bootloader_data;
	bootloader_data.protocol = g_BOOTLOADER_IPC_RAM_PROTOCOL;
	bootloader_data.dfu_version = g_BOOTLOADER_DFU_VERSION;
	bootloader_data.major = g_BOOTLOADER_VERSION_MAJOR;
	bootloader_data.minor = g_BOOTLOADER_VERSION_MINOR;
	bootloader_data.patch = g_BOOTLOADER_VERSION_PATCH;
	bootloader_data.prerelease = g_BOOTLOADER_VERSION_PRERELEASE;
	bootloader_data.build_type = g_BOOTLOADER_BUILD_TYPE;

	uint8_t size = sizeof(bootloader_data);

	// should not happen but we don't want asserts in the bootloader, so we just truncate the struct
	if (size > BLUENET_IPC_RAM_DATA_ITEM_SIZE) {
		size = BLUENET_IPC_RAM_DATA_ITEM_SIZE;
	}
	setRamData(IPC_INDEX_BOOTLOADER_VERSION, (uint8_t*)&bootloader_data, size);
}


/**
 * Round up val to the next page boundary (from nrf_dfu_utils.h)
 */
#define ALIGN_TO_PAGE(val) ALIGN_NUM((CODE_PAGE_SIZE), (val))

/**@brief Function for application main entry. */
int main(void)
{
	uint32_t ret_val;

	boards_config_t board = {};
	configure_board(&board);
	cs_gpio_init(&board);

	(void) NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
	NRF_LOG_DEFAULT_BACKENDS_INIT();

	NRF_LOG_INFO("Reset reason: %u", NRF_POWER->RESETREAS);

	cs_check_gpregret();

	NRF_LOG_INFO("Protect");
	NRF_LOG_FLUSH();

	// Protect MBR and bootloader code from being overwritten.
#if NORDIC_SDK_VERSION == 15
	ret_val = nrf_bootloader_flash_protect(0, MBR_SIZE, false);
	APP_ERROR_CHECK(ret_val);
	ret_val = nrf_bootloader_flash_protect(BOOTLOADER_START_ADDR, BOOTLOADER_SIZE, false);
	APP_ERROR_CHECK(ret_val);
#else
	ret_val = nrf_bootloader_flash_protect(0, MBR_SIZE);
	APP_ERROR_CHECK(ret_val);
	ret_val = nrf_bootloader_flash_protect(BOOTLOADER_START_ADDR, BOOTLOADER_SIZE);
	APP_ERROR_CHECK(ret_val);
#endif
//	ret_val = nrf_bootloader_flash_protect(0, ALIGN_TO_PAGE(BOOTLOADER_START_ADDR + BOOTLOADER_SIZE), false);
//	APP_ERROR_CHECK(ret_val);

	set_bootloader_info();

	NRF_LOG_INFO("Init");
	NRF_LOG_FLUSH();

	// Need to adjust dfu_enter_check().
	// Or we can simply use NRF_BL_DFU_ENTER_METHOD_GPREGRET
	// and set GPREGRET to (BOOTLOADER_DFU_GPREGRET_MASK | BOOTLOADER_DFU_START_BIT_MASK) to start enter dfu mode.
	// See https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.3.0/group__nrf__bootloader__config.html
	ret_val = nrf_bootloader_init(dfu_observer);
	APP_ERROR_CHECK(ret_val);

	NRF_LOG_INFO("Start app");
	NRF_LOG_FLUSH();

	// Either there was no DFU functionality enabled in this project or the DFU module detected
	// no ongoing DFU operation and found a valid main application.
	// Boot the main application.
	nrf_bootloader_app_start();

	// Should never be reached.
	NRF_LOG_INFO("After main");
}

/**
 * @}
 */
