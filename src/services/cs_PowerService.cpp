/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */


//#include <cmath> // try not to use this!
#include <cstdio>

#include "common/cs_Boards.h"
#include "common/cs_Config.h"
#include "cs_nRF51822.h"
#include "drivers/cs_RTC.h"
#include "services/cs_PowerService.h"

using namespace BLEpp;

PowerService::PowerService(Nrf51822BluetoothStack& _stack) :
		_stack(&_stack), _currentLimitCharacteristic(NULL), _currentLimitVal(0)
{

	setUUID(UUID(POWER_SERVICE_UUID));
	//setUUID(UUID(0x3800)); // there is no BLE_UUID for indoor localization (yet)
	
	LOGi("Create power service");

	characStatus.reserve(5);
	characStatus.push_back( { "PWM",
			PWM_UUID,
			true,
			static_cast<addCharacteristicFunc>(&PowerService::addPWMCharacteristic)});
	characStatus.push_back( { "Get Current",
			CURRENT_GET_UUID,
			true,
			static_cast<addCharacteristicFunc>(&PowerService::addGetCurrentCharacteristic)});
	characStatus.push_back( { "Current Curve",
			CURRENT_CURVE_UUID,
			true,
			static_cast<addCharacteristicFunc>(&PowerService::addCurrentCurveCharacteristic)});
	characStatus.push_back( { "Current Consumption",
			CURRENT_CONSUMPTION_UUID,
			true,
			static_cast<addCharacteristicFunc>(&PowerService::addCurrentConsumptionCharacteristic)});
	characStatus.push_back( { "Current Limit",
			CURRENT_LIMIT_UUID,
			false,
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
			PWM::getInstance().setValue(0, value);
		});
}

// Do we really want to use the PWM for this, or just set the pin to zero?
// TODO: turn off normally, but make sure we enable the completely PWM again on request
void PowerService::turnOff() {
	PWM::getInstance().setValue(0, 0);
}

// Do we really want to use the PWM for this, or just set the pin to zero?
// TODO: turn on normally, but make sure we enable the completely PWM again on request
void PowerService::turnOn() {
	PWM::getInstance().setValue(0, (uint8_t)-1);
}

/**
 * Dim the light, note that we use PWM. You might need another way to dim the light! For example by only turning on for
 * a specific duty-cycle after the detection of a zero crossing.
 */
void PowerService::dim(uint8_t value) {
	PWM::getInstance().setValue(0, value);
}

void PowerService::addGetCurrentCharacteristic() {
	createCharacteristic<uint8_t>()
		.setUUID(UUID(getUUID(), CURRENT_GET_UUID))
		.setName("Get Current")
		.setDefaultValue(0)
		.setWritable(true)
		.onWrite([&](const uint8_t& value) -> void {
			sampleCurrentInit();
			uint16_t current_rms = sampleCurrentFinish(value);
			if ((value & 0x01) && _currentConsumptionCharacteristic != NULL) {
				(*_currentConsumptionCharacteristic) = current_rms;
			}
			if ((value & 0x02) && _currentCurveCharacteristic != NULL) {
				(*_currentCurveCharacteristic) = _streamBuffer; // TODO: stream curve
			}
		});
	ADC::getInstance().init(PIN_AIN_ADC);
}

void PowerService::addCurrentCurveCharacteristic() {
	_currentCurveCharacteristic = &createCharacteristic<StreamBuffer>()
		.setUUID(UUID(getUUID(), CURRENT_CURVE_UUID))
		.setName("Current Curve")
		.setWritable(false)
		.setNotifies(true);
//		.onWrite([&](const uint16_t& value) -> void {
//			sampleVoltageCurveInit();
////			PWM::getInstance().setValue(0, value);
//			sampleVoltageCurveFinish();
//		})
//		.onRead([&] () -> uint16_t {
//			sampleVoltageCurveInit();
//			return sampleVoltageCurveFinish();
//		});
}

void PowerService::addCurrentConsumptionCharacteristic() {
//	LOGd("create characteristic to read power consumption");
	_currentConsumptionCharacteristic = &createCharacteristic<uint16_t>()
		.setUUID(UUID(getUUID(), CURRENT_CONSUMPTION_UUID))
		.setName("Current Consumption")
		.setDefaultValue(0)
		.setWritable(false)
		.setNotifies(true);
}

