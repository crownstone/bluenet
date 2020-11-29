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

void IBeaconParser::handleEvent(event_t& evt){
	if (evt.type != CS_TYPE::EVT_DEVICE_SCANNED) {
		return;
	}

	scanned_device_t* dev = UNTYPIFY(EVT_DEVICE_SCANNED, evt.data);



}


iBeacon IBeaconParser::getIBeacon(scanned_device_t* dev){
	uint16_t companyId = *((uint16_t*)(manufacturerData.data));
		if (companyId != BleCompanyId::Tile) {
			return;
		}

}
