/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <common/cs_Component.h>
#include <events/cs_EventListener.h>

#include <localisation/cs_AssetFilterPacketAccessors.h>

#include <optional>
#include <protocol/cs_AssetFilterPackets.h>
#include <structs/cs_AssetFilterStructs.h>

/**
 * Keeps up the asset filters.
 *
 * - Stores filters in flash, and reads them on init.
 * - Allocates RAM for the filters.
 * - Handles commands that modify the filters.
 * - Keeps up the master version and CRC.
 * - Keeps up "modification in progress".
 */
class AssetFilterStore : public EventListener, public Component {
public:
	/**
	 * set _filterModificationInProgress to false.
	 */
	cs_ret_code_t init();

	/**
	 * Returns true if this object is ready to be used.
	 * I.e. the store is not in progress of updates and
	 * master version is non-zero.
	 */
	bool isReady();

	/**
	 * Whether changes are in progress.
	 *
	 * Meaning someone is actively modifying the filters, and a commit is expected to be done soon.
	 */
	bool isInProgress();

	/**
	 * Get the number of filters.
	 */
	uint8_t getFilterCount();

	/**
	 * Get the Nth filter.
	 */
	AssetFilter getFilter(uint8_t index);

	/**
	 * Returns the index of the filter with given filterId, if any.
	 */
	std::optional<uint8_t> findFilterIndex(uint8_t filterId);

	/**
	 * Get the current master version.
	 */
	uint16_t getMasterVersion();

	/**
	 * Get the current master CRC.
	 */
	uint32_t getMasterCrc();

	/**
	 * Max number of filters.
	 */
	constexpr static uint8_t MAX_FILTER_IDS = 8;

	/**
	 * Max total size that the filters take up in RAM.
	 *
	 * Set to: TypeSize(CS_TYPE::STATE_ASSET_FILTER_512) + sizeof(asset_filter_runtime_data_t)
	 */
	constexpr static size_t FILTER_BUFFER_SIZE = 520;

	/**
	 * Time after last edit command (upload, remove), until "modification in progress" times out.
	 */
	constexpr static int MODIFICATION_IN_PROGRESS_TIMEOUT_SECONDS = 20;

private:
	/**
	 * List of pointers to the allocated buffers for the filters.
	 * The filters in this array are always sorted by filterId.
	 * Null pointers will always be at the back of the array, so you can stop iterating when you encounter a null pointer.
	 *
	 * To access a filter, construct an accessor object of type TrackingFilter for the given buffer.
	 */
	uint8_t* _filters[MAX_FILTER_IDS] = {};

	/**
	 * Number of allocated filters in the filters array.
	 */
	uint8_t _filtersCount = 0;

	/**
	 * Keeps track of the version of the filters.
	 * When this value is 0, the filters are invalid.
	 */
	uint16_t _masterVersion = 0;

	/**
	 * CRC over all the filter IDs and CRCs.
	 *
	 * This is updated by the commit command if it matches.
	 */
	uint32_t _masterCrc;

	/**
	 * When this value is not 0, the filters are being modified.
	 *
	 * Reduced by 1 every tick.
	 */
	uint16_t _modificationInProgressCountdown = 0;

	/**
	 * Allocates RAM for a filter of given size, and adds it to the filters array.
	 * - Does NOT check if filterId is already in the list.
	 * - Adds size of runtime data.
	 * - Checks max filters (MAX_FILTER_IDS).
	 * - Checks max ram size (FILTER_BUFFER_SIZE).
	 *
	 * @param[in] filterId        ID of the filter.
	 * @param[in] stateDataSize   Size of the filter data used in State, result of getStateSize(filter data size).
	 *
	 * Returns nullptr on failure, pointer to the buffer on success.
	 */
	uint8_t* allocateFilter(uint8_t filterId, size_t stateDataSize);

	/**
	 * Same as deallocateFilterByIndex, but looks up the filter by the filterId.
	 *
	 * Returns true when id is found and filter is deallocated, false else.
	 */
	bool deallocateFilter(uint8_t filterId);

	/**
	 * Deallocates the filter at given index in the filters array.
	 *
	 * If a gap is created in the array, this method moves all filters
	 * with an index above the given one down one index to close this gap.
	 */
	void deallocateFilterByIndex(uint8_t parsingFilterIndex);

	/**
	 * Get the State type, given the filter data size.
	 *
	 * Returns CONFIG_DO_NOT_USE for invalid size.
	 */
	CS_TYPE getStateType(uint16_t filterDataSize);

	/**
	 * Get the size required for State, given the filter data size.
	 */
	uint16_t getStateSize(uint16_t filterDataSize);

