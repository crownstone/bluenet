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
 * Transforms EVT_DEVICE_SCANNED into iBeacon events.
 *
 * Filters company ids to prevent memory overflow.
 */
class IBeaconParser : public EventListener {
public:
	void handleEvent(event_t& evt);

private:
	void /* IBeacon */ getIBeacon(scanned_device_t* scanned_device);


	/**
	 *  Checks service uuids of the scanned device and returns true
	 *  if we can find a 16 bit tile service uuid.
	 *
	 *  Caveat: expects the services to be listed in the type
	 *  BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE.
	 */
	bool isTileDevice(scanned_device_t* scanned_device);
};
