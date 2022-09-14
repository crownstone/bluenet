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

#include "app_error.h"
#include "app_error_weak.h"
#include "bootloader/cs_MemoryLayout.h"
#include "cfg/cs_Boards.h"
#include "cfg/cs_DeviceTypes.h"
#include "cs_BootloaderConfig.h"
#include "cs_GpRegRetConfig.h"
#include "drivers/cs_Uicr.h"
#include "ipc/cs_IpcRamData.h"
#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_dfu_timers.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"
#include "nrf_dfu.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_mbr.h"
#include "nrf_nvmc.h"
#include "protocol/cs_ErrorCodes.h"

static void on_error(void) {
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

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t* p_file_name) {
	NRF_LOG_ERROR("%s:%d", p_file_name, line_num);
	on_error();
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
	NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
	on_error();
}

void app_error_handler_bare(uint32_t error_code) {
	NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
	on_error();
}

/**
 * @brief Function notifies certain events in DFU process.
 */
static void dfu_observer(nrf_dfu_evt_type_t evt_type) {
	switch (evt_type) {
		case NRF_DFU_EVT_DFU_FAILED: break;
		case NRF_DFU_EVT_DFU_ABORTED: break;
		case NRF_DFU_EVT_DFU_INITIALIZED: break;
		case NRF_DFU_EVT_TRANSPORT_ACTIVATED: break;
		case NRF_DFU_EVT_DFU_STARTED: break;
		default: break;
	}
}

/**
 * Make sure all GPIO is initialized.
 */
void cs_gpio_init(boards_config_t* board) {
	if (board->flags.usesNfcPins) {
		// Enable NFC pins to be used as GPIO.
		// Warning: this is stored in UICR, so it's persistent.
		// Warning: NFC pins leak a bit of current when not at same voltage level.
		cs_ret_code_t retCode = enableNfcPinsAsGpio();
		if (retCode != ERR_SUCCESS) {
			// Not much we can do here.
		}
	}

	// Turn dimmer off if present
	if (board->pinDimmer != PIN_NONE) {
		nrf_gpio_cfg_output(board->pinDimmer);
		if (board->flags.dimmerInverted) {
			nrf_gpio_pin_set(board->pinDimmer);
		}
		else {
			nrf_gpio_pin_clear(board->pinDimmer);
		}
	}

	if (board->pinEnableDimmer != PIN_NONE) {
		nrf_gpio_cfg_output(board->pinEnableDimmer);
		nrf_gpio_pin_clear(board->pinEnableDimmer);
	}

	// Relay pins
	if (board->pinRelayOff != PIN_NONE) {
		nrf_gpio_cfg_output(board->pinRelayOff);
		nrf_gpio_pin_clear(board->pinRelayOff);
	}
	if (board->pinRelayOn != PIN_NONE) {
		nrf_gpio_cfg_output(board->pinRelayOn);
		nrf_gpio_pin_clear(board->pinRelayOn);
	}
}

/** See cs_GpRegRet.h */
void cs_check_gpregret() {
	uint32_t gpregret = NRF_POWER->GPREGRET;
	bool start_dfu    = false;
	uint32_t counter  = gpregret & CS_GPREGRET_COUNTER_MASK;
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
		NRF_POWER->GPREGRET = gpregret + 1;
	}

	if (start_dfu) {
		NRF_LOG_INFO("Start DFU");
		NRF_POWER->GPREGRET = BOOTLOADER_DFU_START;
	}
	gpregret = NRF_POWER->GPREGRET;
	NRF_LOG_INFO("GPREGRET=%u", gpregret);
}

/*
 * Writes bootloader information. This is mainly version information about the bootloader. It also contains
 * information on firmware being just activated or not. The field updateError can be used to commmunicate difficulties
 * retrieving the IPC ram data. Although this data is written by the bootloader itself it might not be present
 * (at a cold boot) or there might be other unforeseen error conditions.
 */
void set_bootloader_info() {
	NRF_LOG_INFO("Set bootloader info");
	NRF_LOG_FLUSH();

	bluenet_ipc_data_t ipcData;
	uint8_t dataSize;

	ipcData.bootloaderData.flagsRaw = 0;

	// First get previous ram data.
	// We don't want to overwrite "justActivated" for example.
	int retCode = getRamData(IPC_INDEX_BOOTLOADER_INFO, ipcData.raw, &dataSize, sizeof(ipcData.raw));
	if (retCode != IPC_RET_SUCCESS) {
		NRF_LOG_INFO("Failed to get bootloader info: retCode=%i", retCode);
		// This can also happen when there is random data in ram.
		// In this case: overwrite all data.
		ipcData.bootloaderData.flags.justActivated = 0;
		ipcData.bootloaderData.flags.versionError  = 0;
		ipcData.bootloaderData.flags.readError     = 1;
	}
	else {
		if (ipcData.bootloaderData.ipcDataMajor != g_BOOTLOADER_IPC_RAM_MAJOR) {
			// We don't know how to parse this, so don't use it.
			// Once we update the major, we can add backwards compatibility here.
			ipcData.bootloaderData.flags.justActivated = 0;
			ipcData.bootloaderData.flags.versionError  = 1;
			ipcData.bootloaderData.flags.readError     = 0;
		}
	}

	// Since we may have a newer minor version, use the latest dataSize.
	dataSize = sizeof(ipcData.bootloaderData);

	ipcData.bootloaderData.ipcDataMajor         = g_BOOTLOADER_IPC_RAM_MAJOR;
	ipcData.bootloaderData.ipcDataMinor         = g_BOOTLOADER_IPC_RAM_MINOR;
	ipcData.bootloaderData.dfuVersion           = g_BOOTLOADER_DFU_VERSION;
	ipcData.bootloaderData.bootloaderMajor      = g_BOOTLOADER_VERSION_MAJOR;
	ipcData.bootloaderData.bootloaderMinor      = g_BOOTLOADER_VERSION_MINOR;
	ipcData.bootloaderData.bootloaderPatch      = g_BOOTLOADER_VERSION_PATCH;
	ipcData.bootloaderData.bootloaderPrerelease = g_BOOTLOADER_VERSION_PRERELEASE;
	ipcData.bootloaderData.bootloaderBuildType  = g_BOOTLOADER_BUILD_TYPE;
	setRamData(IPC_INDEX_BOOTLOADER_INFO, ipcData.raw, dataSize);
}

/**
 * Round up val to the next page boundary (from nrf_dfu_utils.h)
 */
#define ALIGN_TO_PAGE(val) ALIGN_NUM((CODE_PAGE_SIZE), (val))

/**@brief Function for application main entry. */
int main(void) {
	uint32_t ret_val;

	boards_config_t board = {};
	configure_board(&board);
	cs_gpio_init(&board);

	(void)NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
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
	// See
	// https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.3.0/group__nrf__bootloader__config.html
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
