/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <ble/cs_UUID.h>
#include <protocol/cs_ErrorCodes.h>
#include <cstring>
#include <util/cs_UuidParser.h>
#include <util/cs_BleError.h>

UUID::UUID() {}

UUID::UUID(ble_uuid_t uuid):
		_uuid(uuid)
{}

UUID::UUID(const char* fullUuid) {
	ble_uuid128_t uuid;
	bool success = BLEutil::parseUuid(fullUuid, strlen(fullUuid), uuid.uuid128, sizeof(uuid.uuid128));
	if (!success) {
		APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
	}

	ret_code_t nrfCode = fromFullUuidInternal(uuid);
	APP_ERROR_CHECK(nrfCode);
}

UUID::UUID(uint16_t shortUuid) {
	ret_code_t nrfCode = fromShortUuidInternal(shortUuid);
	APP_ERROR_CHECK(nrfCode);
}

UUID::UUID(const UUID& baseUuid, uint16_t shortUuid) {
	ret_code_t nrfCode = fromBaseUuidInternal(baseUuid, shortUuid);
	APP_ERROR_CHECK(nrfCode);
}

///// Public functions /////

cs_ret_code_t UUID::fromFullUuid(const char* fullUuid) {
	ble_uuid128_t uuid;
	bool success = BLEutil::parseUuid(fullUuid, strlen(fullUuid), uuid.uuid128, sizeof(uuid.uuid128));
	if (!success) {
		return ERR_WRONG_PARAMETER;
	}
	return fromFullUuid(uuid);
}

cs_ret_code_t UUID::fromFullUuid(const ble_uuid128_t& fullUuid) {
	ret_code_t nrfCode = fromFullUuidInternal(fullUuid);
	return fromNrfCode(nrfCode);
}

cs_ret_code_t UUID::fromShortUuid(uint16_t shortUuid) {
	ret_code_t nrfCode = fromShortUuidInternal(shortUuid);
	return fromNrfCode(nrfCode);
}

cs_ret_code_t UUID::fromBaseUuid(const UUID& baseUuid, uint16_t shortUuid) {
	ret_code_t nrfCode = fromBaseUuidInternal(baseUuid, shortUuid);
	return fromNrfCode(nrfCode);
}

ble_uuid_t UUID::getUuid() const {
	return _uuid;
}

///// Internal functions /////

ret_code_t UUID::fromShortUuidInternal(uint16_t shortUuid) {
	_uuid.type = BLE_UUID_TYPE_BLE;
	_uuid.uuid = shortUuid;
	return NRF_SUCCESS;
}

ret_code_t UUID::fromBaseUuidInternal(const UUID& baseUuid, uint16_t shortUuid) {
	if (baseUuid._uuid.type == BLE_UUID_TYPE_UNKNOWN) {
		return NRF_ERROR_INVALID_PARAM;
	}
	_uuid.type = baseUuid._uuid.type;
	_uuid.uuid = shortUuid;
	return NRF_SUCCESS;
}

ret_code_t UUID::fromFullUuidInternal(const ble_uuid128_t& fullUuid) {
	ret_code_t nrfCode;
	nrfCode = getFromCache(fullUuid);
	switch (nrfCode) {
		case NRF_SUCCESS: {
			// UUID was already registered.
			return ERR_SUCCESS;
		}
		case NRF_ERROR_NOT_FOUND: {
			// Register UUID
			nrfCode = add(fullUuid);
			if (nrfCode != NRF_SUCCESS) {
				return nrfCode;
			}
			return getFromCache(fullUuid);
		}
	}
	return nrfCode;
}

ret_code_t UUID::getFromCache(const ble_uuid128_t& fullUuid) {
	uint32_t nrfCode = sd_ble_uuid_decode(sizeof(fullUuid.uuid128), fullUuid.uuid128, &_uuid);
	return nrfCode;
}

ret_code_t UUID::add(const ble_uuid128_t& fullUuid) {
	uint32_t nrfCode = sd_ble_uuid_vs_add(&fullUuid, &(_uuid.type));
	return nrfCode;
}

ret_code_t UUID::rem(const ble_uuid_t& uuid) {
	// If the type is set to @ref BLE_UUID_TYPE_UNKNOWN, or the pointer is NULL,
	// the last Vendor Specific base UUID will be removed.
	if (uuid.type < BLE_UUID_TYPE_VENDOR_BEGIN) {
		return ERR_WRONG_PARAMETER;
	}

	// The remove function changes the type, we don't want that.
	uint8_t type = uuid.type;

	uint32_t nrfCode = sd_ble_uuid_vs_remove(&type);
	return nrfCode;
}

cs_ret_code_t UUID::fromNrfCode(ret_code_t nrfCode) {
	switch (nrfCode) {
		case NRF_SUCCESS:              return ERR_SUCCESS;
		case NRF_ERROR_NO_MEM:         return ERR_NO_SPACE;
		case NRF_ERROR_NOT_FOUND:      return ERR_NOT_FOUND;

		// We only get these in rem()
		case NRF_ERROR_INVALID_PARAM:  return ERR_WRONG_PARAMETER;
		case NRF_ERROR_FORBIDDEN:      return ERR_BUSY;
		default:                       return ERR_UNSPECIFIED;
	}
}

bool UUID::operator==(const UUID& other) {
	return (_uuid.uuid == other._uuid.uuid) && (_uuid.type == other._uuid.type);
//	return memcmp(&_uuid, &(other._uuid), sizeof(_uuid)) == 0; // Bad idea, as the struct is not packed.
}
