/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 21, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Boards.h>
#include <util/cs_BleError.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf52.h"

#ifdef __cplusplus
}
#endif

/*
	----------------------
	| GENERAL | PCB      |
	| PRODUCT | VERSION  |
	| INFO    |          |
	----------------------
	| 1 01 02 | 00 92 00 |
	----------------------
	  |  |  |    |  |  |---  Patch number of PCB version
	  |  |  |    |  |------  Minor number of PCB version
	  |  |  |    |---------  Major number of PCB version
	  |  |  |--------------  Product Type: 1 Dev, 2 Plug, 3 Builtin, 4 Guidestone, 5 dongle
	  |  |-----------------  Market: 1 EU, 2 US
	  |--------------------  Family: 1 Crownstone
*/

static inline const char* get_hardware_version() {

	uint32_t hardwareBoard = NRF_UICR->CUSTOMER[UICR_BOARD_INDEX];
	if (hardwareBoard == 0xFFFFFFFF) {
		hardwareBoard = ACR01B2C;
	}

	// Can't use LOGe here, as the bootloader also uses this file.
//	LOGi("UICR");
//	BLEutil::printArray((uint8_t*)NRF_UICR->CUSTOMER, 128);
	
	// CROWNSTONE BUILTINS
	if (hardwareBoard == ACR01B1A) return "10103000100";
	if (hardwareBoard == ACR01B1B) return "10103000200";
	if (hardwareBoard == ACR01B1C) return "10103000300";
	if (hardwareBoard == ACR01B1D) return "10103000400";
	if (hardwareBoard == ACR01B1E) return "10103000500";
//	if (hardwareBoard == ACR01B6C) return "10103010500";
//	if (hardwareBoard == ACR01B6D) return "10103010600";
	if (hardwareBoard == ACR01B9F) return "10103020000";
	
	// CROWNSTONE PLUGS
	if (hardwareBoard == ACR01B2A) return "10102000100";
	if (hardwareBoard == ACR01B2B) return "10102000200";
	if (hardwareBoard == ACR01B2C) return "10102010000";
	if (hardwareBoard == ACR01B2E) return "10102010100";
	if (hardwareBoard == ACR01B2G) return "10102010300";


	// GUIDESTONE
	if (hardwareBoard == GUIDESTONE) return "10104010000";

	// CROWNSTONE USB DONGLE
	if (hardwareBoard == CS_USB_DONGLE) return "10105000000";

	/////////////////////////////////////////////////////////////////
	//// Old and Third Party Boards
	/////////////////////////////////////////////////////////////////

	// Nordic Boards
	if (hardwareBoard == PCA10036) return "PCA10036";
	if (hardwareBoard == PCA10040) return "PCA10040";

	// Can't use LOGe here, as the bootloader also uses this file.
//	LOGe("Failed to define version for hardware board");
	APP_ERROR_CHECK(1);

	return "Unknown";
}
