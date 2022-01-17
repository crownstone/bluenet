/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 21, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include<util/cs_Utils.h>

/**
 * namespace to keep track of the constants related to Mesh DFU.
 *
 * NOTE: corresponds roughly to ble_constans.py and incorporates ble_uuid.py
 */
namespace MeshDfuConstants {

namespace DfuHostSettings {
constexpr uint8_t MaxReconnectionAttempts = 5;
constexpr uint32_t ReconnectTimeoutMs = 5000u;
constexpr uint32_t ConnectionTimeoutMs = 15000u;
constexpr uint32_t DefaultTimeoutMs = 30000u;

}

namespace DFUAdapter {

constexpr uint16_t dfuServiceShortUuid = 0xFE59;
constexpr char controlPointString[] = "8ec90001-f315-4f60-9fb8-838830daea50";
constexpr uint16_t controlPointShortUuid = 1;
constexpr char dataPointString[]    = "8ec90002-f315-4f60-9fb8-838830daea50";
constexpr uint16_t dataPointShortUuid = 2;

constexpr uint8_t CONNECTION_ATTEMPTS = 3;
constexpr uint8_t ERROR_CODE_POS      = 2;
constexpr uint8_t LOCAL_ATT_MTU       = 200;

}  // namespace DFUAdapter

namespace DfuTransportBle {
constexpr uint8_t DEFAULT_TIMEOUT = 20;
constexpr uint8_t RETRIES_NUMBER  = 5;





constexpr auto EXT_ERROR_CODE(uint8_t errorcode) {
	const char* codes[] = {
			"No extended error code has been set. This error indicates an implementation problem.",
			"Invalid error code. This error code should never be used outside of development.",
			"The format of the command was incorrect. This error code is not used in the current implementation, "
			"because @ref NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED and @ref NRF_DFU_RES_CODE_INVALID_PARAMETER cover all "
			"possible format errors.",
			"The command was successfully parsed, but it is not supported or unknown.",
			"The init command is invalid. The init packet either has an invalid update type or it is missing required "
			"fields for the update type (for example, the init packet for a SoftDevice update is missing the "
			"SoftDevice size field).",
			"The firmware version is too low. For an application, the version must be greater than or equal to the "
			"current application. For a bootloader, it must be greater than the current version. This requirement "
			"prevents downgrade attacks.",
			"The hardware version of the device does not match the required hardware version for the update.",
			"The array of supported SoftDevices for the update does not contain the FWID of the current SoftDevice.",
			"The init packet does not contain a signature, but this bootloader requires all updates to have one.",
			"The hash type that is specified by the init packet is not supported by the DFU bootloader.",
			"The hash of the firmware image cannot be calculated.",
			"The type of the signature is unknown or not supported by the DFU bootloader.",
			"The hash of the received firmware image does not match the hash in the init packet.",
			"The available space on the device is insufficient to hold the firmware.",
			"The requested firmware to update was already present on the system."};

	if (errorcode < CsUtils::ArraySize(codes)) {
		return codes[errorcode];
	}

	return "Errorcode out of bounds";
}
}  // namespace DfuTransportBle

}  //  namespace MeshDfuConstants
