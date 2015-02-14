/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#pragma once

#include <cs_BluetoothLE.h>

namespace BLEpp {

class IBeacon {
	private:
		uint16_t _adv_indicator;
		UUID _uuid;
		uint16_t _major;
		uint16_t _minor;
		uint8_t _rssi;

	public:
		IBeacon(UUID uuid, uint16_t major, uint16_t minor, uint8_t rssi) {
			// advertisement indicator for an iBeacon is defined as 0x0215
			_adv_indicator = 0x0215;
			_uuid = uuid;
			_major = major;
			_minor = minor;
			_rssi = rssi;
		}

		/*
		 * size is calculated as:
		 * 		2B		advertisement indicator
		 * 		16B		uuid (as byte array)
		 * 		2B		major
		 * 		2B		minor
		 * 		1B		rssi
		 * 	--------------------------------------
		 * 		23B		total
		 */
		uint8_t size() {
			return 23;
		}

		void toArray(uint8_t* array);
};

} // end namespace
