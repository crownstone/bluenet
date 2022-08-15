/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_iBeacon.h>
#include <cfg/cs_Strings.h>

#include <algorithm>  // for reverse copy

IBeacon::IBeacon(cs_uuid128_t uuid, uint16_t major, uint16_t minor, int8_t rssi) {
	// advertisement indicator for an iBeacon is defined as 0x0215
	_params.adv_indicator = CsUtils::convertEndian16(0x0215);
	setUUID(uuid);
	setMajor(major);
	setMinor(minor);
	setTxPower(rssi);
}

void IBeacon::setMajor(uint16_t major) {
	_params.major = CsUtils::convertEndian16(major);
}

uint16_t IBeacon::getMajor() {
	return CsUtils::convertEndian16(_params.major);
}

void IBeacon::setMinor(uint16_t minor) {
	_params.minor = CsUtils::convertEndian16(minor);
}

uint16_t IBeacon::getMinor() {
	return CsUtils::convertEndian16(_params.minor);
}

void IBeacon::setUUID(cs_uuid128_t& uuid) {
	std::reverse_copy(uuid.uuid128, uuid.uuid128 + 16, _params.uuid.uuid128);
}

cs_uuid128_t IBeacon::getUUID() {
	cs_uuid128_t uuid;
	std::reverse_copy(_params.uuid.uuid128, _params.uuid.uuid128 + 16, uuid.uuid128);

	return uuid;
}

void IBeacon::setTxPower(int8_t txPower) {
	_params.txPower = txPower;
}

int8_t IBeacon::getTxPower() {
	return _params.txPower;
}
