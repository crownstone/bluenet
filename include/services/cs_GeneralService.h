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

#define GENERAL_SERVICE_UPDATE_FREQUENCY 10 //! hz

/** General Service for the Crownstone
 *
 * There are several characteristics that fit into the general service description. There is a characteristic
 * that measures the temperature, there are several characteristics that defines the crownstone, namely by
 * name, by type, or by location (room), and there is a characteristic to update its firmware.
 *
 * If meshing is enabled, it is also possible to send a message into the mesh network using a characteristic.
 */
class GeneralService: public Service, EventListener {
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

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void createCharacteristics();

protected:
	/** Enable the temperature characteristic.
 	 */
	inline void addTemperatureCharacteristic();

	/** Enable the reset characteristic.
	 *
	 * The reset characteristic can be used to enter bootloader mode and update the firmware.
	 */
	inline void addResetCharacteristic();

private:

	/** Temperature characteristic
	 */
	Characteristic<int32_t>* _temperatureCharacteristic;

	/** Reset characteristic
	 *
	 * Resets device
	 */
	Characteristic<uint8_t>* _resetCharacteristic;
};
