/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <tracking/cs_TrackedDevice.h>
#include <util/cs_Utils.h>


bool TrackedDevice::isValid() {
	return fieldsSet != 0;
}

void TrackedDevice::invalidate() {
	fieldsSet = 0;
}

bool TrackedDevice::allFieldsSet() {
	return fieldsSet == ALL_FIELDS_SET;
}

void TrackedDevice::setAccessLevel(uint8_t accessLevel) {
	data.accessLevel = accessLevel;
	CsUtils::setBit(fieldsSet, BIT_POS_ACCESS_LEVEL);
}

void TrackedDevice::setLocation(uint8_t locationId, uint8_t timeoutMinutes) {
	data.data.locationId = locationId;
	CsUtils::setBit(fieldsSet, BIT_POS_LOCATION);
	locationIdTTLMinutes = timeoutMinutes;
}

void TrackedDevice::setProfile(uint8_t profileId) {
	data.data.profileId = profileId;
	CsUtils::setBit(fieldsSet, BIT_POS_PROFILE);
}

void TrackedDevice::setRssiOffset(int8_t rssiOffset) {
	data.data.rssiOffset = rssiOffset;
	CsUtils::setBit(fieldsSet, BIT_POS_RSSI_OFFSET);
}

void TrackedDevice::setFlags(uint8_t flags) {
	data.data.flags.asInt = flags;
	CsUtils::setBit(fieldsSet, BIT_POS_FLAGS);
}

void TrackedDevice::setDevicetoken(uint8_t* deviceToken, uint8_t size) {
	assert(size == TRACKED_DEVICE_TOKEN_SIZE, "Wrong device token size");
	memcpy(data.data.deviceToken, deviceToken, sizeof(data.data.deviceToken));
	CsUtils::setBit(fieldsSet, BIT_POS_DEVICE_TOKEN);
}

void TrackedDevice::setTTL(uint16_t ttlMinutes) {
	data.data.timeToLiveMinutes = ttlMinutes;
	CsUtils::setBit(fieldsSet, BIT_POS_TTL);
}
