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
#include <structs/cs_MeshCommand.h>

#define CROWNSTONE_SERVICE_UPDATE_FREQUENCY 10 //! hz

/** General Service for the Crownstone
 *
 * There are several characteristics that fit into the general service description. There is a characteristic
 * that measures the temperature, there are several characteristics that defines the crownstone, namely by
 * name, by type, or by location (room), and there is a characteristic to update its firmware.
 *
 * If meshing is enabled, it is also possible to send a message into the mesh network using a characteristic.
 */
class CrownstoneService: public BLEpp::Service, EventListener {
public:
	/** Constructor for general crownstone service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store name and other information that is
	 * set over so-called configuration characteristics. It also initializes all characteristics.
	 */
	 CrownstoneService();

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
	/** Initialize a CrownstoneService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	virtual void createCharacteristics();

	/** Enable the command characteristic.
 	 */
	void addControlCharacteristic(buffer_ptr_t buffer, uint16_t size);

	/** Enable the set configuration characteristic.
	 *
	 * The parameter given with onWrite should actually also already be within the space allocated within the
	 * characteristic.
	 * See <_setConfigurationCharacteristic>.
	 */
	void addConfigurationControlCharacteristic(buffer_ptr_t buffer, uint16_t size, EncryptionAccessLevel minimumAccessLevel = ADMIN);

	/** Enable the get configuration characteristic.
	 */
	void addConfigurationReadCharacteristic(buffer_ptr_t buffer, uint16_t size, EncryptionAccessLevel minimumAccessLevel = ADMIN);

	inline void addStateControlCharacteristic(buffer_ptr_t buffer, uint16_t size);
	inline void addStateReadCharacteristic(buffer_ptr_t buffer, uint16_t size);
	inline void addSessionNonceCharacteristic(buffer_ptr_t buffer, uint16_t size);

	/** Enable the mesh characteristic.
	 */
	inline void addMeshCharacteristic();

	StreamBuffer<uint8_t>* getStreamBuffer(buffer_ptr_t& buffer, uint16_t& maxLength);

protected:
	BLEpp::Characteristic<buffer_ptr_t>* _controlCharacteristic;

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
	BLEpp::Characteristic<buffer_ptr_t>* _configurationControlCharacteristic;

	/** Get configuration characteristic
	 *
	 * You will have first to select a configuration before you can read from it. You write the identifiers also
	 * described in <_setConfigurationCharacteristic>.
	 *
	 * Then each of these returns a byte array, with e.g. a name, device type, room, etc.
	 */
	BLEpp::Characteristic<buffer_ptr_t>* _configurationReadCharacteristic;

	//! buffer object to read/write configuration characteristics
	StreamBuffer<uint8_t> *_streamBuffer;

private:

	/** Mesh characteristic
	 *
	 * Sends a message over the mesh network
	 */
	BLEpp::Characteristic<buffer_ptr_t>* _meshControlCharacteristic;
	MeshCommand* _meshCommand;

	BLEpp::Characteristic<buffer_ptr_t>* _stateControlCharacteristic;
	BLEpp::Characteristic<buffer_ptr_t>* _stateReadCharacteristic;
	BLEpp::Characteristic<buffer_ptr_t>* _sessionNonceCharacteristic;


//	StreamBuffer<uint8_t, MAX_MESH_MESSAGE_PAYLOAD_LENGTH>* _meshCommand;

};
