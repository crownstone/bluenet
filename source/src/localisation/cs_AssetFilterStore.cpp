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
#include <storage/cs_State.h>
#include <structs/cs_PacketsInternal.h>
#include <util/cs_Crc32.h>

#define LOGAssetFilterWarn LOGw
#define LOGAssetFilterInfo LOGi
#define LOGAssetFilterDebug LOGvv

cs_ret_code_t AssetFilterStore::init() {
	LOGAssetFilterInfo("init");

	loadFromFlash();
	LOGAssetFilterInfo("Successfully loaded %u filters", _filtersCount);

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

uint8_t* AssetFilterStore::allocateFilter(uint8_t filterId, size_t stateDataSize) {
	size_t allocatedSize = sizeof(asset_filter_runtime_data_t) + stateDataSize;
	LOGAssetFilterDebug("Allocating filter id=%u stateDataSize=%u allocatedSize=%u", filterId, stateDataSize, allocatedSize);

	if (_filtersCount >= MAX_FILTER_IDS) {
		LOGAssetFilterInfo("Too many filters");
		return nullptr;
	}

	if (getTotalHeapAllocatedSize() + allocatedSize > FILTER_BUFFER_SIZE) {
		LOGAssetFilterInfo("Not enough free space for %u", allocatedSize);
		return nullptr;
	}

	// Try heap allocation.
	uint8_t* newFilterBuffer = new (std::nothrow) uint8_t[allocatedSize];

	// memset to 0 to prevent undefined behaviour when 1st chunk is not uploaded.
	memset(newFilterBuffer, 0, allocatedSize);

	if (newFilterBuffer == nullptr) {
		LOGAssetFilterWarn("Filter couldn't be allocated, heap is too full");
		return nullptr;
	}

	// Add the filter to the list, while keeping the list sorted.
	uint8_t indexForNewEntry = 0;
	for (uint8_t i = _filtersCount; i > 0; --i) {
		// _filtersCount points exactly one after the last non-nullptr
		// i is always a valid position: because of previous check it is strictly less than MAX_FILTER_IDS
		// (i-1) won't roll over because i > 0.

		if (AssetFilter(_filters[i - 1]).runtimedata()->filterId > filterId) {
			// Move this entry up one position in the list.
			_filters[i] = _filters[i - 1];
		}
		else {
			indexForNewEntry = i;
			break;
		}
	}

	_filters[indexForNewEntry] = newFilterBuffer;

	_filtersCount++;

	return newFilterBuffer;
}

void AssetFilterStore::deallocateFilterByIndex(uint8_t filterIndex) {
	LOGAssetFilterDebug("Deallocate index=%u", filterIndex);
	if (filterIndex >= MAX_FILTER_IDS) {
		return;
	}

	// Remove from flash.
	auto filter = getFilter(filterIndex);
	uint16_t size = filter.filterdata().length();
	cs_ret_code_t retCode = State::getInstance().remove(getStateType(size), filter.runtimedata()->filterId);
	if (retCode != ERR_SUCCESS) {
		LOGAssetFilterWarn("Remove from state failed retCode=%u", retCode);
	}

	// (delete can handle nullptr according to c++ standard)
	delete[] _filters[filterIndex];

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
	LOGAssetFilterDebug("Deallocate filterId=%u", filterId);

	auto filterIndexOpt = findFilterIndex(filterId);
	if (filterIndexOpt.has_value() == false) {
		LOGAssetFilterDebug("Filter not found");
		return false;
	}

	deallocateFilterByIndex(filterIndexOpt.value());
	return true;
}

CS_TYPE AssetFilterStore::getStateType(uint16_t filterDataSize) {
	for (uint8_t i = 0; i < 255; ++i) {
		CS_TYPE type = getNthStateType(i);
		if (type == CS_TYPE::CONFIG_DO_NOT_USE) {
			LOGw("Invalid size: %u", filterDataSize);
			return CS_TYPE::CONFIG_DO_NOT_USE;
		}
		if (filterDataSize <= TypeSize(type)) {
			return type;
		}
	}
	// Should no be reached.
	return CS_TYPE::CONFIG_DO_NOT_USE;
}

uint16_t AssetFilterStore::getStateSize(uint16_t filterDataSize) {
	CS_TYPE type = getStateType(filterDataSize);
	if (type == CS_TYPE::CONFIG_DO_NOT_USE) {
		return FILTER_BUFFER_SIZE;
	}
	return TypeSize(type);
}

CS_TYPE AssetFilterStore::getNthStateType(uint8_t index) {
	switch (index) {
		case 0:  return CS_TYPE::STATE_ASSET_FILTER_32;
		case 1:  return CS_TYPE::STATE_ASSET_FILTER_64;
		case 2:  return CS_TYPE::STATE_ASSET_FILTER_128;
		case 3:  return CS_TYPE::STATE_ASSET_FILTER_256;
		case 4:  return CS_TYPE::STATE_ASSET_FILTER_512;
		default: return CS_TYPE::CONFIG_DO_NOT_USE;
	}
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
	size_t total = 0;
	for (uint8_t* filter : _filters) {
		if (filter == nullptr) {
			// reached back of the list (nullptr).
			break;
		}
		total += AssetFilter(filter).length();
	}

	LOGAssetFilterDebug("Total heap allocated = %u", total);
	return total;
}

// -------------------------------------------------------------
// ---------------------- Command interface --------------------
// -------------------------------------------------------------

cs_ret_code_t AssetFilterStore::handleUploadFilterCommand(const asset_filter_cmd_upload_filter_t& cmdData) {
	LOGAssetFilterDebug("handleUploadFilterCommand filterId=%u chunkStartIndex=%u, chunkSize=%u, totalSize=%u",
			cmdData.filterId,
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
		LOGAssetFilterDebug("Remove previous filter");
		deallocateFilter(filter.runtimedata()->filterId);

		// After deallocation, the filter is clearly gone.
		filter._data = nullptr;
	}

	// Check if the total size hasn't changed.
	if (filter._data != nullptr && filter.runtimedata()->filterDataSize != cmdData.totalSize) {
		// If total size to allocate changes something is really off on the host side.
		LOGw("Receive different totalSize than previously uncommitted filter");
		deallocateFilter(filter.runtimedata()->filterId);
		return ERR_WRONG_STATE;
	}

	// Check if we need to allocate a new filter.
	if (filter._data == nullptr) {
		// Command totalSize only includes the size of the filter and its metadata, not the runtime data yet.
		uint8_t* newBuf = allocateFilter(cmdData.filterId, getStateSize(cmdData.totalSize));
		filter = AssetFilter(newBuf);

		if (filter._data == nullptr) {
			LOGAssetFilterWarn("Filter allocation failed");
			return ERR_NO_SPACE;
		}

		// initialize runtime data.
		filter.runtimedata()->filterId       = cmdData.filterId;
		filter.runtimedata()->filterDataSize = cmdData.totalSize;
		filter.runtimedata()->flags.asInt    = 0;
		filter.runtimedata()->crc            = 0; // Not required, but looks better.
	}

	// By now, the filter exists, is clean, and the incoming chunk is verified for filterDataSize consistency.

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

	commit(cmdData.masterVersion, cmdData.masterCrc, true);

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
		LOGAssetFilterDebug("filter: id=%u crc=%x",
				AssetFilter(filter).runtimedata()->filterId,
				AssetFilter(filter).runtimedata()->crc);
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
			inProgressTimeout();
		}
	}
}

