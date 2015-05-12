/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

//#include <cstdio>
//
#include "services/cs_PowerService.h"
//
#include "cfg/cs_Boards.h"
//#include "common/cs_Config.h"
//#include "common/cs_Strings.h"
//#include "cs_nRF51822.h"
#include "drivers/cs_RTC.h"
#include "structs/buffer/cs_MasterBuffer.h"
#include "cfg/cs_UuidConfig.h"

#include "drivers/cs_ADC.h"
#include "drivers/cs_PWM.h"
#include <drivers/cs_Timer.h>
#include <drivers/cs_LPComp.h>

#include <cfg/cs_Settings.h>

using namespace BLEpp;

PowerService::PowerService() :
								_pwmCharacteristic(NULL),
								_sampleCurrentCharacteristic(NULL),
								_currentConsumptionCharacteristic(NULL),
								_currentCurveCharacteristic(NULL),
								_currentLimitCharacteristic(NULL),
								_currentCurve(NULL),
								_currentLimitVal(0),
								_adcInitialized(false),
								_currentLimitInitialized(false),
								_samplingType(0)
{

	setUUID(UUID(POWER_UUID));

	setName(BLE_SERVICE_POWER);

	Settings::getInstance();
//	Storage::getInstance().getHandle(PS_ID_POWER_SERVICE, _storageHandle);
//	loadPersistentStorage();

	init();

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)PowerService::staticTick);
}

void PowerService::init() {
	LOGi("Create power service");

#if CHAR_PWM==1
	LOGi("add PWM Characteristic");
	addPWMCharacteristic();
#else
	LOGi("skip PWM Characteristic");
#endif

#if CHAR_SAMPLE_CURRENT==1
	LOGi("add Sample Current Characteristic");
	addSampleCurrentCharacteristic();
	LOGi("add Current Curve Characteristic");
	addCurrentCurveCharacteristic();
	LOGi("add Current Consumption Characteristic");
	addCurrentConsumptionCharacteristic();
#else
	LOGi("skip Sample Current Characteristic");
	LOGi("skip Current Curve Characteristic");
	LOGi("skip Current Consumption Characteristic");
#endif

#if CHAR_CURRENT_LIMIT==1
	LOGi("add Current Limitation");
	addCurrentLimitCharacteristic();
#else
	LOGi("skip Current Limitation");
#endif
}

/**
 * TODO: We should only need to do this once on startup.
 */
void PowerService::tick() {
	//	LOGi("Tock: %d", RTC::now());

	// Initialize at first tick, to delay it a bit, prevents voltage peak going into the AIN pin.
	// TODO: This is not required anymore at later crownstone versions, so this should be done at init().

	if(!_currentLimitInitialized) {
		setCurrentLimit(_currentLimitVal);
		_currentLimitInitialized = true;
	}

	if (!_adcInitialized) {
		// Init only when you sample, so that the the pin is only configured as AIN after the big spike at startup.
		ADC::getInstance().init(PIN_AIN_ADC);
		sampleCurrentInit();
		_adcInitialized = true;
	}

	if (_samplingType && _currentCurve->isFull()) {
		sampleCurrent(_samplingType);
		_samplingType = 0;
	}

	//~ LPComp::getInstance().tick();
	// check if current is not beyond current_limit if the latter is set
	//	if (++tmp_cnt > tick_cnt) {
	//		if (_currentLimitCharacteristic) {
	//			getCurrentLimit();
	//			LOGi("Set default current limit value to %i", _current_limit_val);
	//			*_currentLimitCharacteristic = _current_limit_val;
	//		}
	//		tmp_cnt = 0;
	//	}

	scheduleNextTick();
}

void PowerService::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(POWER_SERVICE_UPDATE_FREQUENCY), this);
}

//void PowerService::loadPersistentStorage() {
//	Storage::getInstance().readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
//}

//void PowerService::savePersistentStorage() {
//	Storage::getInstance().writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
//}

void PowerService::addPWMCharacteristic() {
	_pwmCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_pwmCharacteristic);
	_pwmCharacteristic->setUUID(UUID(getUUID(), PWM_UUID));
	_pwmCharacteristic->setName(BLE_CHAR_PWM);
	_pwmCharacteristic->setDefaultValue(255);
	_pwmCharacteristic->setWritable(true);
	_pwmCharacteristic->onWrite([&](const uint8_t& value) -> void {
		//			LOGi("set pwm to %i", value);
		PWM::getInstance().setValue(0, value);
	});
}

