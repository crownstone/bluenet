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
#include <cfg/cs_StateVars.h>

#include <processing/cs_CommandHandler.h>
#include <processing/cs_PowerSampling.h>

#include <events/cs_EventListener.h>

using namespace BLEpp;

PowerService::PowerService() : EventListener(),
		_pwmCharacteristic(NULL),
		_relayCharacteristic(NULL),
		_sampleCurrentCharacteristic(NULL),
		_powerConsumptionCharacteristic(NULL),
		_currentCurveCharacteristic(NULL),
//		_currentLimitCharacteristic(NULL),
		_currentLimitVal(0),
		_currentLimitInitialized(false)
{
	EventDispatcher::getInstance().addListener(this);

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

	addCharacteristicsDone();
}

/**
 * TODO: We should only need to do this once on startup.
 */
void PowerService::tick() {
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
		CommandHandler::getInstance().handleCommand(CMD_PWM, (buffer_ptr_t)&value, 4);
	});
}

void PowerService::addRelayCharacteristic() {

	_relayCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_relayCharacteristic);
	_relayCharacteristic->setUUID(UUID(getUUID(), RELAY_UUID));
	_relayCharacteristic->setName(BLE_CHAR_RELAY);
	_relayCharacteristic->setDefaultValue(255);
	_relayCharacteristic->setWritable(true);
	_relayCharacteristic->onWrite([&](const uint8_t& value) -> void {
		CommandHandler::getInstance().handleCommand(CMD_SWITCH, (buffer_ptr_t)&value, 4);
	});
}

////! Do we really want to use the PWM for this, or just set the pin to zero?
////! TODO: turn off normally, but make sure we enable the completely PWM again on request
//void PowerService::turnOff() {
//	//! update pwm characteristic so that the current value can be read from the characteristic
//	if (_pwmCharacteristic != NULL) {
//		(*_pwmCharacteristic) = 0;
//	}
//	PWM::getInstance().setValue(0, 0);
//}
//
////! Do we really want to use the PWM for this, or just set the pin to zero?
////! TODO: turn on normally, but make sure we enable the completely PWM again on request
//void PowerService::turnOn() {
//	//! update pwm characteristic so that the current value can be read from the characteristic
//	if (_pwmCharacteristic != NULL) {
//		(*_pwmCharacteristic) = 255;
//	}
//	PWM::getInstance().setValue(0, 255);
//}
//
//void PowerService::dim(uint8_t value) {
//	//! update pwm characteristic so that the current value can be read from the characteristic
//	if (_pwmCharacteristic != NULL) {
//		(*_pwmCharacteristic) = value;
//	}
//	PWM::getInstance().setValue(0, value);
//}

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

		PowerSampling::getInstance().sampleCurrent(value);
	});
}

void PowerService::addCurrentCurveCharacteristic() {
	_currentCurveCharacteristic = new Characteristic<uint8_t*>();
	addCharacteristic(_currentCurveCharacteristic);
	_currentCurveCharacteristic->setUUID(UUID(getUUID(), CURRENT_CURVE_UUID));
	_currentCurveCharacteristic->setName("Current Curve");
	_currentCurveCharacteristic->setWritable(false);
	_currentCurveCharacteristic->setNotifies(true);

//	_powerCurve = new PowerCurve<uint16_t>();
	MasterBuffer& mb = MasterBuffer::getInstance();
	uint8_t *buffer = NULL;
	uint16_t size = 0;
	mb.getBuffer(buffer, size);
	LOGd("Assign buffer of size %i to current curve", size);
//	_powerCurve->assign(buffer, size);

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

//uint8_t PowerService::getCurrentLimit() {
//	Storage::getUint8(Settings::getInstance().getConfig().currentLimit, _currentLimitVal, 0);
//	LOGi("Obtained current limit from FLASH: %i", _currentLimitVal);
//	return _currentLimitVal;
//}

////! TODO: doesn't work for now
//void PowerService::setCurrentLimit(uint8_t value) {
//	LOGi("Set current limit to: %i", value);
//	_currentLimitVal = value;
//	//if (!_currentLimitInitialized) {
//	//_currentLimit.init();
//	//_currentLimitInitialized = true;
//	//}
////	LPComp::getInstance().stop();
////	LPComp::getInstance().config(PIN_AIN_LPCOMP, _currentLimitVal, LPComp::LPC_UP);
////	LPComp::getInstance().start();
//	LOGi("Write value to persistent memory");
//	Storage::setUint8(_currentLimitVal, Settings::getInstance().getConfig().currentLimit);
//	Settings::getInstance().savePersistentStorage();
//}

/**
 * The characteristic that writes a current limit to persistent memory.
 *
 * TODO: Check https://devzone.nordicsemi.com/question/1745/how-to-handle-flashwrit-in-an-safe-way/
 *       Writing to persistent memory should be done between connection/advertisement events...
 */
//! TODO -oDE: make part of configuration characteristic
//void PowerService::addCurrentLimitCharacteristic() {
//	_currentLimitCharacteristic = new Characteristic<uint8_t>();
//	addCharacteristic(_currentLimitCharacteristic);
//	_currentLimitCharacteristic->setNotifies(true);
//	_currentLimitCharacteristic->setUUID(UUID(getUUID(), CURRENT_LIMIT_UUID));
//	_currentLimitCharacteristic->setName("Current Limit");
//	_currentLimitCharacteristic->setDefaultValue(getCurrentLimit());
//	_currentLimitCharacteristic->setWritable(true);
//	_currentLimitCharacteristic->onWrite([&](const uint8_t& value) -> void {
//		setCurrentLimit(value);
//		_currentLimitInitialized = true;
//	});
//
//	//! TODO: we have to delay the init, since there is a spike on the AIN pin at startup!
//	//! For now: init at onWrite, so we can still test it.
//	//	_currentLimit.start(&_currentLimitVal);
//	//	_currentLimit.init();
//}

//static int tmp_cnt = 100;
//static int tick_cnt = 100;

void PowerService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case EVT_POWER_OFF:
	case EVT_POWER_ON: {
		if (_pwmCharacteristic) {
			(*_pwmCharacteristic) = *(uint8_t*)p_data;
		}
		break;
	}
	case EVT_POWER_CURVE: {
		if (_currentCurveCharacteristic) {
			_currentCurveCharacteristic->setDataLength(length);
			_currentCurveCharacteristic->notify();
		}
		break;
	}
	case EVT_POWER_CONSUMPTION: {
		if (_powerConsumptionCharacteristic) {
			(*_powerConsumptionCharacteristic) = *(uint32_t*)p_data;
		}
	}
	}
}

