/*
 * TemperatureService.h
 *
 *  Created on: Oct 22, 2014
 *      Author: dominik
 */

#ifndef TEMPERATURESERVICE_H_
#define TEMPERATURESERVICE_H_

#include "temperature.h"
#include "BluetoothLE.h"

extern "C" {
	#include "app_timer.h"
}

#define ADV_INTERVAL_IN_MS 1000
#define APP_TIMER_PRESCALER 0 /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS 1 /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE 4 /**< Size of timer operation queues. */
#define ADVDATA_UPDATE_INTERVAL APP_TIMER_TICKS(ADV_INTERVAL_IN_MS, APP_TIMER_PRESCALER)

#define TEMPERATURE_UUID "f5f93100-59f9-11e4-aa15-123b93f75cba"
//#define TEMPERATURE_UUID "00002220-0000-1000-8000-00805f9b34fb"

using namespace BLEpp;

class TemperatureService: public GenericService {
public:
	TemperatureService();

	void setTemperature(int32_t temperature);

//	void loop();
	void start();

	static TemperatureService& createService(Nrf51822BluetoothStack& stack);

protected:
	CharacteristicT<int32_t>* mTemperatureCharacteristic;
	app_timer_id_t mTemperatureTimer;

};

#endif /* TEMPERATURESERVICE_H_ */
