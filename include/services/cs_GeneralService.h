/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <vector>

#include "characteristics/cs_StreamBuffer.h"
#include "characteristics/cs_MeshMessage.h"
#include "characteristics/cs_UuidConfig.h"
#include "common/cs_Storage.h"
#include "cs_BluetoothLE.h"
#include "processing/cs_Temperature.h"

#define GENERAL_UUID "f5f90000-59f9-11e4-aa15-123b93f75cba"

/* General Service for the Crownstone
 *
 * There are several characteristics that fit into the general service description. There is a characteristic
 * that measures the temperature, there are several characteristics that defines the crownstone, namely by
 * name, by type, or by location (room), and there is a characteristic to update its firmware.
 *
 * If meshing is enabled, it is also possible to send a message into the mesh network using a characteristic.
 */
class GeneralService: public BLEpp::GenericService {
public:
	GeneralService(BLEpp::Nrf51822BluetoothStack &stack);

	/* Update the temperature characteristic.
	*/
	void writeToTemperatureCharac(int32_t temperature);

	/* Update the configuration characteristic.
	 */
	StreamBuffer<uint8_t>* readFromStorage(uint8_t type);

	void writeToConfigCharac(StreamBuffer<uint8_t>& buffer);

	/* Read configuration written by user.
	 */
	void writeToStorage(uint8_t type, uint8_t length, uint8_t* payload);

	/* Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things. Urgent matters have to be 
	 * resolved immediately in interrupt service handlers. The temperature for example is updated every 
	 * tick, because timing is not important for this at all.
	 */
	void tick();

	/** Helper function to generate a GeneralService object
	*/
	static GeneralService& createService(BLEpp::Nrf51822BluetoothStack& stack);

protected:
	BLEpp::Nrf51822BluetoothStack* _stack;

	// References to characteristics
	BLEpp::CharacteristicT<int32_t>* _temperatureCharacteristic;
	BLEpp::Characteristic<int32_t>* _firmwareCharacteristic;
	BLEpp::Characteristic<MeshMessage>* _meshCharacteristic;

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
	BLEpp::Characteristic<StreamBuffer<uint8_t>>* _setConfigurationCharacteristic;
	
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
	BLEpp::Characteristic<StreamBuffer<uint8_t>>* _getConfigurationCharacteristic;

	/* Enable the temperature characteristic.
 	 */
	void addTemperatureCharacteristic();
	/* Enable the set configuration characteristic.
	 *
	 * See <_setConfigurationCharacteristic>.
	 */
	void addSetConfigurationCharacteristic();
	/* Enable the set configuration characteristic.
	 *
	 * See <_selectConfigurationCharacteristic>.
	 */
	void addSelectConfigurationCharacteristic();
	/* Enable the get configuration characteristic.
	 */
	void addGetConfigurationCharacteristic();
	/* Enable the firmware upgrade characteristic.
	 */
	void addFirmwareCharacteristic();
	/* Enable the mesh characteristic.
	 */
	void addMeshCharacteristic();

	/* Retrieve the Bluetooth name from the object representing the BLE stack.
	*/
	std::string&  getBLEName();
	/* Write the Bluetooth name to the object representing the BLE stack.
	 *
	 * This updates the Bluetooth name immediately, however, it does not update the name persistently. It 
	 * has to be written to FLASH in that case.
	 */
	void setBLEName(const std::string &name);

	/* Get a handle to the persistent storage struct and load it from FLASH.
	 *
	 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that 
	 * writing less than a minimal block strains the memory just as much as flashing the entire block. 
	 * Hence, there is an entire struct that can be filled and flashed at once.
	 */
	void loadPersistentStorage();

	/* Save to FLASH.
	*/
	void savePersistentStorage();
private:
	std::string _name;
	std::string _room;
	std::string _type;

	pstorage_handle_t _storageHandle;
	ps_general_service_t _storageStruct;

	/* Select configuration for subsequent read actions on the get configuration characteristic.
	 */
	uint8_t _selectConfiguration;
};
