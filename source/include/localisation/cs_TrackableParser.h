/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <ble/cs_iBeacon.h>


/**
 * Transforms EVT_DEVICE_SCANNED and EVT_ADV_BACKGROUND_PARSED
 * into EVT_TRACKING_UPDATE events.
 *
 * Responsible for throttling input to the localisation module, which
 * may take up more computation resources.
 *
 * Suggestion: This component seems like a natural place to handle
 * identification when ids rotate by generating a (crowntone) mesh
 * local identifier as an abstraction (and size reduction) layer.
 */
class TrackableParser : public EventListener {
public:
	void handleEvent(event_t& evt);

private:
	/**
	 * Check if this device is a Tile device, if so handle it and
	 * return true. Else, return false.
	 */
	bool handleAsTileDevice(scanned_device_t* scanned_device);

	/**
	 * Checks if the mac address of the trackable device is equal
	 * to myTrackableId.
	 */
	bool isMyTrackable(scanned_device_t* scanned_device);

	/**
	 *  Checks service uuids of the scanned device and returns true
	 *  if we can find an official 16 bit Tile service uuid.
	 *
	 *  Caveats:
	 *  - Tiles are not iBeacons.
	 *  - expects the services to be listed in the type
	 *    BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE.
	 */
	bool isTileDevice(scanned_device_t* scanned_device);

	void logServiceData(scanned_device_t* scanned_device);
};
