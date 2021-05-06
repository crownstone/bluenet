/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_BleConstants.h>
#include <common/cs_Types.h>
#include <localisation/cs_Nearestnearestwitnessreport.h>
#include <localisation/cs_TrackableEvent.h>
#include <localisation/cs_TrackableParser.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_ErrorCodes.h>
#include <structs/cs_PacketsInternal.h>
#include <structs/cs_StreamBufferAccessor.h>
#include <util/cs_Crc16.h>
#include <util/cs_CuckooFilter.h>
#include <util/cs_Utils.h>

#include <localisation/cs_TrackableParserPacketAccessors.h>

#define LOGTrackableParserDebug LOGnone
#define LOGTrackableParserWarn LOGw

// ---------------------- Trackable Parser ----------------------

void TrackableParser::init() {
	LOGTrackableParserDebug("trackable parser initialised");
	endProgress(_masterHash, _masterVersion);
}

void TrackableParser::handleEvent(event_t& evt) {
	switch (evt.type) {
		// incoming devices to filter
		case CS_TYPE::EVT_ADV_BACKGROUND_PARSED: {
			auto parsedAdv = CS_TYPE_CAST(EVT_ADV_BACKGROUND_PARSED, evt.data);
			handleBackgroundParsed(parsedAdv);
			return;
		}
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			auto scannedDevice = CS_TYPE_CAST(EVT_DEVICE_SCANNED, evt.data);
			handleScannedDevice(scannedDevice);
			return;
		}

		// incoming filter commands
		case CS_TYPE::CMD_UPLOAD_FILTER: {
			LOGTrackableParserDebug("CMD_UPLOAD_FILTER");

			auto commandPacket         = CS_TYPE_CAST(CMD_UPLOAD_FILTER, evt.data);
			if(commandPacket != nullptr) {
				evt.result.returnCode = handleUploadFilterCommand(commandPacket);
			}
			break;
		}
		case CS_TYPE::CMD_REMOVE_FILTER: {
			LOGTrackableParserDebug("CMD_REMOVE_FILTER");

			auto commandPacket         = CS_TYPE_CAST(CMD_REMOVE_FILTER, evt.data);
			if(commandPacket != nullptr) {
				evt.result.returnCode = handleRemoveFilterCommand(commandPacket);
			}
			break;
		}
		case CS_TYPE::CMD_COMMIT_FILTER_CHANGES: {
			LOGTrackableParserDebug("CMD_COMMIT_FILTER_CHANGES");

			auto commandPacket         = CS_TYPE_CAST(CMD_COMMIT_FILTER_CHANGES, evt.data);
			if(commandPacket != nullptr) {
				evt.result.returnCode = handleCommitFilterChangesCommand(commandPacket);
			}
			break;
		}
		case CS_TYPE::CMD_GET_FILTER_SUMMARIES: {
			LOGTrackableParserDebug("CMD_GET_FILTER_SUMMARIES");
			handleGetFilterSummariesCommand(evt.result);
			break;
		}
		default: break;
	}
}

// TODO(#177858707): remove this when the linked task is implemented
bool findUuid128(scanned_device_t* scannedDevice) {
	uint32_t errCode;
	cs_data_t serviceUuid16List;

	errCode = BLEutil::findAdvType(
			BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE,
			scannedDevice->data,
			scannedDevice->dataSize,
			&serviceUuid16List);

	if (errCode == ERR_SUCCESS) {
		return true;
	}

	errCode = BLEutil::findAdvType(
			BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE,
			scannedDevice->data,
			scannedDevice->dataSize,
			&serviceUuid16List);

	if (errCode == ERR_SUCCESS) {
		return true;
	}

	return false;
}