	/**
	 * Used to loop over all asset filter state types.
	 *
	 * Returns CONFIG_DO_NOT_USE for invalid index.
	 */
	CS_TYPE getNthStateType(uint8_t index);

	/**
	 * Returns a pointer to the filter with given filterId, or nullptr if not found.
	 */
	uint8_t* findFilter(uint8_t filterId);

	/**
	 * Returns the total amount of heap allocated for the filters.
	 */
	size_t getTotalHeapAllocatedSize();

	// -------------------------------------------------------------
	// ---------------------- Command interface --------------------
	// -------------------------------------------------------------

	/**
	 * Handle an upload command.
	 *
	 * Allocates filter if not already done.
	 * Removes existing filter if it was committed.
	 * TOOD: remove existing filter if total size is different?
	 *
	 * @return ERR_PROTOCOL_UNSUPPORTED   For an invalid protocol version.
	 * @return ERR_INVALID_MESSAGE        When the data would go outside the total size.
	 * @return ERR_WRONG_STATE            When the existing, non-commited, filter is of different size.
	 * @return ERR_NO_SPACE               When there is no space for this filter.
	 * @return ERR_SUCCESS                On success.
	 */
	cs_ret_code_t handleUploadFilterCommand(const asset_filter_cmd_upload_filter_t& cmdData);

	/**
	 * Removes given filter immediately.
	 * Flags this crownstone as 'filter modification in progress'.
	 *
	 * @return ERR_PROTOCOL_UNSUPPORTED   For an invalid protocol version.
	 * @return ERR_SUCCESS                When the filter is removed.
	 * @return ERR_SUCCESS_NO_CHANGE      When the filter was already removed.
	 */
	cs_ret_code_t handleRemoveFilterCommand(const asset_filter_cmd_remove_filter_t& cmdData);

	/**
	 * Commit the filters.
	 * - Computes CRCs and checks it against the given CRC.
	 * - Checks the filter data validity.
	 * - Unsets "modification in progress" when all checks are passed.
	 * - Sets master version when all checks are passed.
	 *
	 * @return ERR_PROTOCOL_UNSUPPORTED   For an invalid protocol version.
	 * @return ERR_WRONG_STATE            When a filter is invalid.
	 * @return ERR_MISMATCH               When the computed master CRC is different from given master CRC.
	 * @return ERR_SUCCESS                On success.
	 */
	cs_ret_code_t handleCommitFilterChangesCommand(const asset_filter_cmd_commit_filter_changes_t& cmdData);

	/**
	 * Writes the filter summary in the result.
	 */
	void handleGetFilterSummariesCommand(cs_result_t& result);

	void onTick();

	// -------------------------------------------------------------
	// ---------------------- Utility functions --------------------
	// -------------------------------------------------------------

	/**
	 * To be called when about to modify filters.
	 * - Sets master version to 0.
	 */
	void startInProgress();

	/**
	 * To be called when filters are no longer being modified.
	 */
	void endInProgress(uint16_t newMasterVersion, uint32_t newMasterCrc);

	/**
	 * Called when the modification in progress is timed out.
	 */
	void inProgressTimeout();

	/**
	 * Send an internal event when isInProgress() may have changed.
	 */
	void sendInProgressStatus();

	/**
	 * Commit current filters.
	 * - Checks the filter data validity.
	 * - Computes CRCs and checks it against the given CRC.
	 *
	 * When all checks pass:
	 * - Stores the filters, if store == true.
	 * - Marks filters as committed.
	 * - Unsets "modification in progress".
	 * - Sets master version.
	 */
	cs_ret_code_t commit(uint16_t masterVersion, uint32_t masterCrc, bool store);

	/**
	 * Calculate the master CRC and return it (not stored).
	 *
	 * Assumes filter CRCs are already calculated.
	 */
	uint32_t computeMasterCrc();

	/**
	 * Checks for all filters if the allocated filter data size is equal to the computed size based on its contents.
	 *
	 * - Skips filters that have already been committed.
	 * - Deallocates any filters failing the check.
	 *
	 * @return true          When all filters passed the check.
	 */
	bool validateFilters();

	/**
	 * Computes the CRC of the filters, and sets it in the filter.
	 *
	 * - Skips filters of which the CRC have been calculated before (flags.crcCalculated == true).
	 * - Sets flags.crcCalculated to true.
	 */
	void computeFilterCrcs();

	/**
	 * Store filters to flash.
	 *
	 * - Skips filters that have already been committed.
	 */
	void storeFilters();

	/**
	 * Load filters and version from flash.
	 */
	void loadFromFlash();

	/**
	 * Load filters of a specific type (size) from flash.
	 */
	void loadFromFlash(CS_TYPE type);

	/**
	 * Sets flags.committed to true.
	 */
	void markFiltersCommitted();

public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};