// -------------------------------------------------------------
// ---------------------- Utility functions --------------------
// -------------------------------------------------------------

bool AssetFilterStore::isReady() {
	// _masterVersion must be non-zero. Before that the filters
	assert(_masterVersion == 0 || !isInProgress(), "Master version should be set to 0 when in progress.");
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

	TYPIFY(STATE_ASSET_FILTERS_VERSION) stateVal = {
			.masterVersion = _masterVersion,
			.masterCrc     = _masterCrc
	};
	State::getInstance().set(CS_TYPE::STATE_ASSET_FILTERS_VERSION, &stateVal, sizeof(stateVal));

	sendInProgressStatus();
}

bool AssetFilterStore::isInProgress() {
	return _modificationInProgressCountdown != 0;
}

void AssetFilterStore::inProgressTimeout() {
	LOGAssetFilterInfo("Modification in progress timeout.");

	// Clean up filters that have not been committed.
	for (uint8_t index = 0; index < _filtersCount; /* intentionally no index++ here */) {
		auto filter = AssetFilter(_filters[index]);

		if (filter._data == nullptr) {
			// Early return: last filter has been handled.
			break;
		}

		if (filter.runtimedata()->flags.flags.committed == false) {
			deallocateFilterByIndex(index);

			// Intentionally skipping index++, deallocate shrinks the array we're looping over.
			continue;
		}
		index++;
	}

	sendInProgressStatus();
}

