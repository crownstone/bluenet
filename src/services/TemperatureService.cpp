/*
 * TemperatureService.cpp
 *
 *  Created on: Oct 22, 2014
 *      Author: dominik
 */

#include <services/TemperatureService.h>

TemperatureService::TemperatureService() :
		_temperatureCharacteristic(NULL) {

	setUUID(UUID(TEMPERATURE_UUID));
	setName("Temperature Service");

	addTemperatureCharacteristic();
}

void TemperatureService::addTemperatureCharacteristic() {
	_temperatureCharacteristic = new CharacteristicT<int32_t>();
	_temperatureCharacteristic->setUUID(UUID(getUUID(), 0x01));
	_temperatureCharacteristic->setName("Temperature");
	_temperatureCharacteristic->setDefaultValue(0);
	_temperatureCharacteristic->setNotifies(true);

	addCharacteristic(_temperatureCharacteristic);
}

TemperatureService& TemperatureService::createService(Nrf51822BluetoothStack& stack) {
	TemperatureService* svc = new TemperatureService();
    stack.addService(svc);
    return *svc;
}

void TemperatureService::setTemperature(int32_t temperature) {
	*_temperatureCharacteristic = temperature;
}

void TemperatureService::loop() {
	int32_t temp;
	temp = getTemperature();
	setTemperature(temp);
}
