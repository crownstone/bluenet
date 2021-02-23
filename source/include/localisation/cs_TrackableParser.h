/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_iBeacon.h>
#include <events/cs_EventListener.h>

#include <third/cuckoo/CuckooFilter.h>

/**
 * Transforms EVT_DEVICE_SCANNED and EVT_ADV_BACKGROUND_PARSED
 * into EVT_TRACKING_UPDATE events.
 *
 * Responsible for throttling input to the localisation module, which
 * may take up more computation resources.
 */
class TrackableParser : public EventListener {
public:
	void init();
	void handleEvent(event_t& evt);

	constexpr static size_t MAX_FILTER_IDS = 8;

private:
	/**
	 * Dispatches a TrackedEvent for the given advertisement.
	 */
	void handleBackgroundParsed(adv_background_parsed_t *trackableAdvertisement);




	class FilterMetaData {
	public:
		uint16_t version : 12; // Lollipop
		uint8_t protocolVersion; // ?

		uint8_t profileId; // devices passing the filter are assigned this profile id
		uint8_t inputType; // determines wether the filter has mac or advertisement data as input

		union{
			uint8_t flags;
			struct {
				uint8_t isActive : 1;
				uint8_t pendingRemoval : 1;
			} bits;
		};
	};

	/**
	 * The filters and their associated metadata.
	 */
	CuckooFilter _filters[MAX_FILTER_IDS];
	FilterMetaData _filterMetadata[MAX_FILTER_IDS];
	uint16_t _filterCrcs[MAX_FILTER_IDS];

	/**
	 * Combined metadata for the filters.
	 */
	uint16_t _masterHash;
	uint16_t _masterVersion; // Lollipop


	HasValue<fingerprint> filter(void* data, size_t len);





	/**
	 * Check if this device is a Tile device, if so handle it and
	 * return true. Else, return false.
	 *
	 * Will emit a TrackableEvent when it is a tile device and mac
	 * matches the hard coded address to filter for.
	 */
	bool handleAsTileDevice(scanned_device_t* scannedDevice);

	/**
	 * Checks if the mac address of the trackable device is equal
	 * to myTrackableId.
	 */
	bool isMyTrackable(scanned_device_t* scannedDevice);

	/**
	 *  Checks service uuids of the scanned device and returns true
	 *  if we can find an official 16 bit Tile service uuid.
	 *
	 *  Caveats:
	 *  - Tiles are not iBeacons.
	 *  - expects the services to be listed in the type
	 *    BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE.
	 */
	bool isTileDevice(scanned_device_t* scannedDevice);

	void logServiceData(const char* headerStr, scanned_device_t* scannedDevice);
};
