/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <ble/cs_UuidHelper.h>
#include <protocol/cs_ErrorCodes.h>
#include <util/cs_UuidParser.h>

//ble_uuid128_t UuidHelper::fromString(const char* fullUuid, uint8_t len) {
//	ble_uuid128_t uuid = {};
//	BLEutil::parseUuid(fullUuid, len, uuid.uuid128, sizeof(uuid.uuid128));
//	return uuid;
//}

cs_ret_code_t UuidHelper::add(const ble_uuid_t& baseUuid, uint16_t shortUuid, ble_uuid_t& resultUuid) {
	if (baseUuid.type < BLE_UUID_TYPE_VENDOR_BEGIN) {
		return ERR_WRONG_PARAMETER;
	}
	resultUuid.type = baseUuid.type;
	resultUuid.uuid = shortUuid;
	return ERR_SUCCESS;
}

cs_ret_code_t UuidHelper::add(const ble_uuid128_t& fullUuid, ble_uuid_t& resultUuid) {
	cs_ret_code_t retCode;
	retCode = getFromCache(fullUuid, resultUuid);
	switch (retCode) {
		case ERR_SUCCESS: {
			// UUID was already registered.
			return ERR_SUCCESS;
		}
		case ERR_NOT_FOUND: {
			// Register UUID
			retCode = add(fullUuid, resultUuid.type);
			if (retCode != ERR_SUCCESS) {
				return retCode;
			}
			return getFromCache(fullUuid, resultUuid);
		}
	}
	return ERR_UNSPECIFIED;
}

cs_ret_code_t UuidHelper::getFromCache(const ble_uuid128_t& fullUuid, ble_uuid_t& resultUuid) {
	uint32_t nrfCode = sd_ble_uuid_decode(sizeof(fullUuid.uuid128), fullUuid.uuid128, &resultUuid);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			return ERR_SUCCESS;
		}
		case NRF_ERROR_NOT_FOUND: {
			return ERR_NOT_FOUND;
		}
	}
	return ERR_UNSPECIFIED;
}

cs_ret_code_t UuidHelper::add(const ble_uuid128_t& fullUuid, uint8_t& type) {
	uint32_t nrfCode = sd_ble_uuid_vs_add(&fullUuid, &type);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			return ERR_SUCCESS;
		}
		case NRF_ERROR_NO_MEM: {
			return ERR_NO_SPACE;
		}
	}
	return ERR_UNSPECIFIED;
}

cs_ret_code_t UuidHelper::rem(const ble_uuid_t& shortUuid) {
	// If the type is set to @ref BLE_UUID_TYPE_UNKNOWN, or the pointer is NULL,
	// the last Vendor Specific base UUID will be removed.
	if (shortUuid.type == BLE_UUID_TYPE_UNKNOWN) {
		return ERR_WRONG_PARAMETER;
	}

	// The remove function changes the type, we don't want that.
	uint8_t type = shortUuid.type;

	uint32_t nrfCode = sd_ble_uuid_vs_remove(&type);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			return ERR_SUCCESS;
		}
		case NRF_ERROR_INVALID_ADDR:
		case NRF_ERROR_INVALID_PARAM: {
			return ERR_NOT_FOUND;
		}
		case NRF_ERROR_FORBIDDEN: {
			return ERR_BUSY;
		}
	}
	return ERR_UNSPECIFIED;
}