// Do we really want to use the PWM for this, or just set the pin to zero?
// TODO: turn off normally, but make sure we enable the completely PWM again on request
void PowerService::turnOff() {
	// update pwm characteristic so that the current value can be read from the characteristic
	if (_pwmCharacteristic != NULL) {
		(*_pwmCharacteristic) = 0;
	}
	PWM::getInstance().setValue(0, 0);
}

// Do we really want to use the PWM for this, or just set the pin to zero?
// TODO: turn on normally, but make sure we enable the completely PWM again on request
void PowerService::turnOn() {
	// update pwm characteristic so that the current value can be read from the characteristic
	if (_pwmCharacteristic != NULL) {
		(*_pwmCharacteristic) = 255;
	}
	PWM::getInstance().setValue(0, 255);
}

void PowerService::dim(uint8_t value) {
	// update pwm characteristic so that the current value can be read from the characteristic
	if (_pwmCharacteristic != NULL) {
		(*_pwmCharacteristic) = value;
	}
	PWM::getInstance().setValue(0, value);
}

void PowerService::addSampleCurrentCharacteristic() {
	_sampleCurrentCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_sampleCurrentCharacteristic);
	_sampleCurrentCharacteristic->setUUID(UUID(getUUID(), SAMPLE_CURRENT_UUID));
	_sampleCurrentCharacteristic->setName("Sample Current");
	_sampleCurrentCharacteristic->setDefaultValue(0);
	_sampleCurrentCharacteristic->setWritable(true);
	_sampleCurrentCharacteristic->onWrite([&](const uint8_t& value) -> void {
		MasterBuffer& mb = MasterBuffer::getInstance();
		if (!mb.isLocked()) {
			mb.lock();
		} else {
			LOGe("Buffer is locked. Cannot be written!");
			return;
		}

		// Start storing the samples
		_currentCurve->clear();
		ADC::getInstance().setCurrentCurve(_currentCurve);

		_samplingType = value;
	});
}

void PowerService::addCurrentCurveCharacteristic() {
	_currentCurveCharacteristic = new Characteristic<uint8_t*>();
	addCharacteristic(_currentCurveCharacteristic);
	_currentCurveCharacteristic->setUUID(UUID(getUUID(), CURRENT_CURVE_UUID));
	_currentCurveCharacteristic->setName("Current Curve");
	_currentCurveCharacteristic->setWritable(false);
	_currentCurveCharacteristic->setNotifies(true);

	_currentCurve = new CurrentCurve<uint16_t>();
	MasterBuffer& mb = MasterBuffer::getInstance();
	uint8_t *buffer = NULL;
	uint16_t size = 0;
	mb.getBuffer(buffer, size);
	LOGd("Assign buffer of size %i to current curve", size);
	_currentCurve->assign(buffer, size);

	_currentCurveCharacteristic->setValue(buffer);
	_currentCurveCharacteristic->setMaxLength(size);
	_currentCurveCharacteristic->setDataLength(size);
}

void PowerService::addCurrentConsumptionCharacteristic() {
	_currentConsumptionCharacteristic = new Characteristic<uint16_t>();
	addCharacteristic(_currentConsumptionCharacteristic);
	_currentConsumptionCharacteristic->setUUID(UUID(getUUID(), CURRENT_CONSUMPTION_UUID));
	_currentConsumptionCharacteristic->setName("Current Consumption");
	_currentConsumptionCharacteristic->setDefaultValue(0);
	_currentConsumptionCharacteristic->setWritable(false);
	_currentConsumptionCharacteristic->setNotifies(true);
}

uint8_t PowerService::getCurrentLimit() {
	Storage::getUint8(Settings::getInstance().getConfig().current_limit, _currentLimitVal, 0);
	LOGi("Obtained current limit from FLASH: %i", _currentLimitVal);
	return _currentLimitVal;
}

