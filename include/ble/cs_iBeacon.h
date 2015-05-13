/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#pragma once

#include <ble/cs_UUID.h>

#include <drivers/cs_Serial.h>

namespace BLEpp {

/* Implementation of the iBeacon specification.
 *
 * The implementation of the iBeacon specification is only about advertising at predefined intervals and
 * casting a payload (the manufacturing data of an advertisement package) in the form of a specific
 * structure:
 *
 *  * iBeacon Prefix
 *  * proximity UUID. An identifier which should be used to distinguish your beacons from others (for a
 *    given application, all of your iBeacons share the same UUID), the individual iBeacons are then
 *    distinguished by major and minor numbers
 *  * major (group level identifier, e.g. on the level of a building, unique amongst all nodes with a
 *    given proximity UUID)
 *  * minor (individual nodes, unique amongst all nodes with a given proximity UUID and major number)
 *  * rssi value (known rssi value at 1m distance, used to calculate actual distance from the iBeacon)
 *
 * Note that you might not be able to use this commercially! Although I would be surprised if it is possible
 * to patent a "struct" or the Apple prefix.
 */

class IBeacon {
	private:
		/* Advertisement indicator, defined as 0x0215 for iBeacons
		 */
		uint16_t _adv_indicator;

		/* proximity UUID, shared for all iBeacons for a given application
		 */
		ble_uuid128_t _uuid;

		/* Major number (group level identifier)
		 */
		uint16_t _major;

		/* Minor number (individual nodes)
		 */
		uint16_t _minor;

		/* Known (calibrated) rssi value at 1m distance
		 *
		 * This value has to be calibrated for each iBeacon so that it represents the
		 * signal strength of the iBeacon at 1m distance. The value is then used by a
		 * smartphone together with the current rssi reading to calculate the current
		 * distance from the iBeacon.
		 */
		int8_t _rssi;

	public:
		/* Default constructor for the iBeacon class
		 *
		 * @uuid the UUID for this application
		 *
		 * @major the major value for this iBeacon (group level identifier)
		 *
		 * @minor the minor value for this iBeacon (individual node)
		 *
		 * @rssi the calibrated rssi value at 1m distance
		 */
		IBeacon(ble_uuid128_t uuid, uint16_t major, uint16_t minor, int8_t rssi) {
			// advertisement indicator for an iBeacon is defined as 0x0215
			_adv_indicator = 0x0215;
			_uuid = uuid;
			_major = major;
			_minor = minor;
			_rssi = rssi;
		}

		/* The size of the iBeacon advertisement data
		 *
		 * size is calculated as:
		 * 		2B		advertisement indicator
		 * 		16B		uuid (as byte array)
		 * 		2B		major
		 * 		2B		minor
		 * 		1B		rssi
		 * 	--------------------------------------
		 * 		23B		total
		 *
		 * @return the size of the advertisement data
		 */
		uint8_t size() {
			return 23;
		}

		/* Serializes the object to a byte array
		 *
		 * @array pointer to the preallocated byte array where the
		 *   data should be copied into. Use <size> to get the required
		 *   length of the array
		 *
		 * The format of the array is:
		 *   ADVERTISEMENT_INDICATOR, UUID, MAJOR, MINOR, RSSI
		 *
		 * The function also converts the values from little-endian (used by
		 * nordic) to the big-endian format, required for the iBeacon
		 * advertisement package
		 */
		void toArray(uint8_t* array);

		/* Set major value */
		inline void setMajor(uint16_t major) { LOGd("setMajor: %d", major); _major = major; }
		/* Get major value  */
		inline uint16_t getMajor() { return _major; }

		/* Set minor value */
		inline void setMinor(uint16_t minor) { LOGd("setMinor: %d", minor); _minor = minor; }
		/* Get minor value */
		inline uint16_t getMinor() { return _minor; }

		/* Set UUID */
		inline void setUUID(ble_uuid128_t uuid) { LOGd("setUUID"); _uuid = uuid; }
		/* Get UUID */
		inline ble_uuid128_t getUUID() { return _uuid; }

		/* Set RSSI value */
		inline void setRSSI(int8_t rssi) { LOGd("setRSSI: %d", rssi); _rssi = rssi; }
		/* Get RSSI value */
		inline int8_t getRSSI() { return _rssi; }
};

} // end namespace
