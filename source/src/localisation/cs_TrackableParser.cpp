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


// ====================== Mac Filter =====================

bool TrackableParser::isMyTrackable(scanned_device_t* scannedDevice) {
	TrackableId myTrackableId{0xe4, 0x96, 0x62, 0x0d, 0x5a, 0x5b}; // Arend's test Tile, as read on nrf connect app
	TrackableId incomingTrackableId(scannedDevice->address, sizeof(scannedDevice->address));

	return myTrackableId == incomingTrackableId;
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

	TrackableId tile(scannedDevice->address, sizeof(scannedDevice->address));

	LOGTrackableParserDebug("incoming tile device: rssi=%i ", scannedDevice->rssi);
	tile.print();

	if (!isMyTrackable(scannedDevice)) {
		// it was a Tile device, so return true.
		return true;
	}

	logServiceData(scannedDevice);

	TrackableEvent trackEvt;
	trackEvt.rssi = scannedDevice->rssi;
	trackEvt.dispatch();

	return true;
}


// ======================== Utils ========================

void TrackableParser::logServiceData(scanned_device_t* scannedDevice) {
	uint32_t errCode;
	cs_data_t serviceData;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_SERVICE_DATA,
			scannedDevice->data,
			scannedDevice->dataSize,
			&serviceData);

	if (errCode != ERR_SUCCESS) {
		return;
	}

	_log(SERIAL_DEBUG, false, "servicedata trackableparser: ");
	_logArray(SERIAL_DEBUG,false,serviceData.data, serviceData.len);
}

