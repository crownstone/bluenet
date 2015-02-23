/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#pragma once

#include <cs_UUID.h>

namespace BLEpp {

/* Implementation of the iBeacon specification.
 *
 * The implementation of the iBeacon specification is only about advertising at predefined intervals and 
 * casting a payload in the form of a specific structure:
 *
 *  * Apple Prefix
 *  * UUID of company
 *  * major (group level identifier, e.g. on the level of a building)
 *  * minor (individual nodes)
 *
 * Note that you might not be able to use this commercially! Although I would be surprised if it is possible
 * to patent a "struct" or the Apple prefix.
 */
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

		/* The size of the iBeacon advertisement
		 *
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
