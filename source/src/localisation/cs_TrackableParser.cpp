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
#include <structs/cs_PacketsInternal.h>
#include <structs/cs_StreamBufferAccessor.h>
#include <util/cs_Utils.h>

// REVIEW: This won't be recognized by binary logger.
#define LOGTrackableParserDebug LOGnone

void TrackableParser::init() {
}

void TrackableParser::handleEvent(event_t& evt) {
	if (evt.type == CS_TYPE::EVT_ADV_BACKGROUND_PARSED) {
		adv_background_parsed_t *parsedAdv = CS_TYPE_CAST(EVT_ADV_BACKGROUND_PARSED, evt.data);
		handleBackgroundParsed(parsedAdv);
		return;
	}

	if (evt.type == CS_TYPE::EVT_DEVICE_SCANNED) {
		scanned_device_t* scannedDevice = CS_TYPE_CAST(EVT_DEVICE_SCANNED, evt.data);
		handleAsTileDevice(scannedDevice);
		// add other trackable device types here

		return;
	}
}


void TrackableParser::handleBackgroundParsed(adv_background_parsed_t *trackableAdv) {
	// TODO: implement when we have a good representation of trackables in the mesh.
	//
	//	TrackableEvent trackEvent;
	//	trackEvent.id = TrackableId(trackableAdv->macAddress);
	//	trackEvent.rssi = trackableAdv->adjustedRssi;
	//
	//	trackEvent.dispatch();
}

// -------------------------------------------------------------
// ------------------ Internal filter management ---------------
// -------------------------------------------------------------

bool TrackableParser::allocateParsingFilter(uint8_t filterId, size_t size) {
	// structure of the buffer:
	// [CuckooFilter_0][CuckooData_0] ... [CuckooFilter_n][CuckooData_n] 0x00 ... 0x00
	// remove operations will immediately move all data to make space.
	// to find index of first empty buffer space, check the highest indexed filter.

	// preliminary implementation: ignore filterId, always start at index 0
	(void)filterId;
	size_t parsingFilterIndex = 0;
	size_t filterDataIndex = 0 + sizeof(ParsingFilter);

	if (FILTER_BUFFER_SIZE < parsingFilterIndex) {
		// can't even allocate the CuckooFilter object in this case.
		return false;
	}

	size_t bufferSpaceLeftForFilterData = FILTER_BUFFER_SIZE - parsingFilterIndex;

	uint8_t* parsingFilterPosition =  _filterBuffer + parsingFilterIndex;
	uint8_t* filterDataPosition    =  _filterBuffer + filterDataIndex;

	// placement new is used to construct an object into a user specified location
	// without copying. It is exception safe by specification.
	_parsingFilters[0] = new (parsingFilterPosition) ParsingFilter;

	// After constructing the instance, we need to assign the buffer space for it.
	bool filterDataFitsBuffer = _parsingFilters[0]->filter.assignBuffer (
			bucket_count, nests_per_bucket,
			filterDataPosition, bufferSpaceLeftForFilterData);

	if (!filterDataFitsBuffer) {
		// buffer not big enough for requested number of fingerprints,
		// reverting changes (placement new requires manual destruction).
		_parsingFilters[0]->~ParsingFilter();
		_parsingFilters[0] = nullptr;
		return false;
	}

	return true;
}

bool TrackableParser::applyFilterChunk (
		uint8_t filterId,
		uint16_t chunkStartIndex,
		uint8_t* chunk,
		uint16_t chunkSize) {
	ParsingFilter* parsingFilter = findParsingFilter(filterId);

	if (parsingFilter == nullptr) {
		// If this is the 'first chunk', try to allocate a filter.
		allocateFilter(filterId, )
		return false;
	}

	// First chunk needs to be handled separately.


	return true;
}


TrackableParser::ParsingFilter* TrackableParser::findParsingFilter(uint8_t filterId) {
	return reinterpret_cast<ParsingFilter*>(_filterBuffer);
}


