/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

//#include "third/std/function.h"
//#include <vector>

//#include <common/cs_Types.h>

//#include "characteristics/cs_UuidConfig.h"
#include "drivers/cs_Storage.h"
#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include "drivers/cs_LPComp.h"
#include "structs/buffer/cs_CircularBuffer.h"
#include "structs/buffer/cs_StackBuffer.h"
#include "structs/buffer/cs_DifferentialBuffer.h"
#include "structs/cs_PowerSamples.h"
//#include "protocol/cs_MeshMessageTypes.h"

#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include <events/cs_EventListener.h>

#define POWER_SERVICE_UPDATE_FREQUENCY 10 //! hz

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

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void createCharacteristics();

	/** Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things.
	 * Urgent matters have to be resolved immediately in interrupt service handlers.
	 */
//	void tick();

//	void scheduleNextTick();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

protected:
	//! The characteristics in this service
	void addPWMCharacteristic();
	void addRelayCharacteristic();
	void addPowerSamplesCharacteristic();
	void addPowerConsumptionCharacteristic();
//	void addCurrentLimitCharacteristic();

private:
	//! References to characteristics that need to be written from other functions
	Characteristic<uint8_t> *_pwmCharacteristic;
	Characteristic<uint8_t> *_relayCharacteristic;
	Characteristic<int32_t> *_powerConsumptionCharacteristic;
	Characteristic<uint8_t*> *_powerSamplesCharacteristic;

};
