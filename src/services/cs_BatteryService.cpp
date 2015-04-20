/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr. 20, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <cs_BluetoothLE.h>

namespace BLEpp {

/* Battery service
 *
 * Defines a single characteristic to read a battery level. This is a predefined UUID, stored at
 * <BLE_UUID_BATTERY_LEVEL_CHAR>. The name is "battery", and the default value is 100.
 */
class BatteryService : public GenericService {

public:
	// Define func_t as a templated function with an unsigned byte
	typedef function<uint8_t()> func_t;

protected:
	// A single characteristic with an unsigned 8-bit value
	Characteristic<uint8_t> *_characteristic;
	// A function for callback, not in use
	func_t _func;
public:
	// Constructor sets name, allocate characteristic, sets UUID, and sets default value.
	BatteryService(): GenericService() {
		setUUID(UUID(BLE_UUID_BATTERY_SERVICE));
		setName(BLE_SERVICE_BATTERY);

		_characteristic = new Characteristic<uint8_t>();
		addCharacteristic(_characteristic);

		_characteristic->setUUID(UUID(BLE_UUID_BATTERY_LEVEL_CHAR));
		_characteristic->setName(BLE_CHAR_BATTERY);
		_characteristic->setDefaultValue(100);
	}

	/* Set the battery level
	 * @batteryLevel level of the battery in percentage
	 *
	 * Indicates the level of the battery in a percentage to the user. This is of no use for a device attached to
	 * the main line. This function writes to the characteristic to show it to the user.
	 */
	void setBatteryLevel(uint8_t batteryLevel){
		(*_characteristic) = batteryLevel;
	}

	/* Set a callback function for a battery level change
	 * @func callback function
	 *
	 * Not in use
	 */
	void setBatteryLevelHandler(func_t func) {
		_func = func;
	}
};

} // put in BLEpp namespace
