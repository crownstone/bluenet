/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2016
 * License: LGPLv3+
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
	  |  |  |--------------  Product Type: 1 Dev, 2 Plug, 3 Builtin, 4 Guidestone
	  |  |-----------------  Market: 1 EU, 2 US
	  |--------------------  Family: 1 Crownstone
*/

inline const char* get_hardware_version() {

	uint32_t hardwareBoard = NRF_UICR->CUSTOMER[UICR_BOARD_INDEX];
	if (hardwareBoard == 0xFFFFFFFF) {
		hardwareBoard = ACR01B2C;
	}

//	LOGi("UICR");
//	BLEutil::printArray((uint8_t*)NRF_UICR->CUSTOMER, 128);
	
	// CROWNSTONE BUILTINS
	if (hardwareBoard == ACR01B1A) return "10103000100";
	if (hardwareBoard == ACR01B1B) return "10103000200";
	if (hardwareBoard == ACR01B1C) return "10103000300";
	if (hardwareBoard == ACR01B1D) return "10103000400";
	if (hardwareBoard == ACR01B1E) return "10103000500";
	if (hardwareBoard == ACR01B6A) return "10103020100";
	
	// CROWNSTONE PLUGS
	if (hardwareBoard == ACR01B2A) return "10102000100";
	if (hardwareBoard == ACR01B2B) return "10102000200";
	if (hardwareBoard == ACR01B2C) return "10102010000";
	if (hardwareBoard == ACR01B2E) return "10102010100";
	if (hardwareBoard == ACR01B2F) return "10102010200";


	// GUIDESTONE
	if (hardwareBoard == GUIDESTONE) return "10104010000";

	/////////////////////////////////////////////////////////////////
	//// Old and Third Party Boards
	/////////////////////////////////////////////////////////////////

	if (hardwareBoard == CROWNSTONE5) return "plugin_quant";
	if (hardwareBoard == PLUGIN_FLEXPRINT_01) return "plugin_flexprint_01";

	// Nordic Boards
	if (hardwareBoard == PCA10036) return "PCA10036";
	if (hardwareBoard == PCA10040) return "PCA10040";

	LOGe("Failed to define version for hardware board");
	APP_ERROR_CHECK(1);

	return "Unknown";
}
