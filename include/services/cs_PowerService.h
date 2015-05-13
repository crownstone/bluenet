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
#include "structs/cs_CurrentCurve.h"
#include "drivers/cs_Storage.h"
//#include "ble/cs_BluetoothLE.h"
#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
//#include "drivers/cs_ADC.h"
#include "drivers/cs_LPComp.h"
//#include "drivers/cs_RTC.h"

#define POWER_SERVICE_UPDATE_FREQUENCY 10 // hz

class PowerService : public BLEpp::Service {
public:
//	typedef function<int8_t()> func_t;

	/* Constructor for power service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store current limit.
	 * It also initializes all characteristics.
	 */
	PowerService();

	/* Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void init();

	/* Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things.
	 * Urgent matters have to be resolved immediately in interrupt service handlers.
	 */
	void tick();
	
	void scheduleNextTick();

	/* Switch off the light
	 */
	void turnOff();
	
	/* Switch on the light
	 */
	void turnOn();
	
	/* Dim the light by using PWM.
	 *
	 * You might need another way to dim the light! For example by only turning on for
	 * a specific duty-cycle after the detection of a zero crossing.
	 */
	void dim(uint8_t value);
protected:
	// The characteristics in this service
	void addPWMCharacteristic();
	void addSampleCurrentCharacteristic();
	void addCurrentCurveCharacteristic();
	void addCurrentConsumptionCharacteristic();
	void addCurrentLimitCharacteristic();

	/* Initializes and starts the ADC
	 */
	void sampleCurrentInit();

	/* Fill up the current curve and send it out over bluetooth
	 * @type specifies over which characteristic the current curve should be sent.
	 */
	void sampleCurrent(uint8_t type);

	/* Get the stored current limit.
	 */
	uint8_t getCurrentLimit();
	void setCurrentLimit(uint8_t value);

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
	// References to characteristics that need to be written from other functions
	BLEpp::Characteristic<uint8_t> *_pwmCharacteristic;
	BLEpp::Characteristic<uint8_t> *_sampleCurrentCharacteristic;
	BLEpp::Characteristic<uint16_t> *_currentConsumptionCharacteristic;
	BLEpp::Characteristic<uint8_t*> *_currentCurveCharacteristic;
	BLEpp::Characteristic<uint8_t> *_currentLimitCharacteristic;

	CurrentCurve<uint16_t>* _currentCurve;

	uint8_t _currentLimitVal;

//	pstorage_handle_t _storageHandle;
//	ps_power_service_t _storageStruct;

	bool _adcInitialized;
	bool _currentLimitInitialized;
	uint8_t _samplingType;
};
