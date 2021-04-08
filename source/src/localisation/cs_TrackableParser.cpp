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
#include <util/cs_CuckooFilter.h>
#include <util/cs_Utils.h>

#define LOGTrackableParserDebug LOGd
#define LOGTrackableParserWarn LOGw

// ---------------------- local/temp utils ----------------------

void logfilter(tracking_filter_t* filter) {
	if (filter != nullptr) {
		LOGTrackableParserDebug("medatadata.inputType: %d", filter->metadata.inputType);
		LOGTrackableParserDebug("medatadata.profileId: %x", filter->metadata.profileId);
		LOGTrackableParserDebug("medatadata.protocol: %d", filter->metadata.protocol);
		LOGTrackableParserDebug("medatadata.version: %d", filter->metadata.version);
		LOGTrackableParserDebug("medatadata.flags: %x", filter->metadata.flags);
		LOGTrackableParserDebug("filterdata.: %d", filter->filterdata.bucketCountLog2);
		LOGTrackableParserDebug("filterdata.: %d", filter->filterdata.nestsPerBucket);
		LOGTrackableParserDebug("filterdata.: %d", filter->filterdata.victim.fingerprint);
		LOGTrackableParserDebug("filterdata.: %d", filter->filterdata.victim.bucketA);
		LOGTrackableParserDebug("filterdata.: %d", filter->filterdata.victim.bucketB);

		auto cuckoo = CuckooFilter(filter->filterdata);
		LOGTrackableParserDebug("cuckoo filter buffer size: %d", cuckoo.bufferSize());
	}
	else {
		LOGTrackableParserDebug("Trying to print tracking filter but pointer is null");
	}
}

// ---------------------- Trackable Parser ----------------------

void TrackableParser::init() {
	LOGTrackableParserDebug("trackable parser initialised");
	filterModificationInProgress = false;
}

void TrackableParser::handleEvent(event_t& evt) {
	switch (evt.type) {
		// incoming devices to filter
		case CS_TYPE::EVT_ADV_BACKGROUND_PARSED: {
			adv_background_parsed_t* parsedAdv = CS_TYPE_CAST(EVT_ADV_BACKGROUND_PARSED, evt.data);
			handleBackgroundParsed(parsedAdv);
			return;
		}
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			scanned_device_t* scannedDevice = CS_TYPE_CAST(EVT_DEVICE_SCANNED, evt.data);
			handleScannedDevice(scannedDevice);
			return;
		}

		// incoming filter commands
		case CS_TYPE::CMD_UPLOAD_FILTER: {
			LOGTrackableParserDebug("CMD_UPLOAD_FILTER");

			trackable_parser_cmd_upload_filter_t* cmd_data = CS_TYPE_CAST(CMD_UPLOAD_FILTER, evt.data);

			evt.result.returnCode = handleUploadFilterCommand(cmd_data);
			break;
		}
		case CS_TYPE::CMD_REMOVE_FILTER: {
			LOGTrackableParserDebug("CMD_REMOVE_FILTER");

			trackable_parser_cmd_remove_filter_t* cmd_data = CS_TYPE_CAST(CMD_REMOVE_FILTER, evt.data);

			handleRemoveFilterCommand(cmd_data);
			break;
		}
		case CS_TYPE::CMD_COMMIT_FILTER_CHANGES: {
			LOGTrackableParserDebug("CMD_COMMIT_FILTER_CHANGES not implemented yet");

			trackable_parser_cmd_commit_filter_changes_t* cmd_data = CS_TYPE_CAST(CMD_COMMIT_FILTER_CHANGES, evt.data);
			handleCommitFilterChangesCommand(cmd_data);
			break;
		}
		case CS_TYPE::CMD_GET_FILTER_SUMMARIES: {
			LOGTrackableParserDebug("CMD_GET_FILTER_SUMMARIES not implemented yet");
			return;

			trackable_parser_cmd_get_filer_summaries_t* cmd_data = CS_TYPE_CAST(CMD_GET_FILTER_SUMMARIES, evt.data);
			handleGetFilterSummariesCommand(cmd_data);
			break;
		}
		default: break;
	}
}

