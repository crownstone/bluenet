/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>

#include <localisation/cs_AssetFilterPacketAccessors.h>

#include <optional>
#include <protocol/cs_AssetFilterPackets.h>
#include <structs/cs_AssetFilterStructs.h>

/**
 * Transforms EVT_DEVICE_SCANNED and EVT_ADV_BACKGROUND_PARSED
 * into EVT_TRACKING_UPDATE events.
 *
 * Responsible for throttling input to the localisation module using filter parsers.
 */
class AssetFilterStore : public EventListener {
public:
	/**
	 * set _filterModificationInProgress to false.
	 */
	cs_ret_code_t init();

	/**
	 * Whether changes are in progress.
	 */
	bool isInProgress();

	uint8_t getFilterCount();
	AssetFilter getFilter(uint8_t index);

	/**
	 * Returns the index of the parsing filter with given filterId,
	 * or an empty optional if that wasn't available.
	 */
	std::optional<uint8_t> findFilterIndex(uint8_t filterId);

	uint16_t getMasterVersion();
	uint16_t getMasterCrc();

	constexpr static uint8_t MAX_FILTER_IDS    = 8;
	constexpr static size_t FILTER_BUFFER_SIZE = 512;

private:
	// -------------------------------------------------------------
	// --------------------- Filter buffer data --------------------
	// -------------------------------------------------------------

	/**
	 * List of pointers to the currently allocated buffers for the filters.
	 * The filters in this array are always sorted by filterId,
	 * and nullptrs only occur at the back in the sense that:
	 *   for any i <= j < MAX_FILTER_IDS:
	 *   if   _parsingFilters[i] == nullptr
	 *   then _parsingFilters[j] == nullptr
	 *
	 *  To access a filter, construct an accessor object of type TrackingFilter
	 *  for the given buffer.
	 */
	uint8_t* _filters[MAX_FILTER_IDS] = {};

	/**
	 * Number of allocated filters in the parsingFilters array.
	 * Shall suffice: 0 <=  _parsingFiltersCount < MAX_FILTER_IDS
	 */
	uint8_t _filtersCount = 0;

	/**
	 * Hash of all the hashes of the allocated filters, sorted by filterId.
	 * This is updated by the commit command if it matches.
	 */
	uint16_t _masterHash;

	/**
	 * Keeps track of the version of the filters -- not in use yet.
	 */
	uint16_t _masterVersion = 0;

	/**
	 * When this value is true:
	 * - no incoming advertisements are parsed.
	 * - filters may be in inconsistent state.
	 *
	 * Defaults to true, so that the system has time to load data from flash.
	 */
	bool _filterModificationInProgress = true;

	// -------------------------------------------------------------
	// ------------------ Advertisment processing ------------------
	// -------------------------------------------------------------

//	/**
//	 * Dispatches a TrackedEvent for the given advertisement.
//	 */
//	void handleScannedDevice(scanned_device_t* device);

//	/**
//	 * Not in use yet
//	 */
//	void handleBackgroundParsed(adv_background_parsed_t* trackableAdvertisement);

	// -------------------------------------------------------------
	// ------------------- Filter data management ------------------
	// -------------------------------------------------------------

	/**
	 * Heap allocates a tracking_filter_t object, if there is enough free
	 * space on the heap and if the total allocated size of this TrackableParser
	 * component used for filters will not exceed the maximum as result of this
	 * allocation.
	 *
	 * Initializes the .runtimedata of the newly allocated tracking_filter_t
	 * with the given size, filterid and crc == 0.
	 *
	 * Returns nullptr on failure, pointer to the buffer on success.
	 * On success the buffer pointer is also appended to the _parsingFilters.
	 */
	uint8_t* allocateFilter(uint8_t filterId, size_t payloadSize);

	/**
	 * Same as deallocateParsingFilterByIndex, but looks up the filter by the filterId.
	 *
	 * Returns true when id is found and filter is deallocated, false else.
	 */
	bool deallocateFilter(uint8_t filterId);

