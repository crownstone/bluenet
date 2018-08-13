/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#include <processing/cs_CommandHandler.h>
#include <services/cs_GeneralService.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_MasterBuffer.h>

GeneralService::GeneralService() : EventListener(),
		_temperatureCharacteristic(NULL), _resetCharacteristic(NULL)
{
	EventDispatcher::getInstance().addListener(this);

	setUUID(UUID(GENERAL_UUID));
	setName(BLE_SERVICE_GENERAL);

//	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) GeneralService::staticTick);
}

void GeneralService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, BLE_SERVICE_GENERAL);

#if CHAR_TEMPERATURE==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_TEMPERATURE);
	addTemperatureCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, CHAR_TEMPERATURE);
#endif

#if CHAR_RESET==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_RESET);
	addResetCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, CHAR_RESET);
#endif

	addCharacteristicsDone();
}

//void GeneralService::tick() {
//	scheduleNextTick();
//}

//void GeneralService::scheduleNextTick() {
//	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(GENERAL_SERVICE_UPDATE_FREQUENCY), this);
//}

void GeneralService::addTemperatureCharacteristic() {
	_temperatureCharacteristic = new Characteristic<int32_t>();
	addCharacteristic(_temperatureCharacteristic);

	_temperatureCharacteristic->setUUID(UUID(getUUID(), TEMPERATURE_UUID));
	_temperatureCharacteristic->setName(BLE_CHAR_TEMPERATURE);
	_temperatureCharacteristic->setDefaultValue(0);
	_temperatureCharacteristic->setNotifies(true);
}

void GeneralService::addResetCharacteristic() {
	_resetCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_resetCharacteristic);

	_resetCharacteristic->setUUID(UUID(getUUID(), RESET_UUID));
	_resetCharacteristic->setName(BLE_CHAR_RESET);
	_resetCharacteristic->setDefaultValue(0);
	_resetCharacteristic->setWritable(true);
	_resetCharacteristic->onWrite([&](const uint8_t accessLevel, const uint8_t& value, uint16_t length) -> void {
		CommandHandler::getInstance().resetDelayed(value);
	});
}

void GeneralService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case STATE_TEMPERATURE: {
		if (_temperatureCharacteristic) {
			int32_t temperature = *(int32_t*)p_data;
//			LOGd("udpate temperature characteristic: %d", temperature);
			*_temperatureCharacteristic = temperature;
		}
#ifdef MICRO_VIEW
		//! Update temperature at the display
		write("1 %i\r\n", temp);
#endif
		break;
	}
	}
}