// -------------------------------------------------------------
// ------------------ Advertisment processing ------------------
// -------------------------------------------------------------
void TrackableParser::handleScannedDevice(scanned_device_t* device) {
	if (isInProgress()) {
		return;
	}

	// loop over filters to check mac address
	for (size_t i = 0; i < _parsingFiltersCount; ++i) {
		TrackingFilter trackingFilter(_parsingFilters[i]);
		CuckooFilter cuckoo = trackingFilter.filterdata().filterdata();

		// check mac address for this filter
		if (*trackingFilter.filterdata().metadata().inputType().type() == AdvertisementSubdataType::MacAddress
			&& cuckoo.contains(device->address, MAC_ADDRESS_LEN)) {
			// TODO(#177858707):
			// - get fingerprint from filter instead of literal mac address.
			// - we should just pass on the device, right than copying rssi?
			LOGw("filter %d accepted adv from mac addres: %x %x %x %x %x %x %s",
				 trackingFilter.runtimedata()->filterId,
				 device->address[0],
				 device->address[1],
				 device->address[2],
				 device->address[3],
				 device->address[4],
				 device->address[5],
				 findUuid128(device) ? "button pressed!"
									 : "");  // TODO(#177858707) when in/out types is added, test with actual uuid

			TrackableEvent trackableEventData;
			trackableEventData.id   = TrackableId(device->address, MAC_ADDRESS_LEN);
			trackableEventData.rssi = device->rssi;

			event_t trackableEvent(CS_TYPE::EVT_TRACKABLE, &trackableEventData, sizeof(trackableEventData));
			trackableEvent.dispatch();
			return;
		}
	}

	// TODO(#177858707): Add the AD Data loop for other filter inputTypes.
	// loop over filters fields to check addata fields
	//	// keeps fields as outer loop because that's more expensive to loop over.
	// 	// See BLEutil:findAdvType how to loop the fields.
	//	for (auto field: devicefields) {
	//		for(size_t i = 0; i < _parsingFiltersCount; ++i) {
	//			tracking_filter_t* filter = _parsingFilters[i];
	//			if(filter->metadata.inputType == FilterInputType::AdData) {
	//				// check each AD Data field for this filter
	//				// TODO: query filter for fingerprint
	//				TrackableEvent trackEvent;
	//				trackEvent.dispatch();
	//				return;
	//			}
	//		}
	//	}
}

void TrackableParser::handleBackgroundParsed(adv_background_parsed_t* trackableAdv) {
	// TODO: implement?
}

// -------------------------------------------------------------
// ------------------- Filter data management ------------------
// -------------------------------------------------------------

uint8_t* TrackableParser::allocateParsingFilter(uint8_t filterId, size_t payloadSize) {
	LOGTrackableParserDebug("Allocating parsing filter #%d, of size %d (endindex: %u)", filterId, payloadSize, _parsingFiltersCount);
	size_t totalSize = sizeof(tracking_filter_runtime_data_t) + payloadSize;

	if (_parsingFiltersCount >= MAX_FILTER_IDS) {
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
		LOGTrackableParserWarn("Filter couldn't be allocated, heap is too full");
		return nullptr;
	}

	// create space for the new entry and keep the list sorted
	uint8_t index_for_new_entry = 0;

	for (uint8_t i = _parsingFiltersCount; i > 0; --i) {
		// _parsingFiltersCount points exactly one after the last non-nullptr
		// i is always a valid position: because of previous check it is strictly less than MAX_FILTER_IDS
		// (i-1) won't roll over because i > 0.

		if (TrackingFilter(_parsingFilters[i - 1]).runtimedata()->filterId > filterId) {
			// move this entry up one position in the list.
			_parsingFilters[i] = _parsingFilters[i - 1];
		}
		else {
			index_for_new_entry = i;
			break;
		}
	}

	_parsingFilters[index_for_new_entry] = newFilterBuffer;

	TrackingFilter newFilterAccessor(newFilterBuffer);

	LOGTrackableParserDebug("before assignments");
	// initialize runtime data.
	newFilterAccessor.runtimedata()->filterId  = filterId;
	newFilterAccessor.runtimedata()->totalSize = payloadSize;
	newFilterAccessor.runtimedata()->crc       = 0;

	// don't forget to increment the Count for the filter list.
	_parsingFiltersCount++;

	return newFilterBuffer;
}

void TrackableParser::deallocateParsingFilterByIndex(uint8_t parsingFilterIndex) {
	if (parsingFilterIndex >= MAX_FILTER_IDS) {
		return;
	}

	// (delete can handle nullptr according to c++ standard)
	delete[] _parsingFilters[parsingFilterIndex];

	LOGTrackableParserDebug("post-delete.");

	// shift entries after the deleted one an index downward (overwriting the deleted pointer).
	for (size_t i = parsingFilterIndex + 1; i < _parsingFiltersCount; i++) {
		_parsingFilters[i - 1] = _parsingFilters[i];
	}
	// and remove the trailing pointer.
	_parsingFilters[_parsingFiltersCount - 1] = nullptr;

	// removed an entry, so reduce end by 1.
	_parsingFiltersCount--;
}

