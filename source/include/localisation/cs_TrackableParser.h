/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_iBeacon.h>
#include <events/cs_EventListener.h>
#include <protocol/cs_TrackableParserPackets.h>

#include <third/cuckoo/CuckooFilter.h>

/**
 * Transforms EVT_DEVICE_SCANNED and EVT_ADV_BACKGROUND_PARSED
 * into EVT_TRACKING_UPDATE events.
 *
 * Responsible for throttling input to the localisation module using filter parsers.
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
	// ------------------ Filter buffer definitions ----------------
	// -------------------------------------------------------------

	uint8_t _filterBuffer[FILTER_BUFFER_SIZE];

	// keeps track of first empty byte in buffer.
	size_t _filterBufferEndIndex = 0;

	// -------------------------------------------------------------
	// --------------- Filter definitions and storage --------------
	// -------------------------------------------------------------

	/**
	 * Data associated to a parsing filter that is persisted to flash.
	 */
	struct __attribute__((__packed__)) FilterMetaData {
	public:
		// sync info
		uint8_t protocol; // determines implementation type of filter
		union {
			struct {
				uint16_t value: 12; // Lollipop
				uint16_t unused : 4; // explicit padd
			} version;
			uint16_t version_int;
		};

		uint8_t profileId; // devices passing the filter are assigned this profile id
		uint8_t inputType; // determines wether the filter has mac or advertisement data as input

		// sync state
		union{
			uint8_t flags;
			struct {
				uint8_t isActive : 1;
			} bits;
		};
	};

	/**
	 * Data associated to a parsing filter that is not persisted to flash.
	 */
	struct __attribute__((__packed__)) FilterRuntimeData {
		uint8_t filterId; 				// can be runtime only since it's saved as part of record metadata on flash.
		uint16_t crc;
	};

	/**
	 * The filters and their associated metadata.
	 * Allocated on the FilterBuffer are packed into a single
	 * struct. That saves a bit of administration.
	 *
	 * Warning: keep this structure and order as-is, that makes chunking
	 * the whole thing a stupid load simpler.
	 */
	struct __attribute__((__packed__)) ParsingFilter {
		FilterRuntimeData runtimedata;
		FilterMetaData metadata; 		// must be kept next to cuckoo filter to make copy to flash a simple memcpy.
		CuckooFilter filter;			// must be last item to give space to flexible array

		size_t size() {
			return sizeof(ParsingFilter) + filter.bufferSize();
		}
	};

	// nullptr terminated list of filters pointing into the _filterBuffer.
	ParsingFilter* _parsingFilters[MAX_FILTER_IDS];

	// keeps track of first empty filter pointer in list.
	uint8_t _parsingFiltersEndIndex;

	uint16_t _masterHash; //
	uint16_t _masterVersion; // Lollipop @Persisted

	// -------------------------------------------------------------
	// ------------------ Internal filter management ---------------
	// -------------------------------------------------------------

	/**
	 * Returns a pointer in the _filterBuffer, promising that at least
	 * totalsSize bytes space starting from that address are free to use.
	 *
	 * Returns nullptr on failure.
	 *
	 * Internally adjusts _filterBufferEnd to point to one byte after this array.
	 */
	ParsingFilter* allocateParsingFilter(uint8_t filterId, size_t totalSize);

	/**
	 * Readjust the filterbuffer to create space at the back. Adjust the
	 * _parsingFilter array too.
	 */
	void deallocateParsingFilter(uint8_t filterId);

	/**
	 * Looks up given filter id in the list of filters. Returns nullptr if not found.
	 * Assumes _parsingFilters is nullptr terminated
	 */
	ParsingFilter* findParsingFilter(uint8_t filterId);

	// -------------------------------------------------------------
	// ---------------------- Command interface --------------------
	// -------------------------------------------------------------

	/**
	 * Upon first reception of this command with the given filter_id,
	 * allocate space in the buffer. If this fails, abort. Else set the filter_id to
	 * 'upload in progress'.
	 */
	bool handleUploadFilterCommand(uint8_t filterId, uint16_t chunkStartIndex, uint16_t totalSize, uint8_t* chunk, uint16_t chunkSize);
	bool handleUploadFilterCommand(trackable_parser_cmd_upload_filter_t* cmd_data);

	/**
	 * Removes given filter immediately.
	 * Flags this crownstone as 'filter modification in progress'.
	 */
	void handleRemoveFilterCommand(uint8_t filterId);
	void handleRemoveFilterCommand(trackable_parser_cmd_remove_filter_t* cmd_data);

	/**
	 * Inactive filters are activated.
	 * Crcs are (re)computed.
	 * This crownstones master version and crc are broadcasted over the mesh.
	 * Sets 'filter modification in progress' flag of this crownstone back to off.
	 */
	void handleCommitFilterChangesCommand(uint16_t masterversion, uint16_t mastercrc);
	void handleCommitFilterChangesCommand(trackable_parser_cmd_commit_filter_changes_t* cmd_data);

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
	void handleGetFilterSummariesCommand(trackable_parser_cmd_get_filer_summaries_t* cmd_data);

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
