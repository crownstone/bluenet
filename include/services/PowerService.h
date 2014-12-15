/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef POWERSERVICE_H_
#define POWERSERVICE_H_

#include "BluetoothLE.h"
#include <util/function.h>

#include <drivers/nrf_rtc.h>
#include <drivers/nrf_adc.h>
#include <drivers/LPComp.h>
#include <common/storage.h>
#include <characteristics/charac_config.h>
#include <characteristics/CurrentLimit.h>
#include <vector>

#define POWER_SERVICE_UUID "5b8d0000-6f20-11e4-b116-123b93f75cba"

class PowerService : public BLEpp::GenericService {
public:
	typedef function<int8_t()> func_t;

	PowerService(BLEpp::Nrf51822BluetoothStack& stack);

	void addSpecificCharacteristics();

	static PowerService& createService(BLEpp::Nrf51822BluetoothStack& stack);
	
	void loop();
protected:
	// Enabled characteristics (to be set in constructor)
	
	// The characteristics in this service
	void addPWMCharacteristic();
	void addVoltageCurveCharacteristic();
	void addPowerConsumptionCharacteristic();
	void addCurrentLimitCharacteristic();

	// Some helper functions
	void sampleAdcInit();
	void sampleAdcStart();

	uint8_t getCurrentLimit();

	void TurnOff();
	void TurnOn();
	void Dim(uint8_t value);

	void loadPersistentStorage();
	void savePersistentStorage();
private:
	// References to stack, to e.g. stop advertising if required
	BLEpp::Nrf51822BluetoothStack* _stack;

	// References to characteristics that need to be written from other functions
	BLEpp::Characteristic<uint8_t> *_currentLimitCharacteristic;

	// Current limit
	uint8_t _current_limit_val;

	CurrentLimit _currentLimit;

	pstorage_handle_t _storageHandle;
	ps_power_service_t _storageStruct;
};

#endif /* POWERSERVICE_H_ */