bool TrackableParser::deallocateParsingFilter(uint8_t filterId) {
	LOGTrackableParserDebug("deallocating %u", filterId);

	auto filterIndexOpt = findParsingFilterIndex(filterId);
	if (filterIndexOpt) {
		deallocateParsingFilterByIndex(filterIndexOpt.value());
		return true;
	}

	LOGTrackableParserDebug("tried to deallocate but filter id wasn't found.");
	return false;
}

uint8_t* TrackableParser::findParsingFilter(uint8_t filterId) {
	auto filterIndexOpt = findParsingFilterIndex(filterId);

	return filterIndexOpt ? _parsingFilters[filterIndexOpt.value()] : nullptr;
}

std::optional<size_t> TrackableParser::findParsingFilterIndex(uint8_t filterId) {
	LOGTrackableParserDebug("Looking up filter %d, end index: %d", filterId, _parsingFiltersCount);

	uint8_t* trackingFilter;
	for (size_t index = 0; index < _parsingFiltersCount; ++index) {
		trackingFilter = _parsingFilters[index];

		if (trackingFilter == nullptr) {
			LOGw("_parsingFiltersCount incorrect: found nullptr before reaching end of filter "
				 "list.");
			return {};
		}

		if (TrackingFilter(trackingFilter).runtimedata()->filterId == filterId) {
			LOGTrackableParserDebug("Filter found at index %d", index);
			return index;
		}
	}

	return {};
}

size_t TrackableParser::getTotalHeapAllocatedSize() {
	LOGTrackableParserDebug("computing allocated size");

	size_t total = 0;
	for (uint8_t* trackingFilter : _parsingFilters) {
		if (trackingFilter == nullptr) {
			// reached back of the list (nullptr).
			break;
		}
		total += TrackingFilter(trackingFilter).length();
	}

	return total;
}

// -------------------------------------------------------------
// ---------------------- Command interface --------------------
// -------------------------------------------------------------

cs_ret_code_t TrackableParser::handleUploadFilterCommand(trackable_parser_cmd_upload_filter_t* cmd_data) {
	startProgress();

	LOGTrackableParserDebug("upload command received chunkStartIndex %d, chunkSize %d, totalSize %d",
			cmd_data->chunkStartIndex,
			cmd_data->chunkSize,
			cmd_data->totalSize);

	if (cmd_data->chunkStartIndex + cmd_data->chunkSize > cmd_data->totalSize) {
		LOGTrackableParserWarn("Chunk overflows total size.");
		return ERR_INVALID_MESSAGE;
	}

	// find or allocate a parsing filter
	TrackingFilter parsingFilter(findParsingFilter(cmd_data->filterId));

	// check if we need to clean up an old filter.
	if (parsingFilter._data != nullptr && parsingFilter.runtimedata()->crc != 0) {
		LOGTrackableParserDebug("removing pre-existing filter with same id (%d)", cmd_data->filterId);
		deallocateParsingFilter(parsingFilter.runtimedata()->filterId);

		// after deallocation, the filter is clearly gone.
		parsingFilter._data = nullptr;
	}

	// check if the total size hasn't changed in the mean time
	if (parsingFilter._data != nullptr && parsingFilter.runtimedata()->totalSize != cmd_data->totalSize) {
		// If total size to allocate changes something is really off on the host side
		LOGe("Corrupt chunk of upload data received. Deallocating partially constructed filter.");
		deallocateParsingFilter(parsingFilter.runtimedata()->filterId);
		return ERR_WRONG_STATE;
	}

	// check if we need to create a new filter
	if (parsingFilter._data == nullptr) {
		LOGTrackableParserDebug("parsing filter %d not found", cmd_data->filterId);
		// command totalSize only includes the size of the cuckoo filter and its metadata, not the runtime data yet.
		parsingFilter = allocateParsingFilter(cmd_data->filterId, cmd_data->totalSize);

		if (parsingFilter._data == nullptr) {
			// failed to handle command, no space.
			LOGTrackableParserWarn("parsing filter allocation failed");
			return ERR_NO_SPACE;
		}
	}

	// by now, the filter exists, is clean, and the incoming chunk is verified for totalSize consistency.

	// chunk index starts counting from metadata onwards (ignoring runtimedata)
	uint8_t* parsingFilterBase_ptr = parsingFilter.filterdata()._data;
	uint8_t* parsingFilterChunk_ptr = parsingFilterBase_ptr + cmd_data->chunkStartIndex;

	// apply filter chunk, counting chunk index from metadata onwards:
	std::memcpy(parsingFilterChunk_ptr, cmd_data->chunk, cmd_data->chunkSize);

	return ERR_SUCCESS;
}

