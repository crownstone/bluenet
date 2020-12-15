/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_BleConstants.h>
#include <common/cs_Types.h>
#include <structs/cs_PacketsInternal.h>
#include <structs/cs_StreamBufferAccessor.h>
#include <util/cs_Utils.h>
#include <logging/cs_Logger.h>

#include <localisation/cs_Nearestnearestwitnessreport.h>
#include <localisation/cs_TrackableParser.h>
#include <localisation/cs_TrackableEvent.h>

// REVIEW: This won't be recognized by binary logger.
#define TrackableParser_LOGd LOGnone

void TrackableParser::init() {
}

void TrackableParser::handleEvent(event_t& evt) {
	if (evt.type == CS_TYPE::EVT_ADV_BACKGROUND_PARSED) {
		adv_background_parsed_t *parsed_adv = CS_TYPE_CAST(EVT_ADV_BACKGROUND_PARSED,evt.data);
		handleBackgroundParsed(parsed_adv);
		return;
	}

	if (evt.type == CS_TYPE::EVT_DEVICE_SCANNED) {
		scanned_device_t* scanned_device = CS_TYPE_CAST(EVT_DEVICE_SCANNED, evt.data);
		handleAsTileDevice(scanned_device);
		// add other trackable device types here

		return;
	}
}


void TrackableParser::handleBackgroundParsed(
		adv_background_parsed_t *trackable_advertisement) {
	// TODO: implement when we have a good representation of trackables in the mesh.
	//
	//	TrackableEvent trackevent;
	//	trackevent.id = TrackableId(trackable_advertisement->macAddress);
	//	trackevent.rssi = trackable_advertisement->adjustedRssi;
	//
	//	trackevent.dispatch();
}


// ====================== Mac Filter =====================

bool TrackableParser::isMyTrackable(scanned_device_t* scanned_device) {
	// Note: mac address here as read in nrf connect app, hence the std::reverse call.
	uint8_t my_tile_mac[] = {0xe4, 0x96, 0x62, 0x0d, 0x5a, 0x5b};
	std::reverse(std::begin(my_tile_mac), std::end(my_tile_mac));
	TrackableId myTrackable(my_tile_mac);

	// construct TrackableId for incomming scan
	TrackableId mac(scanned_device->address);

	return mac == myTrackable;
}

// ======================== Tile ========================

bool TrackableParser::isTileDevice(scanned_device_t* scanned_device) {
	uint32_t errCode;
	cs_data_t service_uuid16_list;

	errCode = BLEutil::findAdvType(
			BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE,
			scanned_device->data,
			scanned_device->dataSize,
			&service_uuid16_list );

	if (errCode != ERR_SUCCESS) {
		return false;
	}

	uint16_t* uuid_list = reinterpret_cast<uint16_t*>(service_uuid16_list.data);

	BleServiceUuid tile_ids[] = {
			BleServiceUuid::TileX,
			BleServiceUuid::TileY,
			BleServiceUuid::TileZ };

	for (auto i = 0u; i < service_uuid16_list.len; i++) {
		for (auto j = 0u; j < std::size(tile_ids); j++) {
			if (uuid_list[i] == tile_ids[j]) {
				return true;
			}
		}
	}

	return false;
}


bool TrackableParser::handleAsTileDevice(scanned_device_t* scanned_device) {
	if (!isTileDevice(scanned_device)) {
		return false;
	}


	TrackableId tile(scanned_device->address);

	TrackableParser_LOGd("Tile device: rssi=%d ", scanned_device->rssi);
	tile.print(" ");

	if(!isMyTrackable(scanned_device)) {
		// it was a Tile device, so return true.
		return true;
	}

	// logServiceData("Tile device servicedata", scanned_device);

	TrackableEvent trackevt;
	trackevt.rssi = scanned_device->rssi;
	trackevt.dispatch();

	return true;
}


// ======================== Utils ========================

void TrackableParser::logServiceData(const char* headerstr, scanned_device_t* scanned_device) {
	uint32_t errCode;
	cs_data_t service_data;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_SERVICE_DATA,
			scanned_device->data,
			scanned_device->dataSize,
			&service_data);

	if (errCode != ERR_SUCCESS) {
		return;
	}

	// REVIEW: Use the Util function for this.
	_log(SERIAL_INFO, false, "len=%d data=[", service_data.len);
	for (auto i = 0u; i < service_data.len; i++) {
		_log(SERIAL_INFO, false, " %2x,", service_data.data[i]);
	}
	_log(SERIAL_INFO, true, "]");
}

