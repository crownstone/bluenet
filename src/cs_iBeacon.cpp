/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <cs_iBeacon.h>
#include <util/cs_Utils.h>

using namespace BLEpp;

void IBeacon::toArray(uint8_t* array) {

	*((uint16_t*) array) = BLEutil::convertEndian16(_adv_indicator);
	array += 2;

	ble_uuid128_t uuid = (ble_uuid128_t) _uuid;
	for (int i = 0; i < 16; ++i) {
		*array++ = uuid.uuid128[15 - i];
	}

	*((uint16_t*) array) = BLEutil::convertEndian16(_major);
	array += 2;

	*((uint16_t*) array) = BLEutil::convertEndian16(_minor);
	array += 2;

	*array = _rssi;
}
