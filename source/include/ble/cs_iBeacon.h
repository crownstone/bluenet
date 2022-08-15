/**
 * @file
 * Universally Unique IDentifiers for BLE services and characteristics.
 *
 * @authors Crownstone Team, Christopher Mason.
 * @copyright Crownstone B.V.
 * @date Nov 4, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_UUID.h>
#include <protocol/cs_Typedefs.h>
#include <util/cs_Utils.h>

/** Implementation of the iBeacon specification.
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
	/**
	 * Union so that we can directly use the array for the advertisment's manufacturing data.
	 * Because the advertisement package has to be in big-endian, and nordic's chip is using
	 * little-endian, the values are converted in the setter/getter functions, so that the array
	 * is correct and doesn't need to be converted anymore to be used in the advertisement data.
	 * As a result, changes to the IBeacon values, such as major, minor, etc, will directly reflect
	 * in the advertisement data once they are changed here.
	 */
	union {
		/** Individual fields.
		 */
		struct {
			/** Advertisement indicator, defined as 0x0215 for iBeacons
			 */
			uint16_t adv_indicator;

			/** Proximity UUID, shared for all iBeacons for a given application
			 */
			cs_uuid128_t uuid;

			/** Major number (group level identifier)
			 */
			uint16_t major;

			/** Minor number (individual nodes)
			 */
			uint16_t minor;

			/** Known (calibrated) rssi value at 1m distance
			 *
			 * This value has to be calibrated for each iBeacon so that it represents the
			 * signal strength of the iBeacon at 1m distance. The value is then used by a
			 * smartphone together with the current rssi reading to calculate the current
			 * distance from the iBeacon.
			 */
			int8_t txPower;
		} _params;
		/** Buffer
		 */
		uint8_t _buffer[sizeof(_params)];
	};

public:
	/** Default constructor for the iBeacon class
	 *
	 * @uuid the UUID for this application
	 *
	 * @major the major value for this iBeacon (group level identifier)
	 *
	 * @minor the minor value for this iBeacon (individual node)
	 *
	 * @rssi the calibrated rssi value at 1m distance
	 */
	IBeacon(cs_uuid128_t uuid, uint16_t major, uint16_t minor, int8_t rssi);

	/** The size of the iBeacon advertisement data
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
	uint8_t size() { return 23; }

	uint8_t* getArray() { return _buffer; }

	/** Set major value */
	void setMajor(uint16_t major);
	/** Get major value  */
	uint16_t getMajor();

	/** Set minor value */
	void setMinor(uint16_t minor);
	/** Get minor value */
	uint16_t getMinor();

	/** Set UUID */
	void setUUID(cs_uuid128_t& uuid);
	/** Get UUID */
	cs_uuid128_t getUUID();

	/** Set RSSI value */
	void setTxPower(int8_t txPower);
	/** Get RSSI value */
	int8_t getTxPower();
};
