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

#include <localisation/cs_Nearestnearestwitnessreport.h>
#include <localisation/cs_TrackableParser.h>
#include <localisation/cs_TrackableEvent.h>

void TrackableParser::handleEvent(event_t& evt) {
	if (evt.type != CS_TYPE::EVT_DEVICE_SCANNED) {
		return;
	}

	scanned_device_t* dev = UNTYPIFY(EVT_DEVICE_SCANNED, evt.data);
	if (handleAsTileDevice(dev)) {
		return;
	}
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
	tile.print("found tile device!");

	if(!isMyTrackable(scanned_device)) {
		// it was a Tile device, so return true.
		return true;
	}

	logServiceData(scanned_device);

	TrackableEvent trackevt;
	trackevt.rssi = scanned_device->rssi;
	trackevt.dispatch();

	return true;
}

// ======================== Utils ========================

void TrackableParser::logServiceData(scanned_device_t* scanned_device) {
	uint32_t errCode;
	cs_data_t service_data;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_SERVICE_DATA,
			scanned_device->data,
			scanned_device->dataSize,
			&service_data);

	if (errCode != ERR_SUCCESS) {
		return;
	}

	// format string
	char buff_start[50] = {0}; // should be len(servicedata) * len(repr) ~= 3*12 = 36.
	char* buff = buff_start;
	for (auto i = 0u; i < service_data.len; i++) {
		buff += sprintf(buff, "%2x,", service_data.data[i]);
	}

	// and log the formatted string
	LOGd("my tile servicedata(%d): [%s]", service_data.len, buff_start);
}

