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
	void handleEvent(event_t & event);


private:
	/**
	 * Own sphere id.
	 */
	TYPIFY(CONFIG_SPHERE_ID) _sphereId = 0;

	/**
	 * Map 16 bit service UUIDs to bit positions in the 128b bitmask.
	 */
	uint8_t _uuidMap[256] = {
			255, 255, 255, 255,  80, 255, 255, 255, 255,  16,  56, 117, 255,  12, 255, 255,
			 36,  37,  90,  42,   1,  53,  58,  70,  71,  77,  81,  83,  87,  89,  91,  92,
			 47,  76,  96,  99, 100, 102, 101,  10, 106,  62,  29,  63, 119, 107, 105, 112,
			113, 114, 123, 255, 255,  65, 255, 255, 255, 255,  20, 255, 255,  84,  48, 255,
			 79, 255,  68,   4, 255, 255,  69,  97, 255, 255, 116, 255, 255, 255, 255,  13,
			255, 255,  72,  88, 255, 255,  18, 255,   9, 255,  66, 255, 255,  34, 255,  94,
			255,   6,  49,  33, 255, 255, 127,  35, 120, 255, 255,  46, 255, 255, 255, 118,
			255,  95, 255, 255, 255, 255, 255,  15, 255, 255,  73, 255, 255, 255,  60, 255,
			255, 255, 255,  55, 255,  51, 121, 255,  85,  86,  22, 103, 109, 255,  30,  41,
			255,   7, 125, 255, 255, 255, 255, 108, 255,  17, 111, 255, 255, 255,  74, 255,
			 28, 255, 255, 255, 255,  50, 255, 255, 255, 110, 255,  54, 255,  11, 255, 255,
			  0, 255, 255, 255,  98,  21, 255, 255, 255, 255,  61,  44,  32, 255, 255,   3,
			255,   5, 255, 255, 255,  40, 255, 255,  59, 255,  26, 255, 255, 255,  93, 255,
			 78,  64, 255, 255,  82, 255, 255,  43, 104, 255, 255,  31, 255,  14, 122,  75,
			  8, 255,  67, 255, 115,  38,  24, 255, 124, 255, 255,  39,  25, 255, 255, 255,
			  2, 255, 255, 255,  57,  45,  23, 255, 126,  52,  19, 255,  27, 255, 255, 255,
	};

	/**
	 * Store the last mapped bitmask.
	 */
	uint64_t _lastBitmask[2] = {0};

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


