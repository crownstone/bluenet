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


void TrackableParser::init() {
	uuid_printer = Coroutine([this](){
		LOGi("current known mac addresses (%d):", mac_log.size());
		for(auto& uuid : mac_log){
			LOGi("mac [%2x %2x %2x %2x %2x %2x]",
					uuid.bytes[0],
					uuid.bytes[1],
					uuid.bytes[2],
					uuid.bytes[3],
					uuid.bytes[4],
					uuid.bytes[5]
		   );
		}

		return Coroutine::delayS(10);
	});
}

void TrackableParser::handleEvent(event_t& evt) {
	if (uuid_printer.handleEvent(evt)) {
		return;
	}

	if (evt.type == CS_TYPE::EVT_ADV_BACKGROUND_PARSED) {
		adv_background_parsed_t *parsed_adv = UNTYPIFY(EVT_ADV_BACKGROUND_PARSED,evt.data);
		handleBackgroundParsed(parsed_adv);
		return;
	}

	if (evt.type != CS_TYPE::EVT_DEVICE_SCANNED) {
		scanned_device_t* scanned_device = UNTYPIFY(EVT_DEVICE_SCANNED, evt.data);

		logUuid(*scanned_device);
		handleAsTileDevice(scanned_device);
		// add other trackable device types here
	}
}


void TrackableParser::handleBackgroundParsed(
		adv_background_parsed_t *trackable_advertisement) {
	// TODO: Throttle here?

	TrackableEvent trackevent;
	trackevent.id = TrackableId(trackable_advertisement->macAddress);
	trackevent.rssi = trackable_advertisement->adjustedRssi;

	trackevent.dispatch();
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

// ======================== UUIDs ========================

void TrackableParser::logUuid(scanned_device_t scanned_device) {
	mac_log.emplace(scanned_device.address);
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

