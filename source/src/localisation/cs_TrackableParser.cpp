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

void TrackableParser::TrackableParser() {
	// construct TrackableId to filter for
	// (input here as read in nrf connect app)
	uint8_t my_tile_mac[] = {0xe4, 0x96, 0x62, 0x0d, 0x5a, 0x5b};
	std::reverse(std::begin(my_tile_mac), std::end(my_tile_mac));
	myTile = TrackableId (my_tile_mac);
}


void TrackableParser::handleEvent(event_t& evt){
	if (evt.type != CS_TYPE::EVT_DEVICE_SCANNED) {
		return;
	}

	scanned_device_t* dev = UNTYPIFY(EVT_DEVICE_SCANNED, evt.data);
	if (handleAsTileDevice(dev)) {
		return;
	}

}

bool TrackableParser::handleAsTileDevice(scanned_device_t* scanned_device) {
	if (!isTileDevice(scanned_device)) {
		return false;
	}

	uint32_t errCode;
	cs_data_t service_data;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_SERVICE_DATA,
			scanned_device->data,
			scanned_device->dataSize,
			&service_data);

	if (errCode != ERR_SUCCESS) {
		// recognized as tile device, malformed a bit but still valid.
		return true;
	}

	LOGd("TODO: log service data for development.");
	return true;
}

bool TrackableParser::handleMyTrackable(scanned_device_t* scanned_device) {
	// construct TrackableId for incomming scan
	TrackableId mac(scanned_device->address);

	if (mac == myTrackable) {
		LOGd("=========================");
		mac.print("found my tile!");
		LOGd("=========================");
	}
}

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
