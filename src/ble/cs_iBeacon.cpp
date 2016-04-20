/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <ble/cs_iBeacon.h>

using namespace BLEpp;

IBeacon::IBeacon(ble_uuid128_t uuid, uint16_t major, uint16_t minor,
		int8_t rssi) {
	//! advertisement indicator for an iBeacon is defined as 0x0215
	_params.adv_indicator = BLEutil::convertEndian16(0x0215);
	setUUID(uuid);
	setMajor(major);
	setMinor(minor);
	setRSSI(rssi);
}

void IBeacon::setMajor(uint16_t major) {
	LOGd("setMajor: %d", major);
	_params.major = BLEutil::convertEndian16(major);
}

uint16_t IBeacon::getMajor() {
	return BLEutil::convertEndian16(_params.major);
}

void IBeacon::setMinor(uint16_t minor) {
	LOGd("setMinor: %d", minor);
	_params.minor = BLEutil::convertEndian16(minor);
}

uint16_t IBeacon::getMinor() {
	return BLEutil::convertEndian16(_params.minor);
}

void IBeacon::setUUID(ble_uuid128_t uuid) {
	LOGd("setUUID");
	_params.uuid = uuid;
}

ble_uuid128_t IBeacon::getUUID() {
	return _params.uuid;
}

void IBeacon::setRSSI(int8_t rssi) {
	LOGd("setRSSI: %d", rssi);
	_params.rssi = rssi;
}

int8_t IBeacon::getRSSI() {
	return _params.rssi;
}