void AssetFilterStore::sendInProgressStatus() {
	TYPIFY(EVT_FILTER_MODIFICATION) modification = isInProgress();
	event_t event(CS_TYPE::EVT_FILTER_MODIFICATION, &modification, sizeof(modification));
	event.dispatch();
}


cs_ret_code_t AssetFilterStore::commit(uint16_t masterVersion, uint32_t masterCrc, bool store) {
	LOGAssetFilterInfo("Commit version=%u CRC=%u store=%u", masterVersion, masterCrc, store);
	if (validateFilters()) {
		return ERR_WRONG_STATE;
	}

	computeFilterCrcs();

	uint32_t computedMasterCrc = computeMasterCrc();
	if (masterCrc != computedMasterCrc) {
		LOGAssetFilterWarn("Master CRC does not match: 0x%x != 0x%x", masterCrc, computedMasterCrc);
		return ERR_MISMATCH;
	}

	if (store) {
		storeFilters();
	}

	markFiltersCommitted();

	endInProgress(masterVersion, masterCrc);
	return ERR_SUCCESS;
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

	LOGAssetFilterDebug("Computed master crc: 0x%x", masterCrc);

	return masterCrc;
}

bool AssetFilterStore::validateFilters() {
	LOGAssetFilterDebug("validateFilters");
	bool checksFailed = false;

	for (size_t index = 0; index < _filtersCount; /* intentionally no index++ here */) {
		auto filter = AssetFilter(_filters[index]);

		if (filter._data == nullptr) {
			// Early return: last filter has been handled.
			break;
		}

		if (filter.runtimedata()->flags.flags.committed == false) {
			LOGAssetFilterDebug("Filter %u not yet commited", index);
			// Filter changed since commit.

			if (!filter.filterdata().isValid()) {
				LOGAssetFilterWarn("Deallocating filter ID=%u because it is invalid.", filter.runtimedata()->filterId);
				deallocateFilterByIndex(index);
				checksFailed = true;
				// Intentionally skipping index++, deallocate shrinks the array we're looping over.
				continue;
			}

			size_t filterDataSizeAllocated = filter.runtimedata()->filterDataSize;
			size_t filterDataSizeCalculated = filter.filterdata().length();
			if (filterDataSizeAllocated != filterDataSizeCalculated) {
				LOGAssetFilterWarn("Deallocating filter ID=%u because filter size does not match: allocated=%u calculated=%u",
						filter.runtimedata()->filterId,
						filterDataSizeAllocated,
						filterDataSizeCalculated);
				_logArray(SERIAL_DEBUG, true, filter.filterdata()._data, filterDataSizeAllocated);

				deallocateFilterByIndex(index);
				checksFailed = true;

				// Intentionally skipping index++, deallocate shrinks the array we're looping over.
				continue;
			}
		}

		index++;
	}

	return checksFailed;
}

void AssetFilterStore::computeFilterCrcs() {
	LOGAssetFilterDebug("computeFilterCrcs");
	for (uint8_t* filterBuffer : _filters) {
		if (filterBuffer == nullptr) {
			break;
		}

		AssetFilter filter(filterBuffer);
		if (filter.runtimedata()->flags.flags.crcCalculated == false) {
			filter.runtimedata()->crc = crc32(filter.filterdata().metadata()._data, filter.runtimedata()->filterDataSize, nullptr);
			filter.runtimedata()->flags.flags.crcCalculated = true;
			LOGAssetFilterDebug("Calculate CRC for filter filterId=%u, CRC=0x%x", filter.runtimedata()->filterId, filter.runtimedata()->crc);
		}
		else {
			LOGAssetFilterDebug("Skip filter CRC calculation for filterId=%u, CRC=0x%x", filter.runtimedata()->filterId, filter.runtimedata()->crc)
		}
	}
}

