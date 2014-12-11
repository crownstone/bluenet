/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */


//#include <cmath> // try not to use this!
#include <cstdio>

#include <services/PowerService.h>
#include <common/config.h>
#include <common/boards.h>
#include <drivers/nrf_rtc.h>

#include "nRF51822.h"

using namespace BLEpp;

PowerService::PowerService(Nrf51822BluetoothStack& _stack) :
		_stack(&_stack), _currentLimitCharacteristic(NULL)
//		, _current_limit_val(0)
{

	setUUID(UUID(POWER_SERVICE_UUID));
	//setUUID(UUID(0x3800)); // there is no BLE_UUID for indoor localization (yet)
	
	LOGi("Create power service");

	characStatus.reserve(4);
	characStatus.push_back( { "PWM",
			PWM_UUID,
			true,
			static_cast<addCharacteristicFunc>(&PowerService::addPWMCharacteristic)});
	characStatus.push_back( { "Voltage Curve",
			VOLTAGE_CURVE_UUID,
			false,
			static_cast<addCharacteristicFunc>(&PowerService::addVoltageCurveCharacteristic)});
	characStatus.push_back( { "Power Consumption",
			POWER_CONSUMPTION_UUID,
			true,
			static_cast<addCharacteristicFunc>(&PowerService::addPowerConsumptionCharacteristic)});
	characStatus.push_back( { "Current Limit",
			CURRENT_LIMIT_UUID,
			true,
			static_cast<addCharacteristicFunc>(&PowerService::addCurrentLimitCharacteristic)});

	Storage::getInstance().getHandle(PS_ID_POWER_SERVICE, _storageHandle);
	loadPersistentStorage();

	// we have to figure out why this goes wrong
//	setName(std::string("Power Service"));

//	// set timer with compare interrupt every 10ms
//	timer_config(10);
}

void PowerService::loadPersistentStorage() {
	Storage::getInstance().getStruct(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void PowerService::savePersistentStorage() {
	Storage::getInstance().setStruct(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void PowerService::addPWMCharacteristic() {
	createCharacteristic<uint8_t>()
		.setUUID(UUID(getUUID(), PWM_UUID))
		.setName("PWM")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t& value) -> void {
//			LOGi("set pwm to %i", value);
			nrf_pwm_set_value(0, value);
		});
}

// Do we really want to use the PWM for this, or just set the pin to zero?
// TODO: turn off normally, but make sure we enable the completely PWM again on request
void PowerService::TurnOff() {
	nrf_pwm_set_value(0, 0);
}

// Do we really want to use the PWM for this, or just set the pin to zero?
// TODO: turn on normally, but make sure we enable the completely PWM again on request
void PowerService::TurnOn() {
	nrf_pwm_set_value(0, (uint8_t)-1);
}

/**
 * Dim the light, note that we use PWM. You might need another way to dim the light! For example by only turning on for
 * a specific duty-cycle after the detection of a zero crossing.
 */
void PowerService::Dim(uint8_t value) {
	nrf_pwm_set_value(0, value);
}


void PowerService::addVoltageCurveCharacteristic() {
	createCharacteristic<uint8_t>()
		.setUUID(UUID(getUUID(), VOLTAGE_CURVE_UUID))
		.setName("Voltage Curve")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t& value) -> void {
//			LOGi("start adc sampling");

			sampleAdcInit();

//			nrf_pwm_set_value(0, value);

			sampleAdcStart();

//			LOGd("Successfully written");
		});
}

void PowerService::addPowerConsumptionCharacteristic() {
//	LOGd("create characteristic to read power consumption");
	createCharacteristic<uint32_t>()
		.setUUID(UUID(getUUID(), POWER_CONSUMPTION_UUID))
		.setName("Power consumption")
		.setDefaultValue(0)
		.setWritable(false)
		.setNotifies(true);
}

uint16_t PowerService::getCurrentLimit() {
	loadPersistentStorage();
	LOGi("Obtained current limit from FLASH: %i", _storageStruct.current_limit);
	return _storageStruct.current_limit;
}

/**
 * The characteristic that writes a current limit to persistent memory.
 *
 * TODO: Check https://devzone.nordicsemi.com/question/1745/how-to-handle-flashwrit-in-an-safe-way/ 
 *       Writing to persistent memory should be done between connection/advertisement events...
 */
