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

PowerService::PowerService(Nrf51822BluetoothStack& _stack, ADC &adc, Storage &storage, RealTimeClock &clock, LPComp &lpcomp) :
		_stack(&_stack), _adc(adc), _storage(storage), _clock(&clock), _current_limit_val(0), _lpcomp(lpcomp),
		_currentLimit(lpcomp), _currentLimitCharacteristic(NULL) {

	setUUID(UUID(POWER_SERVICE_UUID));
	//setUUID(UUID(0x3800)); // there is no BLE_UUID for indoor localization (yet)
	
	LOGi("Create power service");

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
			false,
			static_cast<addCharacteristicFunc>(&PowerService::addPowerConsumptionCharacteristic)});
	characStatus.push_back( { "Current Limit",
			CURRENT_LIMIT_UUID,
			true,
			static_cast<addCharacteristicFunc>(&PowerService::addCurrentLimitCharacteristic)});

	// we have to figure out why this goes wrong
//	setName(std::string("Power Service"));

//	// set timer with compare interrupt every 10ms
//	timer_config(10);
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
	_storage.getUint16(PS_CURRENT_LIMIT, &_current_limit_val);
	LOGi("Obtained current limit from FLASH: %i", _current_limit_val);
	return _current_limit_val;
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
			_current_limit_val = value;
			LOGi("Write value to persistent memory");
			_storage.setUint16(PS_CURRENT_LIMIT, _current_limit_val);

			_lpcomp.stop();
			// There are only 6 levels...
			if (_current_limit_val > 6)
				_current_limit_val = 6;
			_lpcomp.config(PIN_LPCOMP, _current_limit_val, LPComp::UP);
			_lpcomp.start();
		});

	// There are only 6 levels...
	if (_current_limit_val > 6)
		_current_limit_val = 6;
	_lpcomp.config(PIN_LPCOMP, _current_limit_val, LPComp::UP);
	_lpcomp.start();
	_currentLimit.init();
}

static int tmp_cnt = 100;
static int loop_cnt = 100;

/**
 * TODO: We should only need to do this once on startup.
 */
void PowerService::loop() {
	_lpcomp.tick();
	// check if current is not beyond current_limit if the latter is set
	if (++tmp_cnt > loop_cnt) {
		if (_currentLimitCharacteristic) {
			getCurrentLimit();
			LOGi("Set default current limit value to %i", _current_limit_val);
			*_currentLimitCharacteristic = _current_limit_val;
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
				_clock->init();
				_clock->start();

				// Wait for the RTC to actually start
				nrf_delay_us(100);

				LOGi("Start ADC");
				_adc.nrf_adc_start();
				// replace by timer!

}

void PowerService::sampleAdcStart() {
	while (!_adc.getBuffer()->full()) {
		nrf_delay_ms(100);
	}
	LOGi("Number of results: %u", _adc.getBuffer()->count()/2);
	LOGi("Counter is at: %u", _clock->getCount());

	LOGi("Stop ADC converter");
	_adc.nrf_adc_stop();

	// Wait for the ADC to actually stop
	nrf_delay_us(1000);

	LOGi("Stop RTC");
	_clock->stop();
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
	while (!_adc.getBuffer()->empty()) {
		_log(INFO, "%u, ", _adc.getBuffer()->pop());
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

PowerService& PowerService::createService(Nrf51822BluetoothStack& stack, ADC& adc, Storage& storage,
		RealTimeClock &clock, LPComp &lpcomp) {
//	LOGd("Create power service");
	PowerService* svc = new PowerService(stack, adc, storage, clock, lpcomp);
	stack.addService(svc);
	svc->GenericService::addSpecificCharacteristics();
	return *svc;
}

