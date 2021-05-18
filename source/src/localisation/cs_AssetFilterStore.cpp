/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <localisation/cs_AssetFilterPacketAccessors.h>
#include <localisation/cs_AssetFilterStore.h>
#include <localisation/cs_Nearestnearestwitnessreport.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_ErrorCodes.h>
#include <structs/cs_PacketsInternal.h>
#include <util/cs_Crc32.h>
#include <util/cs_CuckooFilter.h>

#define LOGAssetFilterWarn LOGw
#define LOGAssetFilterInfo LOGi
#define LOGAssetFilterDebug LOGd

cs_ret_code_t AssetFilterStore::init() {
	// TODO: Load from flash.
	LOGAssetFilterInfo("init");

//	endInProgress(_masterVersion, _masterCrc);
	listen();
	return ERR_SUCCESS;
}

uint8_t AssetFilterStore::getFilterCount() {
	return _filtersCount;
}

AssetFilter AssetFilterStore::getFilter(uint8_t index) {
	return AssetFilter(_filters[index]);
}

uint16_t AssetFilterStore::getMasterVersion() {
	return _masterVersion;
}

uint32_t AssetFilterStore::getMasterCrc() {
	return _masterCrc;
}

void AssetFilterStore::handleEvent(event_t& evt) {
	switch (evt.type) {
		case CS_TYPE::CMD_UPLOAD_FILTER: {
			auto commandPacket = CS_TYPE_CAST(CMD_UPLOAD_FILTER, evt.data);
			evt.result.returnCode = handleUploadFilterCommand(*commandPacket);
			break;
		}
		case CS_TYPE::CMD_REMOVE_FILTER: {
			auto commandPacket = CS_TYPE_CAST(CMD_REMOVE_FILTER, evt.data);
			evt.result.returnCode = handleRemoveFilterCommand(*commandPacket);
			break;
		}
		case CS_TYPE::CMD_COMMIT_FILTER_CHANGES: {
			auto commandPacket = CS_TYPE_CAST(CMD_COMMIT_FILTER_CHANGES, evt.data);
			evt.result.returnCode = handleCommitFilterChangesCommand(*commandPacket);
			break;
		}
		case CS_TYPE::CMD_GET_FILTER_SUMMARIES: {
			handleGetFilterSummariesCommand(evt.result);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			onTick();
			break;
		}
		default: break;
	}
}

uint8_t* AssetFilterStore::allocateFilter(uint8_t filterId, size_t payloadSize) {
	LOGAssetFilterDebug("Allocating filter #%u, of size %u (endindex: %u)", filterId, payloadSize, _filtersCount);
	size_t totalSize = sizeof(asset_filter_runtime_data_t) + payloadSize;

	if (_filtersCount >= MAX_FILTER_IDS) {
		// not enough space for more filter ids.
		return nullptr;
	}

	if (getTotalHeapAllocatedSize() + totalSize > FILTER_BUFFER_SIZE) {
		// not enough space for filter of this total size.
		return nullptr;
	}

	// try heap allocation
	uint8_t* newFilterBuffer = new (std::nothrow) uint8_t[totalSize];

	if (newFilterBuffer == nullptr) {
		LOGAssetFilterWarn("Filter couldn't be allocated, heap is too full");
		return nullptr;
	}

	// create space for the new entry and keep the list sorted
	uint8_t indexForNewEntry = 0;

	for (uint8_t i = _filtersCount; i > 0; --i) {
		// _parsingFiltersCount points exactly one after the last non-nullptr
		// i is always a valid position: because of previous check it is strictly less than MAX_FILTER_IDS
		// (i-1) won't roll over because i > 0.

		if (AssetFilter(_filters[i - 1]).runtimedata()->filterId > filterId) {
			// move this entry up one position in the list.
			_filters[i] = _filters[i - 1];
		}
		else {
			indexForNewEntry = i;
			break;
		}
	}

	_filters[indexForNewEntry] = newFilterBuffer;

	AssetFilter newFilterAccessor(newFilterBuffer);

	LOGAssetFilterDebug("before assignments");
	// initialize runtime data.
	newFilterAccessor.runtimedata()->filterId    = filterId;
	newFilterAccessor.runtimedata()->totalSize   = payloadSize;
	newFilterAccessor.runtimedata()->flags.asInt = 0;

	// don't forget to increment the Count for the filter list.
	_filtersCount++;

	return newFilterBuffer;
}

