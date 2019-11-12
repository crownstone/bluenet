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

class BackgroundAdvertisementHandler : public EventListener {
public:
	static BackgroundAdvertisementHandler& getInstance() {
		static BackgroundAdvertisementHandler instance;
		return instance;
	}
	void handleEvent(event_t & event);


private:
	BackgroundAdvertisementHandler();

	void parseAdvertisement(scanned_device_t* scannedDevice);
	void handleBackgroundAdvertisement(adv_background_t* backgroundAdvertisement);
	int8_t adjustRssi(int16_t rssi, int16_t rssiOffset);
};


