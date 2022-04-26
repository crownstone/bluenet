/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 21, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Boards.h>
#include <cfg/cs_AutoConfig.h>
#include <protocol/cs_UicrPacket.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf52.h"
#include "string.h"

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
	  |  |  |--------------  Product Type, see cs_DeviceTypes.h: 1 Dev, 2 Plug, 3 Builtin, 4 Guidestone, 5 dongle, 6 Builtin One
	  |  |-----------------  Market: 1 EU, 2 US
	  |--------------------  Family: 1 Crownstone
*/

/**
 * The value written to the UICR is done by a target `write_board_version` created by cmake.
 * This target invokes a tool that extracts the value from the cs_Boards.h file.
 * The string returned here is a user-facing string that is not used anywhere else.
 * The reason to use the UICR for this:
 *   - bootloader and firmware are easier kept in tandem
 *   - firmware does not need to know itself on which hardware it runs, it can just inspect UICR
 *   - hardware version is characteristic for the hardware and should not be updateable (UICR)
 *     - note, hardware patches come of course with a new UICR
 *   - in test and development it's easy to set/get UICR values (without firmware running)
 *     - we can check for firmware compatibility in the build system if there are multiple firmwares
 */
static inline const char* get_hardware_version() {

	uint32_t hardwareBoard = getHardwareBoard();

	// Can't use LOGe here, as the bootloader also uses this file.

	switch (hardwareBoard) {
		// CROWNSTONE BUILTINS
		case ACR01B1A:  return "10103000100";
		case ACR01B1B:  return "10103000200";
		case ACR01B1C:  return "10103000300";
		case ACR01B1D:  return "10103000400";
		case ACR01B1E:  return "10103000500";

		// CROWNSTONE BUILTIN ONES
		case ACR01B10B: return "10106000000";
		case ACR01B10D: return "10106000100";

		// CROWNSTONE BUILTIN TWOS
		case ACR01B13B: return "10108000100";
		case ACR01B15A: return "10108000200";

		// CROWNSTONE PLUGS
		case ACR01B2A: return "10102000100";
		case ACR01B2B: return "10102000200";
		case ACR01B2C: return "10102010000";
		case ACR01B2E: return "10102010100";
		case ACR01B2G: return "10102010300";

		// GUIDESTONE
		case GUIDESTONE: return "10104010000";

		// CROWNSTONE USB DONGLE
		case CS_USB_DONGLE: return "10105000000";

		// Nordic Boards
		case PCA10036: return "PCA10036";
		case PCA10040: return "PCA10040";
		case PCA10100: return "PCA10100";
		case PCA10056: return "PCA10056";

		default: {
			// Can't use LOGe here, as the bootloader also uses this file.
//			LOGe("Failed to define version for hardware board");
//			APP_ERROR_CHECK(1);
			return "Unknown";
		}
	}
}

static inline const char* get_hardware_version_from_uicr(const cs_uicr_data_t* uicrData) {
	// The string is always 11 chars, add 1 byte for the null terminator.
	// Since you can't specify a max width, just use modulo to limit it.
	char versionString[12];
	sprintf(versionString, "%u%02u%02u%02u%02u%02u",
			uicrData->productRegionFamily.fields.productFamily % 10,
			uicrData->productRegionFamily.fields.region % 100,
			uicrData->productRegionFamily.fields.productType % 100,
			uicrData->majorMinorPatch.fields.major % 100,
			uicrData->majorMinorPatch.fields.minor % 100,
			uicrData->majorMinorPatch.fields.patch % 100);
	return versionString;
}