void AssetFilterStore::deallocateFilterByIndex(uint8_t filterIndex) {
	if (filterIndex >= MAX_FILTER_IDS) {
		return;
	}

	// (delete can handle nullptr according to c++ standard)
	delete[] _filters[filterIndex];

	LOGAssetFilterDebug("post-delete.");

	// shift entries after the deleted one an index downward (overwriting the deleted pointer).
	for (size_t i = filterIndex + 1; i < _filtersCount; i++) {
		_filters[i - 1] = _filters[i];
	}
	// and remove the trailing pointer.
	_filters[_filtersCount - 1] = nullptr;

	// removed an entry, so reduce end by 1.
	_filtersCount--;
}

bool AssetFilterStore::deallocateFilter(uint8_t filterId) {
	LOGAssetFilterDebug("deallocating %u", filterId);

	auto filterIndexOpt = findFilterIndex(filterId);
	if (filterIndexOpt) {
		deallocateFilterByIndex(filterIndexOpt.value());
		return true;
	}

	LOGAssetFilterDebug("tried to deallocate but filter id wasn't found.");
	return false;
}

uint8_t* AssetFilterStore::findFilter(uint8_t filterId) {
	auto filterIndexOpt = findFilterIndex(filterId);

	return filterIndexOpt ? _filters[filterIndexOpt.value()] : nullptr;
}

std::optional<uint8_t> AssetFilterStore::findFilterIndex(uint8_t filterId) {
	LOGAssetFilterDebug("Looking up filter %u, end index: %u", filterId, _filtersCount);

	uint8_t* trackingFilter;
	for (uint8_t index = 0; index < _filtersCount; ++index) {
		trackingFilter = _filters[index];

		if (trackingFilter == nullptr) {
			LOGw("_parsingFiltersCount incorrect: found nullptr before reaching end of filter list.");
			return {};
		}

		if (AssetFilter(trackingFilter).runtimedata()->filterId == filterId) {
			LOGAssetFilterDebug("Filter found at index %u", index);
			return index;
		}
	}

	return {};
}

size_t AssetFilterStore::getTotalHeapAllocatedSize() {
	LOGAssetFilterDebug("computing allocated size");

	size_t total = 0;
	for (uint8_t* filter : _filters) {
		if (filter == nullptr) {
			// reached back of the list (nullptr).
			break;
		}
		total += AssetFilter(filter).length();
	}

	LOGAssetFilterDebug("computing allocated size finished");
	return total;
}

// -------------------------------------------------------------
// ---------------------- Command interface --------------------
// -------------------------------------------------------------

cs_ret_code_t AssetFilterStore::handleUploadFilterCommand(const asset_filter_cmd_upload_filter_t& cmdData) {
	LOGAssetFilterDebug("handleUploadFilterCommand chunkStartIndex=%u, chunkSize=%u, totalSize=%u",
			cmdData.chunkStartIndex,
			cmdData.chunkSize,
			cmdData.totalSize);

	if (cmdData.protocolVersion != ASSET_FILTER_CMD_PROTOCOL_VERSION) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	if (cmdData.chunkStartIndex + cmdData.chunkSize > cmdData.totalSize) {
		LOGAssetFilterWarn("Chunk overflows total size.");
		return ERR_INVALID_MESSAGE;
	}

	startInProgress();

	// Find or allocate a filter.
	AssetFilter filter(findFilter(cmdData.filterId));

	// Check if we need to remove an old filter.
	// Note that we can't just remove if there's data, because it might be the previous chunk.
	if (filter._data != nullptr && filter.runtimedata()->flags.flags.committed == true) {
		LOGAssetFilterDebug("removing pre-existing filter with same id (%u)", cmdData.filterId);
		deallocateFilter(filter.runtimedata()->filterId);

		// after deallocation, the filter is clearly gone.
		filter._data = nullptr;
	}

	// check if the total size hasn't changed in the mean time
	if (filter._data != nullptr && filter.runtimedata()->totalSize != cmdData.totalSize) {
		// If total size to allocate changes something is really off on the host side
		LOGe("Corrupt chunk of upload data received. Deallocating partially constructed filter.");
		deallocateFilter(filter.runtimedata()->filterId);
		return ERR_WRONG_STATE;
	}

	// check if we need to create a new filter
	if (filter._data == nullptr) {
		LOGAssetFilterDebug("Filter %u not found", cmdData.filterId);
		// command totalSize only includes the size of the cuckoo filter and its metadata, not the runtime data yet.
		filter = allocateFilter(cmdData.filterId, cmdData.totalSize);

		if (filter._data == nullptr) {
			// failed to handle command, no space.
			LOGAssetFilterWarn("Filter allocation failed");
			return ERR_NO_SPACE;
		}
	}

	// by now, the filter exists, is clean, and the incoming chunk is verified for totalSize consistency.

	// chunk index starts counting from metadata onwards (ignoring runtimedata)
	uint8_t* filterBasePtr = filter.filterdata()._data;
	uint8_t* filterChunkPtr = filterBasePtr + cmdData.chunkStartIndex;

	// apply filter chunk, counting chunk index from metadata onwards:
	std::memcpy(filterChunkPtr, cmdData.chunk, cmdData.chunkSize);

	return ERR_SUCCESS;
}