	/**
	 * Deallocates the filter at given index in the _parsingFilters array.
	 *
	 * If a gap is created in the array, this method moves all filters
	 * with an index above the given one down one index to close this gap.
	 */
	void deallocateFilterByIndex(uint8_t parsingFilterIndex);

	/**
	 * Looks up given filter id in the list of filters. Returns nullptr if not found.
	 *
	 * Assumes there are no 'gaps' in the _parsingFilter array. I.e. that
	 * if _parsingFilters[i] == nullptr then _parsingFilters[j] == nullptr
	 * for all j>=i.
	 */
	uint8_t* findFilter(uint8_t filterId);

	/**
	 * Returns the sums of all getTotalSize of the non-nullptrs in the _parsingFilters array.
	 */
	size_t getTotalHeapAllocatedSize();

	/**
	 * Computes and returns the totalSize allocated for the given filter.
	 * Returns 0 for nullptr.
	 */
	size_t getTotalSize(uint8_t* trackingFilter);

	// -------------------------------------------------------------
	// ---------------------- Command interface --------------------
	// -------------------------------------------------------------

	/**
	 * Upon first reception of this command with the given filterId,
	 * allocate space in the buffer. If this fails, abort. Else set the filterId to
	 * 'upload in progress'.
	 */
	cs_ret_code_t handleUploadFilterCommand(const asset_filter_cmd_upload_filter_t& cmdData);

	/**
	 * Removes given filter immediately.
	 * Flags this crownstone as 'filter modification in progress'.
	 * Returns ERR_SUCCESS_NO_CHANGE if filter not found, else ERR_SUCCESS.
	 */
	cs_ret_code_t handleRemoveFilterCommand(const asset_filter_cmd_remove_filter_t& cmdData);

	/**
	 * Inactive filters are activated.
	 * Crcs are (re)computed.
	 * This crownstones master version and crc are broadcasted over the mesh.
	 * Sets 'filter modification in progress' flag of this crownstone back to off.
	 */
	cs_ret_code_t handleCommitFilterChangesCommand(const asset_filter_cmd_commit_filter_changes_t& cmdData);

	/**
	 * Writes summaries of the filters into the result as a
	 * trackable_parser_cmd_get_filter_summaries_ret_t.
	 *
	 * returnCode is ERR_BUFFER_TOO_SMALL if result data doesn't fit the result buffer.
	 * Else, returnCode is ERR_SUCCESS.
	 */
	void handleGetFilterSummariesCommand(cs_result_t& result);

	// -------------------------------------------------------------
	// ---------------------- Utility functions --------------------
	// -------------------------------------------------------------

	/**
	 * sets _masterVersion to 0 and _filterModificationInProgress to true.
	 *
	 * This will result in the TrackableParser not handling incoming advertisements
	 * and ensure that it is safe to adjust the filters.
	 */
	void startProgress();
	/**
	 * Sets the _masterVersion and _masterHash, and _filterModificationInProgress to false.
	 */
	void endProgress(uint16_t newMasterHash, uint16_t newMasterVersion);

	/**
	 * The master crc is the crc16 of the filters in the buffer.
	 * This method assumes the filter crcs are up to date.
	 * Returns the mastercrc computed from the filter crcs.
	 * _masterHash is not changed by this method.
	 */
	uint16_t masterCrc();

	/**
	 * Checks if the cuckoo filter buffer size plus the constant overhead
	 * equals the total size allocated for the tracking_filter*s in the
	 * _parsingFilter array.
	 *
	 * Deallocates any filters failing the check.
	 *
	 * Check is only performed on filters that are currently in progress (.crc == 0)
	 */
	bool checkFilterSizeConsistency();

	/**
	 * Computes the crcs of filters that currently have their crc set to 0.
	 */
	void computeCrcs();

public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};
