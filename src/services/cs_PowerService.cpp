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

#include <protocol/cs_Mesh.h>
#include <protocol/cs_MeshControl.h>
#include <cfg/cs_Settings.h>

// change to #def to enable output of sample current values
#undef PRINT_SAMPLE_CURRENT

using namespace BLEpp;

PowerService::PowerService() :
		_pwmCharacteristic(NULL),
		_relayCharacteristic(NULL),
		_sampleCurrentCharacteristic(NULL),
		_powerConsumptionCharacteristic(NULL),
		_currentCurveCharacteristic(NULL),
		_currentLimitCharacteristic(NULL),
		_powerCurve(NULL),
		_currentLimitVal(0),
		_adcInitialized(false),
		_currentLimitInitialized(false),
		_samplingType(0),
		_sampling(false),
		_samplingTime(50),
		_samplingInterval(500)
{

	setUUID(UUID(POWER_UUID));

	setName(BLE_SERVICE_POWER);

	Settings::getInstance();
//	Storage::getInstance().getHandle(PS_ID_POWER_SERVICE, _storageHandle);
//	loadPersistentStorage();
	ps_configuration_t cfg = Settings::getInstance().getConfig();
	Storage::getUint16(cfg.samplingTime, _samplingTime, SAMPLING_TIME);
	Storage::getUint16(cfg.samplingInterval, _samplingInterval, SAMPLING_INTERVAL);

	init();

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)PowerService::staticTick);

	LOGe("power service timer id: %d", _appTimerId);

	Timer::getInstance().createSingleShot(_staticSamplingTimer, (app_timer_timeout_handler_t)PowerService::staticSampleCurrent);

	LOGe("sample power timer id: %d", _staticSamplingTimer);
}

void PowerService::init() {
	LOGi("Create power service");

#if CHAR_PWM==1
	LOGi("add PWM Characteristic");
	addPWMCharacteristic();
#else
	LOGi("skip PWM Characteristic");
#endif

#if CHAR_RELAY==1
	LOGi("add Relay Characteristic");
	addRelayCharacteristic();
#else
	LOGi("skip Relay Characteristic");
#endif

#if CHAR_SAMPLE_CURRENT==1
	LOGi("add Sample Current Characteristic");
	addSampleCurrentCharacteristic();
	LOGi("add Current Curve Characteristic");
	addCurrentCurveCharacteristic();
	LOGi("add Current Consumption Characteristic");
	addPowerConsumptionCharacteristic();
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

	//! Initialize at first tick, to delay it a bit, prevents voltage peak going into the AIN pin.
	//! TODO: This is not required anymore at later crownstone versions, so this should be done at init().

#if CHAR_CURRENT_LIMIT==1
	if(!_currentLimitInitialized) {
		setCurrentLimit(_currentLimitVal);
		_currentLimitInitialized = true;
	}
#endif

//	if (!_adcInitialized) {
//		//! Init only when you sample, so that the the pin is only configured as AIN after the big spike at startup.
//		ADC::getInstance().init(PIN_AIN_ADC);
//		sampleCurrentInit();
//		_adcInitialized = true;
//	}

//	if (_samplingType && _currentCurve->isFull()) {
//		sampleCurrentDone(_samplingType);
//		_samplingType = 0;
//	}

	//~ LPComp::getInstance().tick();
	//! check if current is not beyond current_limit if the latter is set
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
//	LOGi("PowerService::scheduleNextTick");
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
			LOGi("set pwm to %i", value);
			PWM::getInstance().setValue(0, value);
	});
}

void PowerService::addRelayCharacteristic() {
	nrf_gpio_cfg_output(PIN_GPIO_RELAY_OFF);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_OFF);
	nrf_gpio_cfg_output(PIN_GPIO_RELAY_ON);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_ON);

	_relayCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_relayCharacteristic);
	_relayCharacteristic->setUUID(UUID(getUUID(), RELAY_UUID));
	_relayCharacteristic->setName(BLE_CHAR_RELAY);
	_relayCharacteristic->setDefaultValue(255);
	_relayCharacteristic->setWritable(true);
	_relayCharacteristic->onWrite([&](const uint8_t& value) -> void {
		if (value == 0) {
			LOGi("trigger relay off pin for %d ms", RELAY_HIGH_DURATION);
			nrf_gpio_pin_set(PIN_GPIO_RELAY_OFF);
			nrf_delay_ms(RELAY_HIGH_DURATION);
			nrf_gpio_pin_clear(PIN_GPIO_RELAY_OFF);
		} else {
			LOGi("trigger relay on pin for %d ms", RELAY_HIGH_DURATION);
			nrf_gpio_pin_set(PIN_GPIO_RELAY_ON);
			nrf_delay_ms(RELAY_HIGH_DURATION);
			nrf_gpio_pin_clear(PIN_GPIO_RELAY_ON);
		}
	});
}

