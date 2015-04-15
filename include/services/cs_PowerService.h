/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include "third/std/function.h"
#include <vector>

#include "characteristics/cs_CurrentLimit.h"
#include "characteristics/cs_UuidConfig.h"
#include "characteristics/cs_CurrentCurve.h"
#include "common/cs_Storage.h"
#include "cs_BluetoothLE.h"
#include "drivers/cs_ADC.h"
#include "drivers/cs_LPComp.h"
#include "drivers/cs_RTC.h"

#define POWER_SERVICE_UUID "5b8d0000-6f20-11e4-b116-123b93f75cba"

class PowerService : public BLEpp::GenericService {
public:
	typedef function<int8_t()> func_t;

	PowerService();

	/* Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void init();

	void addSpecificCharacteristics();

	void tick();
	
	void turnOff();
	
	void turnOn();
	
	void dim(uint8_t value);
protected:
	// Enabled characteristics (to be set in constructor)
	
	// The characteristics in this service
	void addPWMCharacteristic();
	void addSampleCurrentCharacteristic();
	void addCurrentCurveCharacteristic();
	void addCurrentConsumptionCharacteristic();
	void addCurrentLimitCharacteristic();

	// Some helper functions
	void sampleCurrentInit();
	void sampleCurrent(uint8_t type);

	uint8_t getCurrentLimit();

	void loadPersistentStorage();
	void savePersistentStorage();
private:
	// References to characteristics that need to be written from other functions
	BLEpp::Characteristic<uint8_t> *_pwmCharacteristic;
	BLEpp::Characteristic<uint8_t> *_sampleCurrentCharacteristic;
	BLEpp::Characteristic<uint16_t> *_currentConsumptionCharacteristic;
	BLEpp::Characteristic<uint8_t*> *_currentCurveCharacteristic;
	BLEpp::Characteristic<uint8_t> *_currentLimitCharacteristic;

	CurrentCurve<uint16_t>* _currentCurve;

	uint8_t _currentLimitVal;
	CurrentLimit _currentLimit;

	pstorage_handle_t _storageHandle;
	ps_power_service_t _storageStruct;

	bool _adcInitialized;
	bool _currentLimitInitialized;
};