void AssetFilterStore::storeFilters() {
	LOGAssetFilterDebug("storeFilters");
	for (uint8_t* filterBuffer : _filters) {
		if (filterBuffer == nullptr) {
			break;
		}

		AssetFilter filter(filterBuffer);
		if (filter.runtimedata()->flags.flags.committed == true) {
			continue;
		}

		cs_state_data_t stateData(
				getStateType(filter.filterdata().length()),
				filter.runtimedata()->filterId,
				filter.filterdata()._data,
				getStateSize(filter.filterdata().length()));
		LOGAssetFilterDebug("store stateType=%u stateId=%u size=%u", stateData.type, stateData.id, stateData.size);
		State::getInstance().set(stateData);
	}
}

void AssetFilterStore::loadFromFlash() {
	// Load all filters.
	for (uint8_t i = 0; i < 255; ++i) {
		CS_TYPE type = getNthStateType(i);
		if (type == CS_TYPE::CONFIG_DO_NOT_USE) {
			break;
		}
		loadFromFlash(type);
	}

	// Load master version and CRC.
	TYPIFY(STATE_ASSET_FILTERS_VERSION) stateVal;
	State::getInstance().get(CS_TYPE::STATE_ASSET_FILTERS_VERSION, &stateVal, sizeof(stateVal));
	LOGAssetFilterDebug("Loaded from flash: masterVersion=%u masterCrc=%u", stateVal.masterVersion, stateVal.masterCrc);

	// Commit filters.
	cs_ret_code_t retCode = ERR_UNSPECIFIED;
	if (stateVal.masterVersion != 0) {
		retCode = commit(stateVal.masterVersion, stateVal.masterCrc, false);
	}

	// In case of error: remove all filters. (saves memory and improves boot-up speed)
	if (retCode != ERR_SUCCESS) {
		// Remove all filters: reverse iterate.
		for (uint8_t index = _filtersCount; index > 0; --index) {
			deallocateFilterByIndex(index - 1);
		}
		endInProgress(0, 0);
	}
}

void AssetFilterStore::loadFromFlash(CS_TYPE type) {
	LOGAssetFilterDebug("loadFromFlash type=%u", type);
	cs_ret_code_t retCode;

	std::vector<cs_state_id_t>* ids = nullptr;
	retCode = State::getInstance().getIds(type, ids);
	if (retCode != ERR_SUCCESS) {
		return;
	}

	for (auto id: *ids) {
		if (id >= MAX_FILTER_IDS) {
			LOGw("Invalid id: %u", id);
			continue;
		}

		deallocateFilter(id);
		uint16_t stateSize = TypeSize(type);
		uint8_t* filterBuf = allocateFilter(id, stateSize);

		if (filterBuf == nullptr) {
			LOGw("Failed to allocate filter buffer");
			continue;
		}

		AssetFilter filter(filterBuf);

		cs_state_data_t stateData (type, id, filter.filterdata()._data, stateSize);
		retCode = State::getInstance().get(stateData);
		if (retCode != ERR_SUCCESS) {
			deallocateFilter(id);
			continue;
		}

		filter.runtimedata()->filterId       = id;
		filter.runtimedata()->flags.asInt    = 0;
		filter.runtimedata()->filterDataSize = filter.filterdata().length();
		
		if (filter.runtimedata()->filterDataSize > stateSize) {
			LOGw("Calculated filter data size is too large: filterDataSize=%u stateSize=%u", filter.runtimedata()->filterDataSize, stateSize);
			deallocateFilter(id);
			continue;
		}
		LOGAssetFilterDebug("filtertype %u, inputType %u, outputType %u",
				*filter.filterdata().metadata().filterType(),
				*filter.filterdata().metadata().inputType().type(),
				*filter.filterdata().metadata().outputType().outFormat());
	}
}

void AssetFilterStore::markFiltersCommitted() {
	LOGAssetFilterDebug("markFiltersCommitted");
	for (size_t index = 0; index < _filtersCount; ++index) {
		auto filter = AssetFilter(_filters[index]);

		if (filter._data == nullptr) {
			break;
		}
		filter.runtimedata()->flags.flags.committed = true;
	}
}