void PowerService::addCurrentLimitCharacteristic() {
	_currentLimitCharacteristic = createCharacteristicRef<uint16_t>();
	(*_currentLimitCharacteristic)
		.setNotifies(true)
		.setUUID(UUID(getUUID(), CURRENT_LIMIT_UUID))
		.setName("Current Limit")
		.setDefaultValue(getCurrentLimit())
		.setWritable(true)
		.onWrite([&](const uint16_t &value) -> void {
			LOGi("Set current limit to: %i", value);
			_storageStruct.current_limit = value;
			LOGi("Write value to persistent memory");
			savePersistentStorage();

			LPComp::getInstance().stop();
			// There are only 6 levels...
			if (_storageStruct.current_limit > 6)
				_storageStruct.current_limit = 6;
			LPComp::getInstance().config(PIN_LPCOMP, _storageStruct.current_limit, LPComp::UP);
			LPComp::getInstance().start();
		});

	// There are only 6 levels...
	if (_storageStruct.current_limit > 6)
		_storageStruct.current_limit = 6;
	LPComp::getInstance().config(PIN_LPCOMP, _storageStruct.current_limit, LPComp::UP);
	LPComp::getInstance().start();
	_currentLimit.init();
}

static int tmp_cnt = 100;
static int loop_cnt = 100;

/**
 * TODO: We should only need to do this once on startup.
 */
void PowerService::loop() {
	LPComp::getInstance().tick();
	// check if current is not beyond current_limit if the latter is set
	if (++tmp_cnt > loop_cnt) {
		if (_currentLimitCharacteristic) {
			getCurrentLimit();
			LOGi("Set default current limit value to %i", _storageStruct.current_limit);
			*_currentLimitCharacteristic = _storageStruct.current_limit;
		}
		tmp_cnt = 0;
	}
}

void PowerService::sampleAdcInit() {
	/*
				uint64_t rms_sum = 0;
				uint32_t voltage_min = 0xffffffff;
				uint32_t voltage_max = 0;
				// start reading adc
				uint32_t voltage;
				uint32_t samples = 100000;
				//uint32_t subsample = samples / curve_size;
	*/
				//LOGi("Stop advertising");
				//stack->stopAdvertising();

				LOGi("start RTC");
				RealTimeClock::getInstance().init();
				RealTimeClock::getInstance().start();

				// Wait for the RTC to actually start
				nrf_delay_us(100);

				LOGi("Start ADC");
				ADC::getInstance().nrf_adc_start();
				// replace by timer!

}

void PowerService::sampleAdcStart() {
	while (!ADC::getInstance().getBuffer()->full()) {
		nrf_delay_ms(100);
	}
	LOGi("Number of results: %u", ADC::getInstance().getBuffer()->count()/2);
	LOGi("Counter is at: %u", RealTimeClock::getInstance().getCount());

	LOGi("Stop ADC converter");
	ADC::getInstance().nrf_adc_stop();

	// Wait for the ADC to actually stop
	nrf_delay_us(1000);

	LOGi("Stop RTC");
	RealTimeClock::getInstance().stop();
/*
	for (uint32_t i=0; i<samples; ++i) {

		nrf_adc_read(PIN_ADC, &voltage);

		rms_sum += voltage*voltage;
		if (voltage < voltage_min)
			voltage_min = voltage;
		if (voltage > voltage_max)
			voltage_max = voltage;
		//if (!(i % subsample))
		if (i < curve_size)
			curve[i] = voltage;
	}
	uint32_t rms = sqrt(rms_sum/(100*1000));

	// 8a max --> 0.96v max --> voltage value is max 960000, which is smaller than 2^32

	// measured voltage goes from 0-3.6v(due to 1/3 multiplier), measured as 0-255(8bit) or 0-1024(10bit)
//			voltage = voltage*1000*1200*3/255; // nv   8 bit
	voltage     = voltage    *1000*1200*3/1024; // nv   10 bit
	voltage_min = voltage_min*1000*1200*3/1024;
	voltage_max = voltage_max*1000*1200*3/1024;
	rms         = rms        *1000*1200*3/1024;

	uint16_t current = rms / SHUNT_VALUE; // ma

	LOGd("voltage(nV): last=%lu", voltage);
       	//rms=%lu min=%lu max=%lu current=%i mA", voltage, rms, voltage_min, voltage_max, current);
*/

	int i = 0;
	while (!ADC::getInstance().getBuffer()->empty()) {
		_log(INFO, "%u, ", ADC::getInstance().getBuffer()->pop());
		if (!(++i % 10)) {
			_log(INFO, "\r\n");
		}
	}
	_log(INFO, "\r\n");
	//stack->startAdvertising(); // segfault
/*
	uint64_t result = voltage_min;
	result <<= 32;
	result |= voltage_max;
#ifdef NUMBER_CHARAC
	*intchar = result;
	result = rms;
	result <<= 32;
	result |= current;
	*intchar2 = result;
#endif
*/
}

PowerService& PowerService::createService(Nrf51822BluetoothStack& stack) {
//	LOGd("Create power service");
	PowerService* svc = new PowerService(stack);
	stack.addService(svc);
	svc->GenericService::addSpecificCharacteristics();
	return *svc;
}

