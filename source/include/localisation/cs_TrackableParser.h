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
	constexpr static size_t FILTER_BUFFER_SIZE = 512;

private:
	/**
	 * Dispatches a TrackedEvent for the given advertisement.
	 */
	void handleBackgroundParsed(adv_background_parsed_t *trackableAdvertisement);

	// -------------------------------------------------------------
	// --------------- Filter definitions and storage --------------
	// -------------------------------------------------------------

	// working space for the filters.
	uint8_t _filterBuffer[FILTER_BUFFER_SIZE];

	class FilterMetaData {
	public:
		uint8_t profileId; // devices passing the filter are assigned this profile id
		uint8_t inputType; // determines wether the filter has mac or advertisement data as input

		// sync info
		uint8_t protocol; // determines implementation type of filter
		uint16_t version : 12; // Lollipop
		uint16_t unused : 4; // explicit padd

		// sync state
		union{
			uint8_t flags;
			struct {
				uint8_t isActive : 1;
			} bits;
		};
	};

	/**
	 * The filters and their associated metadata.
	 * Allocated on the FilterBuffer.
	 */
	CuckooFilter* _filters[MAX_FILTER_IDS];
	FilterMetaData* _filterMetadata[MAX_FILTER_IDS];
	uint16_t _filterCrcs[MAX_FILTER_IDS]; // since uint16_t is smaller than a pointer, we save space by allocating these here instead of in the buffer.

	uint16_t _masterHash; //
	uint16_t _masterVersion; // Lollipop @Persisted

	// -------------------------------------------------------------
	// ------------------ Internal filter management ---------------
	// -------------------------------------------------------------

	/**
	 * Constructs a CuckooFilter in the _filterBuffer together with
	 * the necessary fingerprint buffer.
	 */
	bool allocateCuckooFilter(uint8_t filterId, uint8_t bucket_count, uint8_t nests_per_bucket);

	// -------------------------------------------------------------
	// ---------------------- Command interface --------------------
	// -------------------------------------------------------------

	/**
	 * Returns:
	 *  - master version
	 *  - master crc
	 *  - for each filter:
	 *    - filter id
	 *    - filter version
	 *    - filter crc
	 */
	void handleGetFilterSummariesCommand();


	/**
	 * Inactive filters are activated.
	 * Crcs are (re)computed.
	 * This crownstones master version and crc are broadcasted over the mesh.
	 * Sets 'filter modification in progress' flag of this crownstone back to off.
	 */
	void handleCommitFilterChangesCommand(uint16_t masterversion, uint16_t mastercrc);

	/**
	 * Removes given filter immediately.
	 * Flags this crownstone as 'filter modification in progress'.
	 */
	void handleRemoveFilterCommand(uint8_t filterId);

	/**
	 * Upon first reception of this command with the given filter_id,
	 * reallocate space. If this fails, abort. Else set the filter_id to
	 * 'upload in progress'.
	 */
	void handleUploadFilterCommand(uint8_t filterId, uint16_t chunkStartIndex, uint16_t totalSize, uint8_t* chunk, uint16_t chunkSize);

	// -------------------------------------------------------------
	// ----------------------- OLD interface -----------------------
	// -------------------------------------------------------------


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
