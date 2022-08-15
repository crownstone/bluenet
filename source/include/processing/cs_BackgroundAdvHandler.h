/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 19, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "ble/cs_Nordic.h"
#include "events/cs_EventListener.h"
#include "util/cs_Utils.h"

/**
 * Class that parses advertisements for background broadcasts.
 *
 * Receives data from either EVT_DEVICE_SCANNED, or EVT_ADV_BACKGROUND.
 * Parses and decrypts.
 * Sends event EVT_ADV_BACKGROUND_PARSED.
 */
class BackgroundAdvertisementHandler : public EventListener {
public:
	static BackgroundAdvertisementHandler& getInstance() {
		static BackgroundAdvertisementHandler instance;
		return instance;
	}
	void handleEvent(event_t& event);

private:
	/**
	 * Own sphere id.
	 */
	TYPIFY(CONFIG_SPHERE_ID) _sphereId = 0;

	/**
	 * Map 16 bit service UUIDs to bit positions in the 128b bitmask.
	 */
	uint8_t _uuidMap[256]              = {
            255, 255, 255, 255, 47,  255, 255, 255, 255, 111, 71,  10,  255, 115, 255, 255, 91,  90,  37,  85,
            126, 74,  69,  57,  56,  50,  46,  44,  40,  38,  36,  35,  80,  51,  31,  28,  27,  25,  26,  117,
            21,  65,  98,  64,  8,   20,  22,  15,  14,  13,  4,   255, 255, 62,  255, 255, 255, 255, 107, 255,
            255, 43,  79,  255, 48,  255, 59,  123, 255, 255, 58,  30,  255, 255, 11,  255, 255, 255, 255, 114,
            255, 255, 55,  39,  255, 255, 109, 255, 118, 255, 61,  255, 255, 93,  255, 33,  255, 121, 78,  94,
            255, 255, 0,   92,  7,   255, 255, 81,  255, 255, 255, 9,   255, 32,  255, 255, 255, 255, 255, 112,
            255, 255, 54,  255, 255, 255, 67,  255, 255, 255, 255, 72,  255, 76,  6,   255, 42,  41,  105, 24,
            18,  255, 97,  86,  255, 120, 2,   255, 255, 255, 255, 19,  255, 110, 16,  255, 255, 255, 53,  255,
            99,  255, 255, 255, 255, 77,  255, 255, 255, 17,  255, 73,  255, 116, 255, 255, 127, 255, 255, 255,
            29,  106, 255, 255, 255, 255, 66,  83,  95,  255, 255, 124, 255, 122, 255, 255, 255, 87,  255, 255,
            68,  255, 101, 255, 255, 255, 34,  255, 49,  63,  255, 255, 45,  255, 255, 84,  23,  255, 255, 96,
            255, 113, 5,   52,  119, 255, 60,  255, 12,  89,  103, 255, 3,   255, 255, 88,  102, 255, 255, 255,
            125, 255, 255, 255, 70,  82,  104, 255, 1,   75,  108, 255, 100, 255, 255, 255,
    };

	/**
	 * Store the last mapped bitmask.
	 */
	uint64_t _lastBitmask[2]                 = {0};

	/**
	 * Store the address of the last bitmask.
	 */
	uint8_t _lastMacAddress[MAC_ADDRESS_LEN] = {0};

	BackgroundAdvertisementHandler();

	/**
	 * Parse, decrypt, and validate an advertisement.
	 */
	void parseAdvertisement(scanned_device_t* scannedDevice);

	/**
	 * Parse an advertisement with incomplete list of service UUIDs.
	 *
	 * These get mapped to a bitmask, and stored together with the mac address.
	 */
	void parseServicesAdvertisement(scanned_device_t* scannedDevice);

	/**
	 * Handle a validated background advertisement.
	 *
	 * Adjusts RSSI, and send event.
	 */
	void handleBackgroundAdvertisement(adv_background_t* backgroundAdvertisement);

	/**
	 * Return the adjusted RSSI value, given the actual RSSI and the offset from the background advertisement payload.
	 */
	int8_t getAdjustedRssi(int16_t rssi, int16_t rssiOffset);
};
