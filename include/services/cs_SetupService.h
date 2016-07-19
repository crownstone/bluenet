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
#include <services/cs_CrownstoneService.h>

//#define GENERAL_SERVICE_UPDATE_FREQUENCY 10 //! hz

/** General Service for the Crownstone
 *
 * There are several characteristics that fit into the general service description. There is a characteristic
 * that measures the temperature, there are several characteristics that defines the crownstone, namely by
 * name, by type, or by location (room), and there is a characteristic to update its firmware.
 *
 * If meshing is enabled, it is also possible to send a message into the mesh network using a characteristic.
 */
class SetupService: public CrownstoneService {
public:
	/** Constructor for general crownstone service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store name and other information that is
	 * set over so-called configuration characteristics. It also initializes all characteristics.
	 */
	SetupService();

	/** Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things. Urgent matters have to be
	 * resolved immediately in interrupt service handlers. The temperature for example is updated every
	 * tick, because timing is not important for this at all.
	 */
//	void tick();

//	void scheduleNextTick();

//	void handleEvent(uint16_t evt, void* p_data, uint16_t length) {};

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void createCharacteristics();

protected:

	/** Enable the command characteristic.
 	 */
//	inline void addControlCharacteristic(buffer_ptr_t buffer, uint16_t size);

	/** Enable the command characteristic.
 	 */
	inline void addMacAddressCharacteristic();

	/** Enable the set configuration characteristic.
	 *
	 * The parameter given with onWrite should actually also already be within the space allocated within the
	 * characteristic.
	 * See <_setConfigurationCharacteristic>.
	 */
//	inline void addSetConfigurationCharacteristic(buffer_ptr_t buffer, uint16_t size);

	/** Enable the get configuration characteristic.
	 */
//	inline void addGetConfigurationCharacteristic(buffer_ptr_t buffer, uint16_t size);

	/** Enable the reset characteristic.
	 *
	 * The reset characteristic can be used to enter bootloader mode and update the firmware.
	 */
//	inline void addResetCharacteristic();

//	StreamBuffer<uint8_t>* getStreamBuffer(buffer_ptr_t& buffer, uint16_t& maxLength);

private:

	// stores the MAC address of the devices to be used for mesh message handling
    ble_gap_addr_t _myAddr;

//	BLEpp::Characteristic<buffer_ptr_t>* _controlCharacteristic;

	BLEpp::Characteristic<buffer_ptr_t>* _macAddressCharacteristic;

	/** Reset characteristic
	 *
	 * Resets device
	 */
//	BLEpp::Characteristic<int32_t>* _resetCharacteristic;

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
//	BLEpp::Characteristic<buffer_ptr_t>* _setConfigurationCharacteristic;
	
	/** Get configuration characteristic
	 *
	 * You will have first to select a configuration before you can read from it. You write the identifiers also
	 * described in <_setConfigurationCharacteristic>.
	 *
	 * Then each of these returns a byte array, with e.g. a name, device type, room, etc.
	 */
//	BLEpp::Characteristic<buffer_ptr_t>* _getConfigurationCharacteristic;

	//! buffer object to read/write configuration characteristics
//	StreamBuffer<uint8_t> *_streamBuffer;

};
