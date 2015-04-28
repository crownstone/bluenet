/**
 * Author: Christopher Mason
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Feb 23, 2015
 * License: LGPLv3+
 */

#include "ble/cs_UUID.h"

#include "util/cs_BleError.h"

namespace BLEpp {

// TODO: CRAPPY IMPLEMENTATION!!!! implement in a clean, methodical and understandable way

/// UUID //////////////////////////////////////////////////////////////////////////////////////////

uint16_t UUID::init() {

	if (_full && _type == BLE_UUID_TYPE_UNKNOWN) {
		ble_uuid128_t u = *this;

		ble_uuid_t uu;

		uint32_t error_code = sd_ble_uuid_decode(16, u.uuid128, &uu);

		if (error_code == NRF_ERROR_NOT_FOUND) {
			BLE_CALL(sd_ble_uuid_vs_add, (&u, &_type));

			_uuid = (uint16_t) (u.uuid128[13] << 8 | u.uuid128[12]);
		} else if (error_code == NRF_SUCCESS) {
			_type = uu.type;
			_uuid = uu.uuid;
		} else {
			BLE_THROW("Failed to add uuid.");
		}
	} else if (_type == BLE_UUID_TYPE_UNKNOWN) {
		BLE_THROW("TODO generate random UUID");
	} else {
		// nothing to do.
	}
	return _type;
}

UUID::operator ble_uuid_t() {
	ble_uuid_t ret;
	ret.uuid = _uuid;
	ret.type = _type;

	return ret;
}

UUID::operator ble_uuid128_t() {
	ble_uuid128_t res;

	int i = 0;
	int j = 0;
	int k = 0;
	uint8_t c;
	uint8_t v = 0;
	for (; ((c = _full[i]) != 0) && (j < 16); i++) {
		uint8_t vv = 0;

		if (c == '-' || c == ' ') {
			continue;
		} else if (c >= '0' && c <= '9') {
			vv = c - '0';
		} else if (c >= 'A' && c <= 'F') {
			vv = c - 'A' + 10;
		} else if (c >= 'a' && c <= 'f') {
			vv = c - 'a' + 10;
		} else {
			BLE_THROW("invalid character");
//				char cc[] = {c};// can't just call std::string(c) apparently.
//				BLE_THROW(std::string("Invalid character ") + std::string(1,cc[0]) + " in UUID.");
		}

		v = v * 16 + vv;

		if (k++ % 2 == 1) {
			res.uuid128[15 - (j++)] = v;
			v = 0;
		}

	}
	if (j < 16) {
		BLE_THROW("UUID too short.");
	} else if (_full[i] != 0) {
		BLE_THROW("UUID too long.");
	}

	return res;
}

}