cs_ret_code_t TrackableParser::handleRemoveFilterCommand(trackable_parser_cmd_remove_filter_t* cmd_data) {
	startProgress();

	return deallocateParsingFilter(cmd_data->filterId) ? ERR_SUCCESS : ERR_NOT_FOUND;
}

cs_ret_code_t TrackableParser::handleCommitFilterChangesCommand(
		trackable_parser_cmd_commit_filter_changes_t* cmd_data) {

	if (checkFilterSizeConsistency()) {
		return ERR_WRONG_STATE;
	}

	computeCrcs();

	uint16_t masterHash = masterCrc();
	if (cmd_data->masterCrc != masterHash) {
		LOGTrackableParserWarn("Master Crc did not match (%x != %x, _filterModificationInProgress stays true.",
				cmd_data->masterCrc,
				masterHash);
		return ERR_MISMATCH;
	}

	// finish commit by writing the _masterHash
	endProgress(cmd_data->masterCrc, cmd_data->masterVersion);

	// NOTE: if writing flash, this may need to be delayed until write success.
	return ERR_SUCCESS;
}

void TrackableParser::handleGetFilterSummariesCommand(cs_result_t& result) {
	LOGTrackableParserDebug("handle get filter summaries:");
	for (auto& trackingFilter : _parsingFilters) {
		if (trackingFilter == nullptr) {
			LOGTrackableParserDebug("filter: nullptr");
			continue;
		}
		LOGTrackableParserDebug("filter: %u", trackingFilter->runtimedata.filterId);
	}

	// stack allocate a buffer summaries object fitting at most max summaries:
	auto requiredBuffSize = sizeof(trackable_parser_cmd_get_filter_summaries_ret_t)
							+ sizeof(tracking_filter_summary_t) * _parsingFiltersCount;

	if (result.buf.len < requiredBuffSize) {
		result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}

	// placement new constructs the object in the buff and now has enough space for the summaries.
	auto retvalptr  = new (result.buf.data) trackable_parser_cmd_get_filter_summaries_ret_t;
	result.dataSize = requiredBuffSize;

	retvalptr->masterCrc     = _masterHash;
	retvalptr->masterVersion = _masterVersion;
	retvalptr->freeSpace     = getTotalHeapAllocatedSize();

	for (size_t i = 0; i < _parsingFiltersCount; i++) {
		TrackingFilter filter(_parsingFilters[i]);
		retvalptr->summaries[i].id   = filter.runtimedata()->filterId;
		retvalptr->summaries[i].type = static_cast<uint8_t>(*filter.filterdata().metadata().type());
		retvalptr->summaries[i].crc  = filter.runtimedata()->crc;
	}

	result.returnCode = ERR_SUCCESS;
}

// -------------------------------------------------------------
// ---------------------- Utility functions --------------------
// -------------------------------------------------------------
void TrackableParser::startProgress() {
	_filterModificationInProgress = true;
	_masterVersion                = 0;
}

void TrackableParser::endProgress(uint16_t newMasterHash, uint16_t newMasterVersion) {
	_masterHash                   = newMasterHash;
	_masterVersion                = newMasterVersion;
	_filterModificationInProgress = false;
}

bool TrackableParser::isInProgress() {
	return _filterModificationInProgress;
}