// -------------------------------------------------------------
// ---------------------- Command interface --------------------
// -------------------------------------------------------------

void TrackableParser::handleGetFilterSummariesCommand() {
	// TODO(Arend): implement later.
}

void TrackableParser::handleCommitFilterChangesCommand(uint16_t masterversion, uint16_t mastercrc) {
	// TODO(Arend): implement later.
}

void TrackableParser::handleRemoveFilterCommand(uint8_t filterId) {
	// TODO(Arend): implement later.
}

bool TrackableParser::handleUploadFilterCommand(uint8_t filterId, uint16_t chunkStartIndex, uint16_t totalSize, uint8_t* chunk, uint16_t chunkSize) {
	ParsingFilter* parsingFilter = findParsingFilter(filterId);

	if(parsingFilter == nullptr) {
		parsingFilter = allocateParsingFilter(filterId, totalSize);
		if(parsingFilter == nullptr) {
			// failed to handle command, no space.
			return false;
		}
	}

	// note: we're not doing corruption checks here yet
	// E.g.: when totalSize changes for a specific filterId before
	//       a commit is reached, we have a parsingFilter allocated
	//       but it's of the wrong size.

	// apply filter chunk:
	std::memcpy (parsingFilter + chunkStartIndex, chunk, chunkSize);

	return true;
}

// -------------------------------------------------------------
// ----------------------- OLD interface -----------------------
// -------------------------------------------------------------




// ====================== Mac Filter =====================

bool TrackableParser::isMyTrackable(scanned_device_t* scannedDevice) {
	// Note: mac address here as read in nrf connect app, hence the std::reverse call.
	uint8_t myTileMac[] = {0xe4, 0x96, 0x62, 0x0d, 0x5a, 0x5b};
	std::reverse(std::begin(myTileMac), std::end(myTileMac));
	TrackableId myTrackable(myTileMac);

	// construct TrackableId for incomming scan
	TrackableId mac(scannedDevice->address);

	return mac == myTrackable;
}

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

	BleServiceUuid tileServiceUuids[] = {
			BleServiceUuid::TileX,
			BleServiceUuid::TileY,
			BleServiceUuid::TileZ };

	for (auto i = 0u; i < serviceUuid16List.len; i++) {
		for (auto j = 0u; j < std::size(tileServiceUuids); j++) {
			if (uuidList[i] == tileServiceUuids[j]) {
				return true;
			}
		}
	}

	return false;
}


bool TrackableParser::handleAsTileDevice(scanned_device_t* scannedDevice) {
	if (!isTileDevice(scannedDevice)) {
		return false;
	}


	TrackableId tile(scannedDevice->address);

	LOGTrackableParserDebug("Tile device: rssi=%i ", scannedDevice->rssi);
	tile.print(" ");

	if (!isMyTrackable(scannedDevice)) {
		// it was a Tile device, so return true.
		return true;
	}

	// logServiceData("Tile device servicedata", scanned_device);

	TrackableEvent trackEvt;
	trackEvt.rssi = scannedDevice->rssi;
	trackEvt.dispatch();

	return true;
}


// ======================== Utils ========================

// REVIEW: string as argument can't be left out from binary size.
void TrackableParser::logServiceData(const char* headerStr, scanned_device_t* scannedDevice) {
	uint32_t errCode;
	cs_data_t serviceData;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_SERVICE_DATA,
			scannedDevice->data,
			scannedDevice->dataSize,
			&serviceData);

	if (errCode != ERR_SUCCESS) {
		return;
	}

	// REVIEW: Use the Util function for this.
	_log(SERIAL_INFO, false, "len=%d data=[", serviceData.len);
	for (auto i = 0u; i < serviceData.len; i++) {
		_log(SERIAL_INFO, false, " %2x,", serviceData.data[i]);
	}
	_log(SERIAL_INFO, true, "]");
}

