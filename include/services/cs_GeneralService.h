/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

//#include <vector>
//
#include "drivers/cs_Storage.h"
#include "structs/buffer/cs_StreamBuffer.h"
//#include "characteristics/cs_MeshMessage.h"
//#include "characteristics/cs_UuidConfig.h"
//#include "common/cs_Storage.h"
//#include "ble/cs_BluetoothLE.h"
#include <ble/cs_Stack.h>
#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
//#include "processing/cs_Temperature.h"

#define GENERAL_SERVICE_UPDATE_FREQUENCY 10 // hz

/* General Service for the Crownstone
 *
 * There are several characteristics that fit into the general service description. There is a characteristic
 * that measures the temperature, there are several characteristics that defines the crownstone, namely by
 * name, by type, or by location (room), and there is a characteristic to update its firmware.
 *
 * If meshing is enabled, it is also possible to send a message into the mesh network using a characteristic.
 */
class GeneralService: public BLEpp::Service {
public:
	/* Constructor for general crownstone service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store name and other information that is
	 * set over so-called configuration characteristics. It also initializes all characteristics.
	 */
	 GeneralService();

	/* Overload start
	 */
//	void startAdvertising(BLEpp::Nrf51822BluetoothStack* stack);

	/* Update the temperature characteristic.
	 * @temperature A value in Celcius directly from the chip
	 *
	 * This writes a value to the temperature characteristic which can subsequently read out by the user. If we write
	 * often to this characteristic this value will be updated.
	 */
	void writeToTemperatureCharac(int32_t temperature);

	/* Update the configuration characteristic.
	 * @type configuration type to read
	 *
	 * Persistent memory can be used to store multiple types of objects. The "name" of the device is one type for
	 * example, the "floor" is another. Type here specifies the category of the entity to be retrieved from FLASH.
	 */
//	bool readFromStorage(uint8_t type);

	/* Write to the "get configuration" characteristic
	 *
	 * Writing is done by setting the data length properly and notifying the characteristic (and hence the softdevice)
	 * that there is a new value available.
	 */
	void writeToConfigCharac();

	/* Write configuration to FLASH.
	 * @type configuration type to write
	 * @length length of the data to write to FLASH
	 * @payload the data itself to write to FLASH
	 *
	 * The configuration is (probably) set by the user through the "set configuration" characteristic and needs to be
	 * written to FLASH. That is what this function takes care of. The writing itself will be done asynchronously, so
	 * there is no return value that indicates success or not.
	 */
//	void writeToStorage(uint8_t type, uint8_t length, uint8_t* payload);

	/* Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things. Urgent matters have to be
	 * resolved immediately in interrupt service handlers. The temperature for example is updated every
	 * tick, because timing is not important for this at all.
	 */
	void tick();

	void scheduleNextTick();

	/* Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void init();


protected:
	/* Reference to the Bluetooth LE stack.
	 *
	 * A reference to the BLE stack does allow this service to:
	 *
	 *  * add itself to the stack
	 *  * stop/start advertising
	 *  * set the BLE name
	 */
//	BLEpp::Nrf51822BluetoothStack* _stack;

	/* Temperature characteristic
	 */
	BLEpp::Characteristic<int32_t>* _temperatureCharacteristic;

	/* Reset characteristic
	 *
	 * Resets device
	 */
	BLEpp::Characteristic<int32_t>* _resetCharacteristic;

	/* Mesh characteristic
	 *
	 * Sends a message over the mesh network
	 */
	BLEpp::Characteristic<buffer_ptr_t>* _meshCharacteristic;

	/* Set configuration characteristic
	 *
	 * The configuration characteristic reuses the format of the mesh messages. The type are identifiers that are
	 * established:
	 *
	 *  * 0 name
	 *  * 1 device type
	 *  * 2 room
	 *  * 3 floor level
	 *
	 * As you see these are similar to current characteristics and will replace them in the future to save space.
	 * Every characteristic namely occupies a bit of RAM (governed by the SoftDevice, so not under our control).
	 */
	BLEpp::Characteristic<buffer_ptr_t>* _setConfigurationCharacteristic;
	
	/* Select configuration characteristic
	 *
	 * Just write an identifier to read subsequently from it using <_getConfigurationCharacteristic>. See for the
	 * possible values <_setConfigurationCharacteristic>.
	 */
	BLEpp::Characteristic<uint8_t>* _selectConfigurationCharacteristic;

	/* Get configuration characteristic
	 *
	 * You will have first to select a configuration before you can read from it. You write the identifiers also
	 * described in <_setConfigurationCharacteristic>.
	 *
	 * Then each of these returns a byte array, with e.g. a name, device type, room, etc.
	 */
	BLEpp::Characteristic<buffer_ptr_t>* _getConfigurationCharacteristic;

	/* Enable the temperature characteristic.
 	 */
	inline void addTemperatureCharacteristic();

	/* Enable the set configuration characteristic.
	 *
	 * The parameter given with onWrite should actually also already be within the space allocated within the
	 * characteristic.
	 * See <_setConfigurationCharacteristic>.
	 */
	inline void addSetConfigurationCharacteristic();

	/* Enable the set configuration characteristic.
	 *
	 * See <_selectConfigurationCharacteristic>.
	 */
	inline void addSelectConfigurationCharacteristic();

	/* Enable the get configuration characteristic.
	 */
	inline void addGetConfigurationCharacteristic();

	/* Enable the reset characteristic.
	 *
	 * The reset characteristic can be used to enter bootloader mode and update the firmware.
	 */
	inline void addResetCharacteristic();

	/* Enable the mesh characteristic.
	 */
	inline void addMeshCharacteristic();

	/* Retrieve the Bluetooth name from the object representing the BLE stack.
	 *
	 * @return name of the device
	 */
//	std::string&  getBLEName();
	/* Write the Bluetooth name to the object representing the BLE stack.
	 *
	 * This updates the Bluetooth name immediately, however, it does not update the name persistently. It
	 * has to be written to FLASH in that case.
	 */
//	void setBLEName(const std::string &name);

	/* Get a handle to the persistent storage struct and load it from FLASH.
	 *
	 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
	 * writing less than a minimal block strains the memory just as much as flashing the entire block.
	 * Hence, there is an entire struct that can be filled and flashed at once.
	 */
//	void loadPersistentStorage();

	/* Save to FLASH.
	 */
//	void savePersistentStorage();

private:

	// buffer object to read/write configuration characteristics
	StreamBuffer<uint8_t> *_streamBuffer;

	/* Select configuration for subsequent read actions on the get configuration characteristic.
	 */
	uint8_t _selectConfiguration;
};