uint8_t PowerService::getCurrentLimit() {
	loadPersistentStorage();
	Storage::getUint8(_storageStruct.current_limit, _currentLimitVal, 0);
	LOGi("Obtained current limit from FLASH: %i", _currentLimitVal);
	return _currentLimitVal;
}

/**
 * The characteristic that writes a current limit to persistent memory.
 *
 * TODO: Check https://devzone.nordicsemi.com/question/1745/how-to-handle-flashwrit-in-an-safe-way/ 
 *       Writing to persistent memory should be done between connection/advertisement events...
 */
void PowerService::addCurrentLimitCharacteristic() {
	_currentLimitCharacteristic = createCharacteristicRef<uint8_t>();
	(*_currentLimitCharacteristic)
		.setNotifies(true)
		.setUUID(UUID(getUUID(), CURRENT_LIMIT_UUID))
		.setName("Current Limit")
		.setDefaultValue(getCurrentLimit())
		.setWritable(true)
		.onWrite([&](const uint8_t &value) -> void {
			LOGi("Set current limit to: %i", value);
			_currentLimitVal = value;

			_currentLimit.start(&_currentLimitVal);
			LOGi("Write value to persistent memory");
			Storage::setUint8(_currentLimitVal, _storageStruct.current_limit);
			savePersistentStorage();
		});

	_currentLimit.start(&_currentLimitVal);
	_currentLimit.init();
}

//static int tmp_cnt = 100;
//static int tick_cnt = 100;

/**
 * TODO: We should only need to do this once on startup.
 */
void PowerService::tick() {
	LPComp::getInstance().tick();
	// check if current is not beyond current_limit if the latter is set
//	if (++tmp_cnt > tick_cnt) {
//		if (_currentLimitCharacteristic) {
//			getCurrentLimit();
//			LOGi("Set default current limit value to %i", _current_limit_val);
//			*_currentLimitCharacteristic = _current_limit_val;
//		}
//		tmp_cnt = 0;
//	}
}

void PowerService::sampleCurrentInit() {
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

	LOGi("Start RTC");
	RealTimeClock::getInstance().init();
	RealTimeClock::getInstance().start();
	ADC::getInstance().setClock(RealTimeClock::getInstance());

	// Wait for the RTC to actually start
	nrf_delay_us(100);

	LOGi("Start ADC");
	ADC::getInstance().start();
	// replace by timer!

}

uint16_t PowerService::sampleCurrentFinish(uint8_t type) {
	while (!ADC::getInstance().getBuffer()->full()) {
		nrf_delay_ms(100);
	}

	LOGi("Stop ADC");
	ADC::getInstance().stop();

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

	uint16_t voltage_max = 0;
	int i = 0;
	while (!ADC::getInstance().getBuffer()->empty()) {
		uint16_t voltage = ADC::getInstance().getBuffer()->pop();

		// First and last elements of the buffer are timestamps
		if ((type & 0x1) && (voltage > voltage_max) && (i>0) && (ADC::getInstance().getBuffer()->count() > 1)) {
			voltage_max = voltage;
		}

		if (type & 0x2) {
			// cast to uint8_t
			_streamBuffer.add((uint8_t)voltage);
			_log(INFO, "%u, ", voltage);
			if (!(++i % 10)) {
				_log(INFO, "\r\n");
			}
		}
	}
	if (type & 0x2) {
		_log(INFO, "\r\n");
	}

	if (type & 0x1) {
		// measured voltage goes from 0-1.2V, measured as 0-1023(10 bit)
		// 1023 * 1200 / 1023 = 1200 < 2^16
		voltage_max = voltage_max*1200/1023; // mV
		uint16_t current_rms = voltage_max * 1000 / SHUNT_VALUE * 1000 / 1414; // mA
		LOGi("Irms = %u mA\r\n", current_rms);
		return current_rms;
	}

	return 0;
}

PowerService& PowerService::createService(Nrf51822BluetoothStack& stack) {
//	LOGd("Create power service");
	PowerService* svc = new PowerService(stack);
	stack.addService(svc);
	svc->GenericService::addSpecificCharacteristics();
	return *svc;
}

