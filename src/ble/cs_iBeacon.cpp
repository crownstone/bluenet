/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <ble/cs_iBeacon.h>
#include <cfg/cs_Strings.h>

using namespace BLEpp;

//#define PRINT_VERBOSE

IBeacon::IBeacon(ble_uuid128_t uuid, uint16_t major, uint16_t minor,
		int8_t rssi) {
	//! advertisement indicator for an iBeacon is defined as 0x0215
	_params.adv_indicator = BLEutil::convertEndian16(0x0215);
	setUUID(uuid);
	setMajor(major);
	setMinor(minor);
	setTxPower(rssi);
}

void IBeacon::setMajor(uint16_t major) {
#ifdef PRINT_VERBOSE
	LOGd(FMT_SET_INT_VAL, "major", major);
#endif

	_params.major = BLEutil::convertEndian16(major);
}

uint16_t IBeacon::getMajor() {
	return BLEutil::convertEndian16(_params.major);
}

void IBeacon::setMinor(uint16_t minor) {
#ifdef PRINT_VERBOSE
	LOGd(FMT_SET_INT_VAL, "minor", minor);
#endif

	_params.minor = BLEutil::convertEndian16(minor);
}

uint16_t IBeacon::getMinor() {
	return BLEutil::convertEndian16(_params.minor);
}

void IBeacon::setUUID(ble_uuid128_t uuid) {
	for (int i = 0; i < 16; ++i) {
		_params.uuid.uuid128[i] = uuid.uuid128[16-1-i];
	}

#ifdef PRINT_VERBOSE
	log(DEBUG, FMT_SET_STR_VAL "UUID", "");
	BLEutil::printInlineArray(_params.uuid.uuid128, 16);
	_log(DEBUG, CRLN);
#endif
}

ble_uuid128_t IBeacon::getUUID() {
	ble_uuid128_t uuid;
	for (int i = 0; i < 16; ++i) {
		uuid.uuid128[i] = _params.uuid.uuid128[16-1-i];
	}
	return uuid;
}

void IBeacon::setTxPower(int8_t txPower) {
#ifdef PRINT_VERBOSE
	LOGd(FMT_SET_INT_VAL, "tx power", txPower);
#endif

	_params.txPower = txPower;
}

int8_t IBeacon::getTxPower() {
	return _params.txPower;
}
