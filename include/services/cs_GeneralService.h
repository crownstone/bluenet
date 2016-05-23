/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <ble/cs_Nordic.h>

#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include <events/cs_EventListener.h>
#include <structs/cs_MeshCharacteristicMessage.h>
#include <structs/buffer/cs_StreamBuffer.h>

#define GENERAL_SERVICE_UPDATE_FREQUENCY 10 //! hz

/** General Service for the Crownstone
 *
 * There are several characteristics that fit into the general service description. There is a characteristic
 * that measures the temperature, there are several characteristics that defines the crownstone, namely by
 * name, by type, or by location (room), and there is a characteristic to update its firmware.
 *
 * If meshing is enabled, it is also possible to send a message into the mesh network using a characteristic.
 */
class GeneralService: public BLEpp::Service, EventListener {
public:
	/** Constructor for general crownstone service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store name and other information that is
	 * set over so-called configuration characteristics. It also initializes all characteristics.
	 */
	 GeneralService();

	/** Update the temperature characteristic.
	 * @temperature A value in Celcius directly from the chip
	 *
	 * This writes a value to the temperature characteristic which can subsequently read out by the user. If we write
	 * often to this characteristic this value will be updated.
	 */
	void writeToTemperatureCharac(int32_t temperature);

	/** Write to the "get configuration" characteristic
	 *
	 * Writing is done by setting the data length properly and notifying the characteristic (and hence the softdevice)
	 * that there is a new value available.
	 */
	void writeToConfigCharac();

	/** Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things. Urgent matters have to be
	 * resolved immediately in interrupt service handlers. The temperature for example is updated every
	 * tick, because timing is not important for this at all.
	 */
	void tick();

	void scheduleNextTick();

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void init();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

private:

	inline void addCommandCharacteristic();

	/** Enable the temperature characteristic.
 	 */
	inline void addTemperatureCharacteristic();

	/** Enable the set configuration characteristic.
	 *
	 * The parameter given with onWrite should actually also already be within the space allocated within the
	 * characteristic.
	 * See <_setConfigurationCharacteristic>.
	 */
	inline void addSetConfigurationCharacteristic();

	/** Enable the set configuration characteristic.
	 *
	 * See <_selectConfigurationCharacteristic>.
	 */
	inline void addSelectConfigurationCharacteristic();

	/** Enable the get configuration characteristic.
	 */
	inline void addGetConfigurationCharacteristic();

	inline void addSelectStateVarCharacteristic();
	inline void addReadStateVarCharacteristic();

	/** Enable the reset characteristic.
	 *
	 * The reset characteristic can be used to enter bootloader mode and update the firmware.
	 */
	inline void addResetCharacteristic();

	/** Enable the mesh characteristic.
	 */
	inline void addMeshCharacteristic();
	inline void removeMeshCharacteristic();

	/** Temperature characteristic
	 */
	BLEpp::Characteristic<int32_t>* _temperatureCharacteristic;

	/** Reset characteristic
	 *
	 * Resets device
	 */
	BLEpp::Characteristic<int32_t>* _resetCharacteristic;

	BLEpp::Characteristic<buffer_ptr_t>* _commandCharacteristic;

	/** Mesh characteristic
	 *
	 * Sends a message over the mesh network
	 */
	BLEpp::Characteristic<buffer_ptr_t>* _meshCharacteristic;

	/** Set configuration characteristic
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
	
	/** Select configuration characteristic
	 *
	 * Just write an identifier to read subsequently from it using <_getConfigurationCharacteristic>. See for the
	 * possible values <_setConfigurationCharacteristic>.
	 */
	BLEpp::Characteristic<uint8_t>* _selectConfigurationCharacteristic;

	/** Get configuration characteristic
	 *
	 * You will have first to select a configuration before you can read from it. You write the identifiers also
	 * described in <_setConfigurationCharacteristic>.
	 *
	 * Then each of these returns a byte array, with e.g. a name, device type, room, etc.
	 */
	BLEpp::Characteristic<buffer_ptr_t>* _getConfigurationCharacteristic;

	BLEpp::Characteristic<buffer_ptr_t>* _selectStateVarCharacteristic;
	BLEpp::Characteristic<buffer_ptr_t>* _readStateVarCharacteristic;

	//! buffer object to read/write configuration characteristics
	StreamBuffer<uint8_t> *_streamBuffer;

	/** Select configuration for subsequent read actions on the get configuration characteristic.
	 */
//	uint8_t _selectConfiguration;

	MeshCharacteristicMessage* _meshMessage;
};