//! Do we really want to use the PWM for this, or just set the pin to zero?
//! TODO: turn off normally, but make sure we enable the completely PWM again on request
void PowerService::turnOff() {
	//! update pwm characteristic so that the current value can be read from the characteristic
	if (_pwmCharacteristic != NULL) {
		(*_pwmCharacteristic) = 0;
	}
	PWM::getInstance().setValue(0, 0);
}

//! Do we really want to use the PWM for this, or just set the pin to zero?
//! TODO: turn on normally, but make sure we enable the completely PWM again on request
void PowerService::turnOn() {
	//! update pwm characteristic so that the current value can be read from the characteristic
	if (_pwmCharacteristic != NULL) {
		(*_pwmCharacteristic) = 255;
	}
	PWM::getInstance().setValue(0, 255);
}

void PowerService::dim(uint8_t value) {
	//! update pwm characteristic so that the current value can be read from the characteristic
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

		//! Start storing the samples
//		_currentCurve->clear();
//		ADC::getInstance().setCurrentCurve(_currentCurve);
//		_samplingType = value;

//		sampleCurrent(value);
		if (value == 0) {
			stopSampling();
		} else {
			startSampling(value);
		}
	});
}

void PowerService::addCurrentCurveCharacteristic() {
	_currentCurveCharacteristic = new Characteristic<uint8_t*>();
	addCharacteristic(_currentCurveCharacteristic);
	_currentCurveCharacteristic->setUUID(UUID(getUUID(), CURRENT_CURVE_UUID));
	_currentCurveCharacteristic->setName("Current Curve");
	_currentCurveCharacteristic->setWritable(false);
	_currentCurveCharacteristic->setNotifies(true);

	_powerCurve = new PowerCurve<uint16_t>();
//	MasterBuffer& mb = MasterBuffer::getInstance();
//	uint8_t *buffer = NULL;
//	uint16_t size = 0;
//	mb.getBuffer(buffer, size);
	uint8_t *buffer = _sampleBuffer;
	uint16_t size = sizeof(_sampleBuffer);
	LOGd("Assign buffer of size %i to current curve", size);
	_powerCurve->assign(buffer, size);

	_currentCurveCharacteristic->setValue(buffer);
	_currentCurveCharacteristic->setMaxLength(size);
	_currentCurveCharacteristic->setDataLength(size);
}

void PowerService::addPowerConsumptionCharacteristic() {
	_powerConsumptionCharacteristic = new Characteristic<uint16_t>();
	addCharacteristic(_powerConsumptionCharacteristic);
	_powerConsumptionCharacteristic->setUUID(UUID(getUUID(), CURRENT_CONSUMPTION_UUID));
	_powerConsumptionCharacteristic->setName("Power Consumption");
	_powerConsumptionCharacteristic->setDefaultValue(0);
	_powerConsumptionCharacteristic->setWritable(false);
	_powerConsumptionCharacteristic->setNotifies(true);
}

uint8_t PowerService::getCurrentLimit() {
	Storage::getUint8(Settings::getInstance().getConfig().current_limit, _currentLimitVal, 0);
	LOGi("Obtained current limit from FLASH: %i", _currentLimitVal);
	return _currentLimitVal;
}

//! TODO: doesn't work for now
void PowerService::setCurrentLimit(uint8_t value) {
	LOGi("Set current limit to: %i", value);
	_currentLimitVal = value;
	//if (!_currentLimitInitialized) {
	//_currentLimit.init();
	//_currentLimitInitialized = true;
	//}
//	LPComp::getInstance().stop();
//	LPComp::getInstance().config(PIN_AIN_LPCOMP, _currentLimitVal, LPComp::LPC_UP);
//	LPComp::getInstance().start();
	LOGi("Write value to persistent memory");
	Storage::setUint8(_currentLimitVal, Settings::getInstance().getConfig().current_limit);
	Settings::getInstance().savePersistentStorage();
}