// -------------------------------------------------------------
// ------------------ Advertisment processing ------------------
// -------------------------------------------------------------
void TrackableParser::handleScannedDevice(scanned_device_t* device) {
	if (filterModificationInProgress) {
		return;
	}

	// loop over filters to check mac address
	for (size_t i = 0; i < _parsingFiltersEndIndex; ++i) {
		tracking_filter_t* trackingFilter = _parsingFilters[i];
		CuckooFilter cuckoo(trackingFilter->filterdata);

		// check mac address for this filter
		if (trackingFilter->metadata.inputType == FilterInputType::MacAddress
			&& cuckoo.contains(device->address, MAC_ADDRESS_LEN)) {
			// TODO: get fingerprint from filter instead of literal mac address.
			// TODO: we should just pass on the device, right than copying rssi?
			LOGw("filter %d accepted mac addres: %x %x %x %x %x %x",
				 trackingFilter->runtimedata.filterId,
				 device->address[0],
				 device->address[1],
				 device->address[2],
				 device->address[3],
				 device->address[4],
				 device->address[5]);

			continue;  // DEBUG
			TrackableEvent trackableEventData;
			trackableEventData.id   = TrackableId(device->address, MAC_ADDRESS_LEN);
			trackableEventData.rssi = device->rssi;

			event_t trackableEvent(CS_TYPE::EVT_TRACKABLE, &trackableEventData, sizeof(trackableEventData));
			trackableEvent.dispatch();
			return;
		}
	}

	// TODO: Add the AD Data loop for other filter inputTypes.
	// loop over filters fields to check addata fields
	//	// keeps fields as outer loop because that's more expensive to loop over.
	// 	// See BLEutil:findAdvType how to loop the fields.
	//	for (auto field: devicefields) {
	//		for(size_t i = 0; i < _parsingFiltersEndIndex; ++i) {
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
	// TODO: implement
}

// -------------------------------------------------------------
// ------------------- Filter data management ------------------
// -------------------------------------------------------------

tracking_filter_t* TrackableParser::allocateParsingFilter(uint8_t filterId, size_t payloadSize) {
	LOGTrackableParserDebug("Allocating parsing filter #%d, of size %d (endindex: %u)", filterId, payloadSize, _parsingFiltersEndIndex);
	size_t totalSize = sizeof(tracking_filter_runtime_data_t) + payloadSize;


	if (getTotalHeapAllocatedSize() + totalSize > FILTER_BUFFER_SIZE) {
		// not enough space for filter of this total size.
		return nullptr;
	}

	if (_parsingFiltersEndIndex >= MAX_FILTER_IDS) {
		// not enough space for more filter ids.
		return nullptr;
	}

	// just a shorthand
	auto& newFilterLocation = _parsingFilters[_parsingFiltersEndIndex];

	LOGTrackableParserDebug("Try allocation");
	// try heap allocation
	uint8_t* foo = new (std::nothrow) uint8_t[totalSize];
	newFilterLocation =	reinterpret_cast<tracking_filter_t*>(foo);

	LOGTrackableParserDebug("after allocation");

	if (newFilterLocation == nullptr) {
		// new couldn't be allocated, heap is too full.
		return nullptr;
	}

	LOGTrackableParserDebug("before assignments");
	// initialize runtime data.
	newFilterLocation->runtimedata.totalSize = payloadSize;
	newFilterLocation->runtimedata.filterId  = filterId;
	newFilterLocation->runtimedata.crc       = 0;

	// don't forget to postcrement the EndIndex for the filter list.
	_parsingFiltersEndIndex++;

	return newFilterLocation;
}

tracking_filter_t* TrackableParser::findParsingFilter(uint8_t filterId) {
	LOGTrackableParserDebug("Looking up filter %d, end index: %d", filterId, _parsingFiltersEndIndex);

	tracking_filter_t* trackingFilter;
	for (size_t index = 0; index < _parsingFiltersEndIndex; ++index) {
		trackingFilter = _parsingFilters[index];

		if (trackingFilter == nullptr) {
			LOGw("_parsingFiltersEndIndex incorrect: found nullptr before reaching end of filter "
				 "list.");
			return nullptr;
		}

		if (trackingFilter->runtimedata.filterId == filterId) {
			LOGTrackableParserDebug("Filter found at index %d", index);
			return _parsingFilters[index];
		}
	}

	return nullptr;
}

void TrackableParser::deallocateParsingFilter(uint8_t filterId) {
	LOGTrackableParserDebug("deallocating %u", filterId);

	for (size_t index = 0; index < _parsingFiltersEndIndex; ++index) {
		tracking_filter_t*& trackingFilter_ref = _parsingFilters[index];
		if(trackingFilter_ref == nullptr) {
			// we keep the array nullptr terminated.
			break;
		}

		if (trackingFilter_ref->runtimedata.filterId == filterId) {
			LOGTrackableParserDebug("found filter. deleting");
			auto asByteArray = reinterpret_cast<uint8_t*>(trackingFilter_ref);
			delete[] asByteArray;
			LOGTrackableParserDebug("post-delete.");

			if(_parsingFiltersEndIndex < MAX_FILTER_IDS) {
				// move the last filter in the array into the empty slot.
				trackingFilter_ref = _parsingFilters[index];
				_parsingFilters[index] = nullptr;
			} else {
				trackingFilter_ref = nullptr;
			}
			// eiter way, the last filter has been cleaned up, so reduce filter end by 1.
			_parsingFiltersEndIndex--;
		}
	}
}

