/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <localisation/cs_AssetFilterStore.h>
#include <localisation/cs_AssetFilterSyncer.h>

class AssetFiltering : EventListener {
public:
	cs_ret_code_t init();

private:
	AssetFilterStore* _filterStore;
	AssetFilterSyncer* _filterSyncer;

	/**
	 * Dispatches a TrackedEvent for the given advertisement.
	 */
	void handleScannedDevice(const scanned_device_t& device);

	void logServiceData(scanned_device_t* scannedDevice);
public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};


