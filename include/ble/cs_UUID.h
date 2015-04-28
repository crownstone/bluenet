/**
 * Author: Christopher Mason
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Feb 23, 2015
 * License: LGPLv3+
 */
#pragma once

//#include <cstdint>

#include <ble/cs_Nordic.h>

namespace BLEpp {

// TODO: CRAPPY IMPLEMENTATION!!!! implement in a clean, methodical and understandable way

/* A Universally Unique IDentifier.
 *
 * This is a 128-bit sequence for non-standard profiles, services, characteristics, or descriptors.
 * There are several identifiers predefined at
 * https://developer.bluetooth.org/gatt/services/Pages/ServicesHome.aspx.
 * Normally a 16-bit sequence is enough to distinguish the different services, a large part of the 128-bit
 * sequence is repeated.
 */
class UUID {

protected:
	/* proprietary UUID std::string. NULL for standard UUIDs.
	 */
	const char*           _full;

	/* 16-bit UUID value or octets 12-13 of 128-bit UUID.
	 */
	uint16_t              _uuid;

	/* UUID type, see <BLE_UUID_TYPES>.
	 */
	uint8_t               _type;

public:
	/* Empty constructor
	 *
	 * Creates an empty UUID object, and sets all variables to unknown
	 */
	UUID() : _full(NULL), _uuid(0xdead), _type(BLE_UUID_TYPE_UNKNOWN) {};

	/* Default constructor
	 *
	 * @fullUid the UUID as a string
	 *
	 * Initialize the object with the given fullUid.
	 */
	UUID(const char* fullUid):
		_full(fullUid), _uuid(0x0), _type(BLE_UUID_TYPE_UNKNOWN) {};

	/* Constructor for Bluetooth SIG UUID (16-bit)
	 *
	 * @uuid the 16-bit UUID
	 *
	 * Initialize the object with the given uuid.
	 *
	 * **Note**: only use this constructor if you have a Bluetooth SIG
	 * certified UUID (16-bit)
	 */
	UUID(uint16_t uuid) :
		_full(NULL), _uuid(uuid), _type(BLE_UUID_TYPE_BLE) { }

	/* Constructor for Characteristic UUID generation
	 *
	 * @parent the UUID of the Service
	 *
	 * @uidValue the identifier of the characteristic
	 *
	 * Initialize the object with the parent's uuid but replace the
	 * bytes 12-13 (little-endian) with the <uidValue>
	 *
	 * Use this function to create UUIDs for charateristics where the
	 * services UUID is known and the charateristic's ID given as an
	 * uint16_t value
	 */
	UUID(UUID& parent, uint16_t uidValue):
		_full(NULL), _uuid(uidValue), _type(parent.init()) {}

	/* Copy constructor
	 *
	 * @rhs the <UUID> object from which the values should be taken over
	 *
	 * Initializes this object with the values of the given UUID object,
	 * thus creating an exact copy of the given UUID.
	 */
	UUID(const UUID& rhs):
		_full(rhs._full), _uuid(rhs._uuid), _type(rhs._type) {}

	/* Constructor
	 *
	 * @value the values of the object as type <ble_uuid_t>
	 *
	 * Initializes this objects with the values given as parameter.
	 */
	UUID(const ble_uuid_t& value):
		_full(NULL), _uuid(value.uuid), _type(value.type) {} // FIXME NRFAPI

	/* Getter for the 16-bit UUID
	 *
	 * @return the 16-bit UUID
	 */
	uint16_t getUuid() const {
		return _uuid;
	}

	/* Getter for the type
	 *
	 * @return the UUID type. See <BLE_UUID_TYPES>
	 */
	uint8_t getType() {
		init();
		return _type;
	}

	/* Initialize this object
	 *
	 * @return the UUID type of this object. See <BLE_UUID_TYPES>
	 */
	uint16_t init();

	/* Cast operator <ble_uuid_t>
	 *
	 * Casts or rather converts this object to the type
	 * <ble_uuid_t>
	 *
	 * @return the converted object as type <ble_uuid_t>
	 */
	operator ble_uuid_t();

	/* Cast operator <ble_uuid128_t>
	 *
	 * Casts or rather converts this object to the type
	 * <ble_uuid128_t> by parsing the <_full> array and
	 * converting it to a 128-bit buetooth address in
	 * little-endian
	 *
	 * @return the bluetooth address as a 16 byte array
	 *   in little-endian
	 */
	operator ble_uuid128_t();
};

}