uint16_t TrackableParser::masterCrc() {
	uint16_t masterCrc = 0;
	uint16_t* prevCrc  = nullptr;

	for (auto trackingFilterBuffer : _parsingFilters) {
		if (trackingFilterBuffer == nullptr) {
			break;
		}

		TrackingFilter trackingFilter(trackingFilterBuffer);

		LOGTrackableParserDebug("filter crc: %x", trackingFilter->runtimedata.crc);

		// appends/applies this filters' crc onto master crc
		masterCrc =
				crc16(reinterpret_cast<const uint8_t*>(trackingFilter.runtimedata()->crc),
					  sizeof(trackingFilter.runtimedata()->crc),
					  prevCrc);

		prevCrc = &masterCrc;  // first iteration will have prevCrc == nullptr, all subsequent ones point to masterCrc.
	}

	LOGTrackableParserDebug("master crc: %x", masterCrc);

	return masterCrc;
}

bool TrackableParser::checkFilterSizeConsistency() {
	bool consistencyChecksFailed = false;

	size_t index = 0;
	while (index < _parsingFiltersCount) {
		auto trackingFilter = TrackingFilter(_parsingFilters[index]);

		if (trackingFilter._data == nullptr) {
			// early return: last filter has been handled.
			break;
		}

		if (trackingFilter.runtimedata()->crc == 0) {
			// filter changed since commit.

			size_t trackingFilterAllocatedSize = trackingFilter.runtimedata()->totalSize;
			size_t trackingFilterSize = trackingFilter.length();

			if (trackingFilterSize != trackingFilterAllocatedSize) {
				// the cuckoo filter size parameters do not match the allocated space.
				// likely cause by malformed data on the host side.
				// This check can't be performed earlier: as long as not all chuncks are
				// received the filterdata may be in invalid state.

				LOGTrackableParserWarn("Deallocating filter %u because of malformed cuckoofilter size: %u != %u",
						trackingFilter.runtimedata()->filterId,
						trackingFilterSize,
						trackingFilterAllocatedSize);

				deallocateParsingFilterByIndex(index);
				consistencyChecksFailed = true;

				// intentionally skipping index++, deallocate shrinks the array we're looping over.
				continue;
			}
		}

		index++;
	}

	return consistencyChecksFailed;
}

void TrackableParser::computeCrcs() {
	for (uint8_t* trackingFilterBuffer : _parsingFilters) {
		if (trackingFilterBuffer == nullptr) {
			break;
		}

		TrackingFilter trackingFilter(trackingFilterBuffer);

		if (trackingFilter.runtimedata()->crc == 0) {
			// filter changed since commit.
			trackingFilter.runtimedata()->crc =
					crc16(trackingFilter.filterdata().metadata()._data,
						  trackingFilter.runtimedata()->totalSize,
						  nullptr);
		}
	}
}

// -------------------------------------------------------------
// ----------------------- OLD interface -----------------------
// -------------------------------------------------------------

// ======================== Tile ========================

bool TrackableParser::isTileDevice(scanned_device_t* scannedDevice) {
	uint32_t errCode;
	cs_data_t serviceUuid16List;

	errCode = BLEutil::findAdvType(
			BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE,
			scannedDevice->data,
			scannedDevice->dataSize,
			&serviceUuid16List);

	if (errCode != ERR_SUCCESS) {
		return false;
	}

	uint16_t* uuidList = reinterpret_cast<uint16_t*>(serviceUuid16List.data);

	BleServiceUuid tileServiceUuids[] = {BleServiceUuid::TileX, BleServiceUuid::TileY, BleServiceUuid::TileZ};

	for (auto i = 0u; i < serviceUuid16List.len; i++) {
		for (auto j = 0u; j < std::size(tileServiceUuids); j++) {
			if (uuidList[i] == tileServiceUuids[j]) {
				return true;
			}
		}
	}

	return false;
}

// ======================== Utils ========================

void TrackableParser::logServiceData(scanned_device_t* scannedDevice) {
	uint32_t errCode;
	cs_data_t serviceData;
	errCode = BLEutil::findAdvType(
			BLE_GAP_AD_TYPE_SERVICE_DATA, scannedDevice->data, scannedDevice->dataSize, &serviceData);

	if (errCode != ERR_SUCCESS) {
		return;
	}

	_log(SERIAL_DEBUG, false, "servicedata trackableparser: ");
	_logArray(SERIAL_DEBUG, true, serviceData.data, serviceData.len);
}