void PowerService::setCurrentLimit(uint8_t value) {
#if CHAR_CURRENT_LIMIT==1
	LOGi("Set current limit to: %i", value);
	_currentLimitVal = value;
	//if (!_currentLimitInitialized) {
	//_currentLimit.init();
	//_currentLimitInitialized = true;
	//}
	LPComp::getInstance().stop();
	LPComp::getInstance().config(PIN_AIN_LPCOMP, _currentLimitVal, LPComp::LPC_UP);
	LPComp::getInstance().start();
	LOGi("Write value to persistent memory");
	Storage::setUint8(_currentLimitVal, Settings::getInstance().getConfig().current_limit);
	Settings::getInstance().savePersistentStorage();
#endif
}

/**
 * The characteristic that writes a current limit to persistent memory.
 *
 * TODO: Check https://devzone.nordicsemi.com/question/1745/how-to-handle-flashwrit-in-an-safe-way/
 *       Writing to persistent memory should be done between connection/advertisement events...
 */
// TODO -oDE: make part of configuration characteristic
void PowerService::addCurrentLimitCharacteristic() {
	_currentLimitCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_currentLimitCharacteristic);
	_currentLimitCharacteristic->setNotifies(true);
	_currentLimitCharacteristic->setUUID(UUID(getUUID(), CURRENT_LIMIT_UUID));
	_currentLimitCharacteristic->setName("Current Limit");
	_currentLimitCharacteristic->setDefaultValue(getCurrentLimit());
	_currentLimitCharacteristic->setWritable(true);
	_currentLimitCharacteristic->onWrite([&](const uint8_t& value) -> void {
		setCurrentLimit(value);
		_currentLimitInitialized = true;
	});

	// TODO: we have to delay the init, since there is a spike on the AIN pin at startup!
	// For now: init at onWrite, so we can still test it.
	//	_currentLimit.start(&_currentLimitVal);
	//	_currentLimit.init();
}

//static int tmp_cnt = 100;
//static int tick_cnt = 100;

void PowerService::sampleCurrentInit() {
	LOGi("Start ADC");
	ADC::getInstance().start();

	//	// Wait for the ADC to actually start
	//	nrf_delay_ms(5);
}

void PowerService::sampleCurrent(uint8_t type) {
	// Stop storing the samples
	ADC::getInstance().setCurrentCurve(NULL);

	uint16_t numSamples = _currentCurve->length();
	LOGd("numSamples = %i", numSamples);
	if ((type & 0x2) && _currentCurveCharacteristic != NULL) {
		_currentCurveCharacteristic->setDataLength(_currentCurve->getDataLength());
		_currentCurveCharacteristic->notify();
	}

	if (numSamples>1) {
		uint32_t voltageSquareMean = 0;
		uint32_t timestamp = 0;
		uint16_t voltage = 0;
		//		uint32_t timeStart = _currentCurve->getTimeStart();
		//		uint32_t timeEnd = _currentCurve->getTimeEnd();

#ifdef MICRO_VIEW
		write("3 [");
#endif
		for (uint16_t i=0; i<numSamples; ++i) {
			if (_currentCurve->getValue(i, voltage, timestamp) != CC_SUCCESS) {
				break;
			}
			//			timestamp = timeStart + i*(timeEnd-timeStart)/(numSamples-1);
			_log(INFO, "%u %u,  ", timestamp, voltage);
#ifdef MICRO_VIEW
			write("%u %u ", timestamp, voltage);
#endif
			if (!((i+1) % 5)) {
				_log(INFO, "\r\n");
			}
			voltageSquareMean += voltage*voltage;
		}
		_log(INFO, "\r\n");
#ifdef MICRO_VIEW
		write("]r\n");
#endif

		if ((type & 0x1) && _currentConsumptionCharacteristic != NULL) {
			// Take the mean
			voltageSquareMean /= numSamples;

			// Measured voltage goes from 0-1.2V, measured as 0-1023(10 bit)
			// Convert to mV^2
			voltageSquareMean = voltageSquareMean*1200/1023*1200/1023;

			// Convert to A^2, use I=V/R
			uint16_t currentSquareMean = voltageSquareMean * 1000 / SHUNT_VALUE * 1000 / SHUNT_VALUE * 1000*1000;

			LOGi("currentSquareMean = %u mA^2", currentSquareMean);

			*_currentConsumptionCharacteristic = currentSquareMean;
		}
	}

	// Unlock the buffer
	MasterBuffer::getInstance().unlock();
}