size_t TrackableParser::getTotalHeapAllocatedSize() {
	LOGTrackableParserDebug("computing allocated size");

	size_t total = 0;
	for (tracking_filter_t* trackingFilter : _parsingFilters) {
		if (trackingFilter == nullptr) {
			// reached back of the list (nullptr).
			break;
		}
		total += sizeof(tracking_filter_runtime_data_t) + trackingFilter->runtimedata.totalSize;
	}

	return total;
}

// -------------------------------------------------------------
// ---------------------- Command interface --------------------
// -------------------------------------------------------------

cs_ret_code_t TrackableParser::handleUploadFilterCommand(trackable_parser_cmd_upload_filter_t* cmd_data) {
	filterModificationInProgress = true;
	_masterVersion = 0;

	LOGTrackableParserDebug("start %d, chunksize %d, total size %d", cmd_data->chunkStartIndex, cmd_data->chunkSize, cmd_data->totalSize);

	if (cmd_data->chunkStartIndex + cmd_data->chunkSize > cmd_data->totalSize) {
		LOGTrackableParserWarn("Chunk overflows total size.");
		return ERR_INVALID_MESSAGE;
	}

	// find or allocate a parsing filter
	tracking_filter_t* parsingFilter = findParsingFilter(cmd_data->filterId);

	// check if we need to clean up an old filter.
	if (parsingFilter != nullptr && parsingFilter->runtimedata.crc != 0) {
		LOGTrackableParserDebug("pre-existing filter %d found", cmd_data->filterId);

		LOGTrackableParserDebug("upload command received but pre-existing filter is not removed.");
		deallocateParsingFilter(parsingFilter->runtimedata.filterId);

		// after deallocation, the filter is clearly gone.
		LOGw("assumption fails, deallocation not implemented yet");
		parsingFilter = nullptr;
	}

	// check if the total size hasn't changed in the mean time
	if (parsingFilter != nullptr && parsingFilter->runtimedata.totalSize != cmd_data->totalSize){
		// this really is a mess. @Bart do we re-allocate and report partial success,
		// or do we refuse to continue and de-allocate to clean up?
		LOGe("Handling of corrupt incoming command not fully implemented.");
		return ERR_WRONG_PARAMETER;
	}

	// check if we need to create a new filter
	if (parsingFilter == nullptr) {
		LOGTrackableParserDebug("parsing filter %d not found", cmd_data->filterId);
		// command totalSize only includes the size of the cuckoo filter and its metadata, not the runtime data yet.
		parsingFilter = allocateParsingFilter(cmd_data->filterId, cmd_data->totalSize);

		if (parsingFilter == nullptr) {
			// failed to handle command, no space.
			LOGTrackableParserWarn("parsing filter allocation failed");
			return ERR_NO_SPACE;
		}
	}

	// by now, the filter exists, is clean, and the incoming chunk is verified for totalSize consistency.

	// chunk index starts counting from metadata onwards (ignoring runtimedata)
	uint8_t* parsingFilterBase_ptr  = reinterpret_cast<uint8_t*>(&(parsingFilter->metadata));
	uint8_t* parsingFilterChunk_ptr = parsingFilterBase_ptr + cmd_data->chunkStartIndex;

	// apply filter chunk, counting chunk index from metadata onwards:
	std::memcpy(parsingFilterChunk_ptr, cmd_data->chunk, cmd_data->chunkSize);

	return ERR_SUCCESS;
}

cs_ret_code_t TrackableParser::handleRemoveFilterCommand(trackable_parser_cmd_remove_filter_t* cmd_data) {
	filterModificationInProgress = true;
	_masterVersion = 0;

	deallocateParsingFilter(cmd_data->filterId);

	return ERR_NOT_IMPLEMENTED; // TODO: change to success
}

cs_ret_code_t TrackableParser::handleCommitFilterChangesCommand(
		trackable_parser_cmd_commit_filter_changes_t* cmd_data) {
	tracking_filter_t* parsingFilter = findParsingFilter(1);
	logfilter(parsingFilter);

	uint8_t testelement[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9};

	auto cuckoo   = CuckooFilter(parsingFilter->filterdata);
	bool contains = cuckoo.contains(testelement, sizeof(testelement));
	LOGd("cuckoo contains test element: %s", contains ? "true" : "false");

	// TODO(Arend): implement later.
	// - compute and check all filter sizes (i.e. cuckoo.size() + sizeof... == runtimemetadata.totalSize
	// - compute and check all filter crcs (maybe only the ones wit crc == 0?)
	// - compute and check(?) master crc
	// - persist all filters
	// - broadcast update to the mesh

	filterModificationInProgress = false;  // NOTE: if writing flash, this may need to be delayed until write success.

	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t TrackableParser::handleGetFilterSummariesCommand(trackable_parser_cmd_get_filer_summaries_t* cmd_data) {
	if (filterModificationInProgress) {
		return ERR_BUSY;
	}

	// TODO(Arend): implement later.
	return ERR_NOT_IMPLEMENTED;
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
