/*
 * TemperatureService.cpp
 *
 *  Created on: Oct 22, 2014
 *      Author: dominik
 */

#include <services/TemperatureService.h>

void onTimer(void * p_context) {
	int32_t temperature = getTemperature();
	((TemperatureService*)p_context)->setTemperature(temperature);
}

TemperatureService::TemperatureService() {
	setUUID(UUID(TEMPERATURE_UUID));

//	setName(std::string("Temperature Service"));

	mTemperatureCharacteristic = new CharacteristicT<int32_t>();
	mTemperatureCharacteristic->setUUID(UUID(getUUID(), 0x01));
	mTemperatureCharacteristic->setName("Temperature");
	mTemperatureCharacteristic->setDefaultValue(0);
	mTemperatureCharacteristic->setNotifies(true);

	addCharacteristic(mTemperatureCharacteristic);
}

TemperatureService& TemperatureService::createService(Nrf51822BluetoothStack& stack) {
	TemperatureService* svc = new TemperatureService();
    stack.addService(svc);
    return *svc;
}

void TemperatureService::setTemperature(int32_t temperature) {
	*mTemperatureCharacteristic = temperature;
}

void TemperatureService::loop() {
	int32_t temp;
	temp = getTemperature();
	setTemperature(temp);
}
