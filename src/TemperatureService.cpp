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
	mTemperatureCharacteristic->setNotifies(true);

	addCharacteristic(mTemperatureCharacteristic);

//    nrf_temp_init();
//	// Initialize timer module, making it use the scheduler
//	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
//	uint32_t err_code = app_timer_create(&mTemperatureTimer, APP_TIMER_MODE_REPEATED, onTimer);
//	APP_ERROR_CHECK(err_code);
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

//void TemperatureService::start() {
//	uint32_t err_code = app_timer_start(mTemperatureTimer, ADVDATA_UPDATE_INTERVAL, this);
//	APP_ERROR_CHECK(err_code);
//}

int32_t count = 0;
void TemperatureService::loop() {

	int32_t volatile temp;
	temp = getTemperature();

//	int32_t volatile temp;
//    NRF_TEMP->TASKS_START = 1; /** Start the temperature measurement. */
//
//    /* Busy wait while temperature measurement is not finished, you can skip waiting if you enable interrupt for DATARDY event and read the result in the interrupt. */
//    /*lint -e{845} // A zero has been given as right argument to operator '|'" */
//    while (NRF_TEMP->EVENTS_DATARDY == 0)
//    {
//        // Do nothing.
//    }
//    NRF_TEMP->EVENTS_DATARDY = 0;
//
//    /**@note Workaround for PAN_028 rev2.0A anomaly 29 - TEMP: Stop task clears the TEMP register. */
//    temp = (nrf_temp_read()/4);
//
//    /**@note Workaround for PAN_028 rev2.0A anomaly 30 - TEMP: Temp module analog front end does not power down when DATARDY event occurs. */
//    NRF_TEMP->TASKS_STOP = 1; /** Stop the temperature measurement. */

	setTemperature(temp);
}
