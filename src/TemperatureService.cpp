/*
 * TemperatureService.cpp
 *
 *  Created on: Oct 22, 2014
 *      Author: dominik
 */

#include "TemperatureService.h"
#include "ble_hts.h"
#include "nrf_temp.h"

void onTimer(void * p_context) {
//	LOG_DEBUG("updating temperature");
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

	addCharacteristic(mTemperatureCharacteristic);

	// Initialize timer module, making it use the scheduler
	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
	uint32_t err_code = app_timer_create(&mTemperatureTimer, APP_TIMER_MODE_REPEATED, onTimer);
	APP_ERROR_CHECK(err_code);
}

TemperatureService& TemperatureService::createService(Nrf51822BluetoothStack& stack) {
	TemperatureService* svc = new TemperatureService();
    stack.addService(svc);
    return *svc;
}

void TemperatureService::setTemperature(int32_t temperature) {
//	LOG_DEBUG("setTemperature: %d", temperature);
	*mTemperatureCharacteristic = temperature;
}

void TemperatureService::start() {
	uint32_t err_code = app_timer_start(mTemperatureTimer, ADVDATA_UPDATE_INTERVAL, this);
	APP_ERROR_CHECK(err_code);
}

//void TemperatureService::loop() {
//
//	int32_t temp;
//  getTemperature(temp);
//
//	setTemperature(temp);
//}
