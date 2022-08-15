/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <structs/cs_PacketsInternal.h>

enum TrackedDeviceFields {
	BIT_POS_ACCESS_LEVEL = 0,
	BIT_POS_LOCATION     = 1,
	BIT_POS_PROFILE      = 2,
	BIT_POS_RSSI_OFFSET  = 3,
	BIT_POS_FLAGS        = 4,
	BIT_POS_DEVICE_TOKEN = 5,
	BIT_POS_TTL          = 6,
};
static const uint8_t ALL_FIELDS_SET = 0x7F;

struct __attribute__((packed)) TrackedDevice {

	// Bitmask to keep up which fields are set, with TrackedDeviceFields as bits.
	uint8_t fieldsSet = 0;
	uint8_t locationIdTTLMinutes;
	uint8_t heartbeatTTLMinutes;
	internal_register_tracked_device_packet_t data;

	device_id_t id();

	bool isValid();

	void invalidate();

	bool allFieldsSet();

	void setAccessLevel(uint8_t accessLevel);
	void setLocation(uint8_t locationId, uint8_t timeoutMinutes);
	void setProfile(uint8_t profileId);
	void setRssiOffset(int8_t rssiOffset);
	void setFlags(uint8_t flags);
	void setDevicetoken(uint8_t* deviceToken, uint8_t size);
	void setTTL(uint16_t ttlMinutes);
};