cs_ret_code_t AssetFilterStore::handleRemoveFilterCommand(const asset_filter_cmd_remove_filter_t& cmdData) {
	LOGAssetFilterDebug("handleRemoveFilterCommand id=%u", cmdData.filterId);

	if (cmdData.protocolVersion != ASSET_FILTER_CMD_PROTOCOL_VERSION) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	if (deallocateFilter(cmdData.filterId)) {
		startInProgress();
		return ERR_SUCCESS;
	}

	return ERR_SUCCESS_NO_CHANGE;
}

cs_ret_code_t AssetFilterStore::handleCommitFilterChangesCommand(const asset_filter_cmd_commit_filter_changes_t& cmdData) {
	LOGAssetFilterDebug("handleCommitFilterChangesCommand version=%u crc=%u", cmdData.masterVersion, cmdData.masterCrc);

	if (cmdData.protocolVersion != ASSET_FILTER_CMD_PROTOCOL_VERSION) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}

	if (checkFilterSizeConsistency()) {
		return ERR_WRONG_STATE;
	}

	computeFilterCrcs();

	uint32_t masterHash = computeMasterCrc();
	if (cmdData.masterCrc != masterHash) {
		LOGAssetFilterWarn("Master Crc did not match: 0x%x != 0x%x, inProgress stays true.", cmdData.masterCrc, masterHash);
		return ERR_MISMATCH;
	}

	// finish commit by writing the _masterHash

	endInProgress(cmdData.masterVersion, cmdData.masterCrc);
	LOGAssetFilterDebug("Accepted commit command, ended filterModificationInProgress");

	event_t event(CS_TYPE::EVT_FILTERS_UPDATED);
	event.dispatch();

	return ERR_SUCCESS;
}

void AssetFilterStore::handleGetFilterSummariesCommand(cs_result_t& result) {
	LOGAssetFilterDebug("handle get filter summaries:");
	for (auto& filter : _filters) {
		if (filter == nullptr) {
			LOGAssetFilterDebug("filter: nullptr");
			// continue instead of break for logging purpose:
			// expecting all subsequent iterations of this loop to get nullptrs
			continue;
		}
		LOGAssetFilterDebug("filter: %u", AssetFilter(filter).runtimedata()->filterId);
	}

	// stack allocate a buffer summaries object fitting at most max summaries:
	auto requiredBuffSize = sizeof(asset_filter_cmd_get_filter_summaries_ret_t)
							+ sizeof(asset_filter_summary_t) * _filtersCount;

	if (result.buf.len < requiredBuffSize) {
		result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}

	// placement new constructs the object in the buff and now has enough space for the summaries.
	auto retvalptr  = new (result.buf.data) asset_filter_cmd_get_filter_summaries_ret_t;
	result.dataSize = requiredBuffSize;

	retvalptr->protocolVersion = ASSET_FILTER_CMD_PROTOCOL_VERSION;
	retvalptr->masterVersion   = _masterVersion;
	retvalptr->masterCrc       = _masterCrc;
	retvalptr->freeSpace       = FILTER_BUFFER_SIZE - getTotalHeapAllocatedSize();

	for (size_t i = 0; i < _filtersCount; i++) {
		AssetFilter filter(_filters[i]);
		retvalptr->summaries[i].id   = filter.runtimedata()->filterId;
		retvalptr->summaries[i].crc  = filter.runtimedata()->crc;
	}

	result.returnCode = ERR_SUCCESS;
}

void AssetFilterStore::onTick() {
	if (_modificationInProgressCountdown) {
		_modificationInProgressCountdown--;
		if (_modificationInProgressCountdown == 0) {
			LOGAssetFilterInfo("Modification in progress timeout.");
			sendInProgressStatus();
		}
	}
}

// -------------------------------------------------------------
// ---------------------- Utility functions --------------------
// -------------------------------------------------------------

