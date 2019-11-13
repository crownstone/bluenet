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
	BackgroundAdvertisementHandler();

	/**
	 * Parse, decrypt, and validate an advertisement.
	 */
	void parseAdvertisement(scanned_device_t* scannedDevice);

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


