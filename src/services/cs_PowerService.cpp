/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <services/cs_PowerService.h>

#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_PowerSampling.h>
#include <structs/buffer/cs_MasterBuffer.h>
#include <protocol/cs_StateTypes.h>

using namespace BLEpp;

PowerService::PowerService() : EventListener(),
		_pwmCharacteristic(NULL),
		_relayCharacteristic(NULL),
		_powerConsumptionCharacteristic(NULL),
		_powerSamplesCharacteristic(NULL)
{
	EventDispatcher::getInstance().addListener(this);

	setUUID(UUID(POWER_UUID));

	setName(BLE_SERVICE_POWER);

	// todo: enable again if tick is needed
//	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)PowerService::staticTick);
}

void PowerService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, BLE_SERVICE_POWER);

#if CHAR_PWM==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_PWM);
	addPWMCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_PWM);
#endif

#if CHAR_RELAY==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_RELAY);
	addRelayCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_RELAY);
#endif

#if CHAR_SAMPLE_CURRENT==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_POWER_SAMPLE);
	addPowerSamplesCharacteristic();
	LOGi(FMT_CHAR_ADD, STR_CHAR_CURRENT_CONSUMPTION);
	addPowerConsumptionCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_PWM);
	LOGi(FMT_CHAR_SKIP, STR_CHAR_CURRENT_CONSUMPTION);
#endif

	addCharacteristicsDone();
}

//void PowerService::tick() {
//	scheduleNextTick();
//}

//void PowerService::scheduleNextTick() {
//	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(POWER_SERVICE_UPDATE_FREQUENCY), this);
//}

void PowerService::addPWMCharacteristic() {
	_pwmCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_pwmCharacteristic);
	_pwmCharacteristic->setUUID(UUID(getUUID(), PWM_UUID));
	_pwmCharacteristic->setName(BLE_CHAR_PWM);
	_pwmCharacteristic->setDefaultValue(255);
	_pwmCharacteristic->setWritable(true);
	_pwmCharacteristic->onWrite([&](const uint8_t accessLevel, const uint8_t& value) -> void {
		CommandHandler::getInstance().handleCommand(CMD_PWM, (buffer_ptr_t)&value, 1);
	});
}

void PowerService::addRelayCharacteristic() {

	_relayCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_relayCharacteristic);
	_relayCharacteristic->setUUID(UUID(getUUID(), RELAY_UUID));
	_relayCharacteristic->setName(BLE_CHAR_RELAY);
	_relayCharacteristic->setDefaultValue(255);
	_relayCharacteristic->setWritable(true);
	_relayCharacteristic->onWrite([&](const uint8_t accessLevel, const uint8_t& value) -> void {
		CommandHandler::getInstance().handleCommand(CMD_RELAY, (buffer_ptr_t)&value, 1);
	});
}

void PowerService::addPowerSamplesCharacteristic() {
	_powerSamplesCharacteristic = new Characteristic<uint8_t*>();
	addCharacteristic(_powerSamplesCharacteristic);
	_powerSamplesCharacteristic->setUUID(UUID(getUUID(), POWER_SAMPLES_UUID));
	_powerSamplesCharacteristic->setName(BLE_CHAR_POWER_SAMPLES);
	_powerSamplesCharacteristic->setWritable(false);
	_powerSamplesCharacteristic->setNotifies(true);

	buffer_ptr_t buffer = NULL;
	uint16_t size = 0;
	PowerSampling::getInstance().getBuffer(buffer, size);
//	LOGd("buffer=%u", buffer);

	_powerSamplesCharacteristic->setValue(buffer);
	_powerSamplesCharacteristic->setMaxGattValueLength(size);
	_powerSamplesCharacteristic->setValueLength(0); //! Initialize with 0 length, to indicate it's invalid data.
}

void PowerService::addPowerConsumptionCharacteristic() {
	_powerConsumptionCharacteristic = new Characteristic<int32_t>();
	addCharacteristic(_powerConsumptionCharacteristic);
	_powerConsumptionCharacteristic->setUUID(UUID(getUUID(), POWER_CONSUMPTION_UUID));
	_powerConsumptionCharacteristic->setName(BLE_CHAR_POWER_CONSUMPTION);
	_powerConsumptionCharacteristic->setDefaultValue(0);
	_powerConsumptionCharacteristic->setWritable(false);
	_powerConsumptionCharacteristic->setNotifies(true);
}

void PowerService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case EVT_POWER_OFF:
	case EVT_POWER_ON: {
		if (_pwmCharacteristic) {
//			LOGi("pwm update");
			(*_pwmCharacteristic) = *(uint8_t*)p_data;
		}
		break;
	}
	case EVT_POWER_SAMPLES_START: {
		if (_powerSamplesCharacteristic) {
//			LOGd("power samples update");
			_powerSamplesCharacteristic->setValueLength(0);
			_powerSamplesCharacteristic->updateValue();
		}
		break;
	}
	case EVT_POWER_SAMPLES_END: {
		if (_powerSamplesCharacteristic) {
//			LOGd("power samples update");
//			LOGi("set power samples %d", length);
			_powerSamplesCharacteristic->setValueLength(length);
			_powerSamplesCharacteristic->updateValue();
		}
		break;
	}
	case STATE_POWER_USAGE: {
		if (_powerConsumptionCharacteristic) {
//			LOGi("power consumption update");
			(*_powerConsumptionCharacteristic) = *(int32_t*)p_data;
		}
	}
	}
}