bool AssetFilterStore::isReady() {
	// _masterVersion must be non-zero. Before that the filters
	assert (_masterVersion == 0 && !isInProgress(),"Master version should be set to 0 when in progress.");
	return _masterVersion != 0;
}

void AssetFilterStore::startInProgress() {
	LOGAssetFilterDebug("startInProgress");
	_modificationInProgressCountdown = 1000 * MODIFICATION_IN_PROGRESS_TIMEOUT_SECONDS / TICK_INTERVAL_MS;
	_masterVersion                   = 0;
	sendInProgressStatus();
}

void AssetFilterStore::endInProgress(uint16_t newMasterVersion, uint32_t newMasterCrc) {
	LOGAssetFilterDebug("endInProgress version=%u crc=%u", newMasterVersion, newMasterCrc);
	_masterVersion                   = newMasterVersion;
	_masterCrc                       = newMasterCrc;
	_modificationInProgressCountdown = 0;
	sendInProgressStatus();
}

bool AssetFilterStore::isInProgress() {
	return _modificationInProgressCountdown != 0;
}

void AssetFilterStore::sendInProgressStatus() {
	TYPIFY(EVT_FILTER_MODIFICATION) modification = isInProgress();
	event_t event(CS_TYPE::EVT_FILTER_MODIFICATION, &modification, sizeof(modification));
	event.dispatch();
}

uint32_t AssetFilterStore::computeMasterCrc() {
	uint32_t masterCrc = crc32(nullptr, 0);

	for (auto filterBuffer : _filters) {
		if (filterBuffer == nullptr) {
			break;
		}

		AssetFilter filter(filterBuffer);

		// apply filterId onto master crc
		masterCrc = crc32(
				reinterpret_cast<const uint8_t*>(&filter.runtimedata()->filterId),
				sizeof(filter.runtimedata()->filterId),
				&masterCrc);

		// apply fitlerCrc onto master crc
		masterCrc = crc32(
				reinterpret_cast<const uint8_t*>(&filter.runtimedata()->crc),
				sizeof(filter.runtimedata()->crc),
				&masterCrc);
	}

	LOGAssetFilterDebug("master crc: 0x%x", masterCrc);

	return masterCrc;
}

bool AssetFilterStore::checkFilterSizeConsistency() {
	bool consistencyChecksFailed = false;

	for (size_t index = 0; index < _filtersCount; /* intentionally no index++ here */) {
		auto filter = AssetFilter(_filters[index]);

		if (filter._data == nullptr) {
			// early return: last filter has been handled.
			break;
		}

		if (filter.runtimedata()->flags.flags.committed == false) {
			filter.runtimedata()->flags.flags.committed = true;
			// filter changed since commit.

			size_t filterAllocatedSize = filter.runtimedata()->totalSize + sizeof(asset_filter_runtime_data_t);
			size_t filterSize          = filter.length();

			if (filterSize != filterAllocatedSize) {
				// the cuckoo filter size parameters do not match the allocated space.
				// likely cause by malformed data on the host side.
				// This check can't be performed earlier: as long as not all chuncks are
				// received the filterdata may be in invalid state.

				LOGAssetFilterWarn("Deallocating filter ID=%u because of invalid filter size: allocated=%u calculated=%u",
						filter.runtimedata()->filterId,
						filterAllocatedSize,
						filterSize);

				deallocateFilterByIndex(index);
				consistencyChecksFailed = true;

				// intentionally skipping index++, deallocate shrinks the array we're looping over.
				continue;
			}
		}

		index++;
	}

	return consistencyChecksFailed;
}

void AssetFilterStore::computeFilterCrcs() {
	for (uint8_t* filterBuffer : _filters) {
		if (filterBuffer == nullptr) {
			break;
		}

		AssetFilter filter(filterBuffer);
		if (filter.runtimedata()->flags.flags.crcCalculated == false) {
			filter.runtimedata()->crc = crc32(filter.filterdata().metadata()._data, filter.runtimedata()->totalSize, nullptr);
			filter.runtimedata()->flags.flags.crcCalculated = true;
			LOGAssetFilterDebug("Calculate CRC for filter filterId=%u, CRC=0x%x", filter.runtimedata()->filterId, filter.runtimedata()->crc);
		}
		else {
			LOGAssetFilterDebug("Skip filter CRC calculation for filterId=%u, CRC=0x%x", filter.runtimedata()->filterId, filter.runtimedata()->crc)
		}
	}
}