/**
 * The characteristic that writes a current limit to persistent memory.
 *
 * TODO: Check https://devzone.nordicsemi.com/question/1745/how-to-handle-flashwrit-in-an-safe-way/
 *       Writing to persistent memory should be done between connection/advertisement events...
 */
//! TODO -oDE: make part of configuration characteristic
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

	//! TODO: we have to delay the init, since there is a spike on the AIN pin at startup!
	//! For now: init at onWrite, so we can still test it.
	//	_currentLimit.start(&_currentLimitVal);
	//	_currentLimit.init();
}

//static int tmp_cnt = 100;
//static int tick_cnt = 100;

//void PowerService::sampleCurrentInit() {
//	LOGi("Start ADC");
//	ADC::getInstance().start();
//
//	//	// Wait for the ADC to actually start
//	//	nrf_delay_ms(5);
//}

uint32_t start;
bool first;

void PowerService::startSampling(uint8_t type) {
	_sampling = true;
	_samplingType = type;
	LOGi("start sampling");

	first = true;

	Timer::getInstance().start(_staticSamplingTimer, 5, this);

}

void PowerService::stopSampling() {
	LOGi("stop sampling");
	_sampling = false;
}



void PowerService::sampleCurrent() {

	if (!_sampling) {
		return;
	}
	if (first) {

		// Start storing the samples
		_voltagePin = false;
		_powerCurve->clear();
		ADC::getInstance().setPowerCurve(_powerCurve);

		ADC::getInstance().init(PIN_AIN_CURRENT);
		ADC::getInstance().start();

		start = RTC::getCount();

		first = false;

	} else if (_powerCurve->isFull()) {

		LOGw("sampling duration: %u", RTC::ticksToMs(RTC::difference(RTC::getCount(), start)));

		uint32_t sampleDone = RTC::getCount();

		sampleCurrentDone();

		LOGw("sampling done duration: %u", RTC::ticksToMs(RTC::difference(RTC::getCount(), sampleDone)));

		if (_sampling) {
			int32_t pause = MS_TO_TICKS(_samplingInterval) - RTC::difference(RTC::getCount(), start);
			pause = pause < 5 ? 5 : pause;
			LOGw("pause: %u", RTC::ticksToMs(pause));
			Timer::getInstance().start(_staticSamplingTimer, pause, this);
			first = true;
		}
		return;
	} else {
		if (NRF_ADC->EVENTS_END) {
			if (_voltagePin) {
				PC_ERR_CODE res = _powerCurve->addVoltage(NRF_ADC->RESULT, RTC::getCount());
	//			LOGd("%i %i voltage: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
				ADC::getInstance().config(PIN_AIN_CURRENT);
			}
			else {
				PC_ERR_CODE res = _powerCurve->addCurrent(NRF_ADC->RESULT, RTC::getCount());
	//			LOGd("%i %i current: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
				ADC::getInstance().config(PIN_AIN_VOLTAGE);
			}
			_voltagePin = !_voltagePin;
			ADC::getInstance().start();
		}
	}

	Timer::getInstance().start(_staticSamplingTimer, 5, this);

}

//void PowerService::sampleCurrent() {
//
//	if (!_sampling) {
//		return;
//	}
//	uint32_t start = RTC::now();
//
//	LOGw("sample current");
//
//	// Start storing the samples
//	_voltagePin = false;
//	_powerCurve->clear();
////	ADC::getInstance().setPowerCurve(_powerCurve);
//
//	ADC::getInstance().init(PIN_AIN_CURRENT);
//	ADC::getInstance().start();
//	while (!_powerCurve->isFull()) {
//		while(!NRF_ADC->EVENTS_END) {}
//		//			NRF_ADC->EVENTS_END	= 0;
//		//			LOGd("got sample");
//		if (_voltagePin) {
//			PC_ERR_CODE res = _powerCurve->addVoltage(NRF_ADC->RESULT, RTC::getCount());
////			LOGd("%i %i voltage: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
//			ADC::getInstance().config(PIN_AIN_CURRENT);
//		}
//		else {
//			PC_ERR_CODE res = _powerCurve->addCurrent(NRF_ADC->RESULT, RTC::getCount());
////			LOGd("%i %i current: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
//			ADC::getInstance().config(PIN_AIN_VOLTAGE);
//		}
//		_voltagePin = !_voltagePin;
//		ADC::getInstance().start();
//	}
//
//	uint32_t done = RTC::now();
//	LOGw("sample dt: %d", done- start);
//
//	sampleCurrentDone();
//
//	if (_sampling) {
//		int32_t pause = _samplingInterval - (RTC::now() - start);
//		pause = pause < 1 ? 1 : pause;
//		LOGw("pause: %d", pause);
//		Timer::getInstance().start(_staticSamplingTimer, MS_TO_TICKS(pause), this);
//	}
//}

