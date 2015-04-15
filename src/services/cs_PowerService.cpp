/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <cstdio>

#include "common/cs_Boards.h"
#include "common/cs_Config.h"
#include "cs_nRF51822.h"
#include "drivers/cs_RTC.h"
#include "services/cs_PowerService.h"
#include "common/cs_MasterBuffer.h"
#include "characteristics/cs_BufferCharacteristic.h"

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
		_currentLimitInitialized(false)
{

	setUUID(UUID(POWER_SERVICE_UUID));

	setName(std::string("Power Service"));

	Storage::getInstance().getHandle(PS_ID_POWER_SERVICE, _storageHandle);
	loadPersistentStorage();

	init();
}

void PowerService::init() {
	LOGi("Create power service");

	addPWMCharacteristic();
	addSampleCurrentCharacteristic();
	addCurrentCurveCharacteristic();
	addCurrentConsumptionCharacteristic();
	addCurrentLimitCharacteristic();
}

void PowerService::loadPersistentStorage() {
	Storage::getInstance().readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void PowerService::savePersistentStorage() {
	Storage::getInstance().writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void PowerService::addPWMCharacteristic() {
	_pwmCharacteristic = new CharacteristicT<uint8_t>();
	addCharacteristic(_pwmCharacteristic);
	_pwmCharacteristic->setUUID(UUID(getUUID(), PWM_UUID));
	_pwmCharacteristic->setName("PWM");
	_pwmCharacteristic->setDefaultValue(255);
	_pwmCharacteristic->setWritable(true);
	_pwmCharacteristic->onWrite([&]() -> void {
			const uint8_t& value = _pwmCharacteristic->getValue();
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

/**
 * Dim the light, note that we use PWM. You might need another way to dim the light! For example by only turning on for
 * a specific duty-cycle after the detection of a zero crossing.
 */
void PowerService::dim(uint8_t value) {
	// update pwm characteristic so that the current value can be read from the characteristic
	if (_pwmCharacteristic != NULL) {
		(*_pwmCharacteristic) = value;
	}
	PWM::getInstance().setValue(0, value);
}

void PowerService::addSampleCurrentCharacteristic() {
	_sampleCurrentCharacteristic = new CharacteristicT<uint8_t>();
	addCharacteristic(_sampleCurrentCharacteristic);
	_sampleCurrentCharacteristic->setUUID(UUID(getUUID(), SAMPLE_CURRENT_UUID));
	_sampleCurrentCharacteristic->setName("Sample Current");
	_sampleCurrentCharacteristic->setDefaultValue(0);
	_sampleCurrentCharacteristic->setWritable(true);
	_sampleCurrentCharacteristic->onWrite([&]() -> void {
			const uint8_t &type = _sampleCurrentCharacteristic->getValue();
			if (!_adcInitialized) {
				// Init only when you sample, so that the the pin is only configured as AIN after the big spike at startup.
				ADC::getInstance().init(PIN_AIN_ADC);
				sampleCurrentInit();
				_adcInitialized = true;
			}
			sampleCurrent(type);
		});
//	ADC::getInstance().init(PIN_AIN_ADC);
}

void PowerService::addCurrentCurveCharacteristic() {
	_currentCurveCharacteristic = new CharacteristicT<uint8_t*>();
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
//	LOGd("create characteristic to read power consumption");
	_currentConsumptionCharacteristic = new CharacteristicT<uint16_t>();
	addCharacteristic(_currentConsumptionCharacteristic);
	_currentConsumptionCharacteristic->setUUID(UUID(getUUID(), CURRENT_CONSUMPTION_UUID));
	_currentConsumptionCharacteristic->setName("Current Consumption");
	_currentConsumptionCharacteristic->setDefaultValue(0);
	_currentConsumptionCharacteristic->setWritable(false);
	_currentConsumptionCharacteristic->setNotifies(true);
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
	_currentLimitCharacteristic = new CharacteristicT<uint8_t>();
	addCharacteristic(_currentLimitCharacteristic);
	_currentLimitCharacteristic->setNotifies(true);
	_currentLimitCharacteristic->setUUID(UUID(getUUID(), CURRENT_LIMIT_UUID));
	_currentLimitCharacteristic->setName("Current Limit");
	_currentLimitCharacteristic->setDefaultValue(getCurrentLimit());
	_currentLimitCharacteristic->setWritable(true);
		//.onWrite([&](const uint8_t &value) -> void {
	_currentLimitCharacteristic->onWrite([&]() -> void {
			const uint8_t& value = _currentLimitCharacteristic->getValue();
			LOGi("Set current limit to: %i", value);
			_currentLimitVal = value;
			if (!_currentLimitInitialized) {
				_currentLimit.init();
				_currentLimitInitialized = true;
			}
			_currentLimit.start(&_currentLimitVal);
			LOGi("Write value to persistent memory");
			Storage::setUint8(_currentLimitVal, _storageStruct.current_limit);
			savePersistentStorage();
		});

	// TODO: we have to delay the init, since there is a spike on the AIN pin at startup!
	// For now: init at onWrite, so we can still test it.
//	_currentLimit.start(&_currentLimitVal);
//	_currentLimit.init();
}

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
	LOGi("Start RTC");
	RealTimeClock::getInstance().init();
	RealTimeClock::getInstance().start();
	ADC::getInstance().setClock(RealTimeClock::getInstance());

	// Wait for the RTC to actually start
	nrf_delay_ms(1);

	LOGi("Start ADC");
	ADC::getInstance().start();

//	// Wait for the ADC to actually start
//	nrf_delay_ms(5);
}

void PowerService::sampleCurrent(uint8_t type) {
	MasterBuffer& mb = MasterBuffer::getInstance();
	if (!mb.isLocked()) {
		mb.lock();

	} else {
		LOGe("Buffer is locked. Cannot be written!");
	}

	// Start storing the samples
	_currentCurve->clear();
	ADC::getInstance().setCurrentCurve(_currentCurve);

	// Give some time to sample
	while (!_currentCurve->isFull()) {
		nrf_delay_ms(10);
	}

	// Stop storing the samples
	ADC::getInstance().setCurrentCurve(NULL);

	uint16_t numSamples = _currentCurve->length();
	if ((type & 0x2) && _currentCurveCharacteristic != NULL) {
		_currentCurveCharacteristic->setDataLength(_currentCurve->getDataLength());
		_currentCurveCharacteristic->notify();
	}

	if (numSamples) {
		uint32_t voltageSquareMean = 0;
		uint32_t timestamp = 0;
		uint16_t voltage = 0;
#ifdef MICRO_VIEW
		write("3 [");
#endif
		for (uint16_t i=0; i<numSamples; ++i) {
			if (_currentCurve->getValue(i, voltage) != CC_SUCCESS) {
				break;
			}
			_log(INFO, "%u %u,  ", timestamp, voltage);
#ifdef MICRO_VIEW
			write("%u %u ", timestamp, voltage);
#endif
			if (!(++i % 5)) {
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
	mb.unlock();
}
