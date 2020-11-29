/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_iBeaconParser.h>
#include <ble/cs_BleConstants.h>
#include <common/cs_Types.h>
#include <structs/cs_PacketsInternal.h>
#include <structs/cs_StreamBufferAccessor.h>
#include <util/cs_Utils.h>


void IBeaconParser::handleEvent(event_t& evt){
	if (evt.type != CS_TYPE::EVT_DEVICE_SCANNED) {
		return;
	}

	scanned_device_t* dev = UNTYPIFY(EVT_DEVICE_SCANNED, evt.data);
	/* auto beacon = */ getIBeacon(dev);

}


void /* IBeacon */ IBeaconParser::getIBeacon(scanned_device_t* scanned_device){
	uint32_t errCode;
	cs_data_t manufacturerData;

	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
			scanned_device->data,
			scanned_device->dataSize,
			&manufacturerData);

	if (errCode != ERR_SUCCESS) {
		return;
	}

	StreamBufferAccessor bufferaccess(manufacturerData);

	uint16_t companyId = bufferaccess.get<uint16_t>();
//	uint8_t appleAdvType = bufferaccess.get<uint8_t>();

	switch(companyId){
		case BleCompanyId::Apple:
			LOGi("iBeacon received: Apple");
			break;
		case BleCompanyId::Crownstone:
			LOGi("iBeacon received: Crownstone");
			break;
		case BleCompanyId::Tile:
			LOGi("iBeacon received: Tile");
			break;
		default:
			break;
	}

}
