/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Characteristic.h>
#include <ble/cs_Service.h>
#include <drivers/cs_LPComp.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventListener.h>
#include <structs/buffer/cs_CircularBuffer.h>
#include <structs/buffer/cs_DifferentialBuffer.h>
#include <structs/buffer/cs_StackBuffer.h>
#include <structs/cs_PowerSamples.h>

//! POWER_SERVICE_UPDATE_FREQUENCY is in Herz.
#define POWER_SERVICE_UPDATE_FREQUENCY 10 

/** PowerService controls the relays on the Crownstone.
 *
 * The PowerService has the following functionalities:
 *   - toggle the relay and hereby switch on/off devices
 *   - control the IGBT and hereby dim devices through pulse width modulation (PWM)
 *   - measure current
 *   - measure power consumption
 *   - set a limit on the maximum current (the relay will be automatically switched off above this threshold)
 */
class PowerService : public Service, EventListener {
public:
	/** Constructor for power service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store current limit.
	 * It also initializes all characteristics.
	 */
	PowerService();

	/** Destructor for power service
	 */
	virtual ~PowerService();

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void createCharacteristics();

	/** Remove all characteristics
	 *
	 * Remove and deallocate all characteristics
	 */
	void removeCharacteristics();

	/** Function handler for incoming events.
	 */
	void handleEvent(event_t & event);

protected:

	//! The dimming characteristic
	void addPWMCharacteristic();
	
	//! The relay characteristic
	void addRelayCharacteristic();

	//! The power samples characteristic
	void addPowerSamplesCharacteristic();
	
	//! The power consumption characteristic
	void addPowerConsumptionCharacteristic();

private:
	//! References to characteristics that need to be written from other functions
	Characteristic<uint8_t> *_pwmCharacteristic;
	Characteristic<uint8_t> *_relayCharacteristic;
	Characteristic<int32_t> *_powerConsumptionCharacteristic;
	Characteristic<uint8_t*> *_powerSamplesCharacteristic;

};