void PowerService::sampleCurrentDone() {

	LOGw("sample current done");

//	uint32_t done = RTC::now();
//	//! Stop storing the samples
//	ADC::getInstance().setCurrentCurve(NULL);
	ADC::getInstance().setPowerCurve(NULL);

	uint16_t numSamples = _powerCurve->length();
	LOGd("numSamples = %i", numSamples);
	if ((_samplingType & 0x2) && _currentCurveCharacteristic != NULL) {
		_currentCurveCharacteristic->setDataLength(_powerCurve->getDataLength());
		_currentCurveCharacteristic->notify();
	}

#ifdef PRINT_SAMPLE_CURRENT
	if (numSamples>1) {
		uint16_t currentSample = 0;
		uint16_t voltageSample = 0;
		uint32_t currentTimestamp = 0;
		uint32_t voltageTimestamp = 0;
		LOGd("Samples:");
		for (uint16_t i=0; i<numSamples; ++i) {
			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
				break;
			}
			_log(DEBUG, "%u %u %u %u,  ", currentTimestamp, currentSample, voltageTimestamp, voltageSample);
			if (!((i+1) % 3)) {
				_log(DEBUG, "\r\n");
			}
		}
		_log(DEBUG, "\r\n");
	}
#endif

#ifdef MIRCO_VIEW
	if (numSamples>1) {
		uint16_t currentSample = 0;
		uint16_t voltageSample = 0;
		uint32_t currentTimestamp = 0;
		uint32_t voltageTimestamp = 0;
		write("3 [");
		for (uint16_t i=0; i<numSamples; ++i) {
			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
				break;
			}
			write("%u %u ", currentTimestamp, currentSample);
//			write("%u %u %u %u ", currentTimestamp, currentSample, voltageTimestamp, voltageSample);
		}
	}
#endif

	if (numSamples>1 && (_samplingType & 0x1) && _powerConsumptionCharacteristic != NULL) {
		uint16_t currentSample = 0;
		uint16_t voltageSample = 0;
		uint32_t currentTimestamp = 0;
		uint32_t voltageTimestamp = 0;
		uint32_t minCurrentSample = 1024;
		uint32_t maxCurrentSample = 0;
		for (uint16_t i=0; i<numSamples; ++i) {
			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
				break;
			}
			if (currentSample > maxCurrentSample) {
				maxCurrentSample = currentSample;
			}
			if (currentSample < minCurrentSample) {
				minCurrentSample = currentSample;
			}

			//! TODO: do some power = voltage * current, make sure we use timestamps too.
		}
		//! 1023*1000*1200 = 1.3e9 < 4.3e9 (max uint32)
		//! currentSampleAmplitude is max: 1023*1000*1200/1023/2 = 600000
		uint32_t currentSampleAmplitude = (maxCurrentSample-minCurrentSample) * 1000 * 1200 / 1023 / 2; //! uV
		//! max of currentAmplitude depends on amplification and shunt value
		uint32_t currentAmplitude = currentSampleAmplitude / VOLTAGE_AMPLIFICATION / SHUNT_VALUE; //! mA
		//! currentRms should be max:  16000
		uint32_t currentRms = currentAmplitude * 1000 / 1414; //! mA
		LOGi("currentRms = %i mA", currentRms);
		uint32_t powerRms = currentRms * 230 / 1000; //! Watt
		LOGi("powerRms = %i Watt", powerRms);
		*_powerConsumptionCharacteristic = powerRms;
	}

	//! Unlock the buffer
	MasterBuffer::getInstance().unlock();

//	uint32_t end = RTC::now();
//	LOGw("report: %d", end - done);
}
