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

#include <drivers/nrf_adc.h>
#include <common/storage.h>
#include <vector>

#define POWER_SERVICE_UUID "5b8d7800-6f20-11e4-b116-123b93f75cba"

//enum CharacteristicUUIDs = {
enum {
	PWM_UUID                                 = 0x1,
	VOLTAGE_CURVE_UUID                       = 0x2,
	POWER_CONSUMPTION_UUID                   = 0x3,
	CURRENT_LIMIT_UUID                       = 0x4,
	CharacteristicUUIDCount                  = 4
};

struct CharacteristicStatus {
	uint8_t UUID;
	bool enabled;
};
typedef struct CharacteristicStatus CharacteristicStatusT;

class PowerService : public BLEpp::GenericService {
public:
	typedef function<int8_t()> func_t;

	PowerService(BLEpp::Nrf51822BluetoothStack& stack, ADC &adc, Storage &storage);

	void addSpecificCharacteristics();

	static PowerService& createService(BLEpp::Nrf51822BluetoothStack& stack, ADC &adc, Storage &storage);
	
	void loop();
protected:
	// Array of enabled characteristics (to be set in constructor)
	std::vector<CharacteristicStatusT> characStatus;
/*
	int enabledCharacteristicsCount;
	int *enabledCharacteristics;
	int disabledCharacteristicsCount;
	int *disabledCharacteristics;
*/
	// The characteristics in this service
	void addPWMCharacteristic();
	void addVoltageCurveCharacteristic();
	void addPowerConsumptionCharacteristic();
	void addCurrentLimitCharacteristic();

	// Some helper functions
	void sampleAdcInit();
	void sampleAdcStart();

	uint16_t getCurrentLimit();

	// References to characteristics that need to be written from other functions
	BLEpp::Characteristic<uint16_t> *_currentLimitCharacteristic;
private:
	// References to stack, to e.g. stop advertising if required
	BLEpp::Nrf51822BluetoothStack* _stack;

	// We need an AD converter for this service
	ADC &_adc;

	// We need persistent storage for this service
	Storage &_storage;

	// Current limit
	uint16_t _current_limit;
};

#endif /* POWERSERVICE_H_ */
