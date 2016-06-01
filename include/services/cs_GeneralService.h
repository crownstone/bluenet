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
#include <structs/buffer/cs_CharacBuffer.h>

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

	/** Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things. Urgent matters have to be
	 * resolved immediately in interrupt service handlers. The temperature for example is updated every
	 * tick, because timing is not important for this at all.
	 */
//	void tick();

//	void scheduleNextTick();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

protected:
	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void init();

	/** Enable the command characteristic.
 	 */
	inline void addControlCharacteristic(buffer_ptr_t buffer, uint16_t size);

	/** Enable the temperature characteristic.
 	 */
	inline void addTemperatureCharacteristic();

	/** Enable the set configuration characteristic.
	 *
	 * The parameter given with onWrite should actually also already be within the space allocated within the
	 * characteristic.
	 * See <_setConfigurationCharacteristic>.
	 */
	inline void addSetConfigurationCharacteristic(buffer_ptr_t buffer, uint16_t size);

	/** Enable the get configuration characteristic.
	 */
	inline void addGetConfigurationCharacteristic(buffer_ptr_t buffer, uint16_t size);

	inline void addSelectStateVarCharacteristic(buffer_ptr_t buffer, uint16_t size);
	inline void addReadStateVarCharacteristic(buffer_ptr_t buffer, uint16_t size);

	/** Enable the reset characteristic.
	 *
	 * The reset characteristic can be used to enter bootloader mode and update the firmware.
	 */
	inline void addResetCharacteristic();

	/** Enable the mesh characteristic.
	 */
	inline void addMeshCharacteristic();
	inline void removeMeshCharacteristic();

	CharacBuffer<uint8_t>* getCharacBuffer(buffer_ptr_t& buffer, uint16_t& maxLength);

private:

	BLEpp::Characteristic<buffer_ptr_t>* _controlCharacteristic;

	/** Temperature characteristic
	 */
	BLEpp::Characteristic<int32_t>* _temperatureCharacteristic;

	/** Reset characteristic
	 *
	 * Resets device
	 */
	BLEpp::Characteristic<int32_t>* _resetCharacteristic;

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
//	BLEpp::Characteristic<uint8_t>* _selectConfigurationCharacteristic;

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
	CharacBuffer<uint8_t> *_streamBuffer;

	MeshCharacteristicMessage* _meshMessage;

};
