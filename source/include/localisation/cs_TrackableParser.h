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
#include <structs/cs_TrackableParserStructs.h>

/**
 * Transforms EVT_DEVICE_SCANNED and EVT_ADV_BACKGROUND_PARSED
 * into EVT_TRACKING_UPDATE events.
 *
 * Responsible for throttling input to the localisation module using filter parsers.
 */
class TrackableParser : public EventListener {
public:
	/**
	 * TODO(Arend): load filters from memory.
	 * - after loading, set filterModificationInProgress to false.
	 */
	void init();
	void handleEvent(event_t& evt);

	constexpr static size_t MAX_FILTER_IDS     = 8;
	constexpr static size_t FILTER_BUFFER_SIZE = 512;

private:
	// -------------------------------------------------------------
	// ------------------ Advertisment processing ------------------
	// -------------------------------------------------------------

	void handleScannedDevice(scanned_device_t* device);

	/**
	 * Dispatches a TrackedEvent for the given advertisement.
	 */
	void handleBackgroundParsed(adv_background_parsed_t* trackableAdvertisement);

	// -------------------------------------------------------------
	// --------------------- Filter buffer data --------------------
	// -------------------------------------------------------------

	/**
	 * The raw byte buffer in which TrackingFilters are allocated.
	 * Memory is managed as a stack:
	 * - the top is administrated by _filterBufferEndIndex
	 * - new filters are allocated at the top (if space allows)
	 * - when a filter is removed, all filters on top of it are relocated
	 *   immediately to avoid gaps.
	 */
	uint8_t _filterBuffer[FILTER_BUFFER_SIZE];
	size_t _filterBufferEndIndex = 0;

	/**
	 * List of pointers to the currently allocated filters in the _filterBuffer.
	 * The memory is managed in same fashiona as the _filterBuffer itself.
	 */
	tracking_filter_t* _parsingFilters[MAX_FILTER_IDS];
	uint8_t _parsingFiltersEndIndex = 0;

	uint16_t _masterHash;
	uint16_t _masterVersion;  // Lollipop @Persisted

	/**
	 * When this value is true:
	 * - no incoming advertisements are parsed.
	 * - filters may be in inconsistent state.
	 *
	 * Defaults to true, so that the system has time to load data from flash.
	 */
	bool filterModificationInProgress = true;

	// -------------------------------------------------------------
	// ------------------- Filter data management ------------------
	// -------------------------------------------------------------

	/**
	 * Returns a pointer in the _filterBuffer, promising that at least
	 * totalsSize bytes space starting from that address are free to use.
	 *
	 * Initializes the .runtimedata of the newly allocated tracking_filter_t
	 * with the given size, filterid and crc == 0.
	 *
	 * Returns nullptr on failure.
	 *
	 * Internally adjusts _filterBufferEnd to point to one byte after this array.
	 */
	tracking_filter_t* allocateParsingFilter(uint8_t filterId, size_t totalSize);

	/**
	 * Readjust the filterbuffer to create space at the back. Adjust the
	 * _parsingFilter array too.
	 */
	void deallocateParsingFilter(uint8_t filterId);

	/**
	 * Looks up given filter id in the list of filters. Returns nullptr if not found.
	 *
	 * Assumes there are no 'gaps' in the _parsingFilter array. I.e. that
	 * if _parsingFilters[i] == nullptr then _parsingFilters[j] == nullptr
	 * for all j>=i.
	 */
	tracking_filter_t* findParsingFilter(uint8_t filterId);

	// -------------------------------------------------------------
	// ---------------------- Command interface --------------------
	// -------------------------------------------------------------

	/**
	 * Upon first reception of this command with the given filterId,
	 * allocate space in the buffer. If this fails, abort. Else set the filterId to
	 * 'upload in progress'.
	 */
	cs_ret_code_t handleUploadFilterCommand(trackable_parser_cmd_upload_filter_t* cmdData);

	/**
	 * Removes given filter immediately.
	 * Flags this crownstone as 'filter modification in progress'.
	 */
	cs_ret_code_t handleRemoveFilterCommand(trackable_parser_cmd_remove_filter_t* cmdData);

	/**
	 * Inactive filters are activated.
	 * Crcs are (re)computed.
	 * This crownstones master version and crc are broadcasted over the mesh.
	 * Sets 'filter modification in progress' flag of this crownstone back to off.
	 */
	cs_ret_code_t handleCommitFilterChangesCommand(
			trackable_parser_cmd_commit_filter_changes_t* cmdData);

	/**
	 * Returns:
	 *  - master version
	 *  - master crc
	 *  - for each filter:
	 *    - filter id
	 *    - filter version
	 *    - filter crc
	 */
	cs_ret_code_t handleGetFilterSummariesCommand(
			trackable_parser_cmd_get_filer_summaries_t* cmdData);

	// -------------------------------------------------------------
	// ----------------------- OLD interface -----------------------
	// -------------------------------------------------------------

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

	void logServiceData(scanned_device_t* scannedDevice);
};
