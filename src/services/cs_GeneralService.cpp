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

void GeneralService::handleEvent(event_t & event) {
	switch(event.type) {
		case CS_TYPE::STATE_TEMPERATURE: {
			if (_temperatureCharacteristic) {
				TYPIFY(STATE_TEMPERATURE) temperature = *(TYPIFY(STATE_TEMPERATURE)*)event.data;
				*_temperatureCharacteristic = temperature;
			}
			break;
		}
		default: {}
	}
}

