/*
 * TemperatureService.h
 *
 *  Created on: Oct 22, 2014
 *      Author: dominik
 */

#ifndef TEMPERATURESERVICE_H_
#define TEMPERATURESERVICE_H_

#include <processing/temperature.h>
#include "BluetoothLE.h"

#define TEMPERATURE_UUID "f5f93100-59f9-11e4-aa15-123b93f75cba"
//#define TEMPERATURE_UUID "00002220-0000-1000-8000-00805f9b34fb"

using namespace BLEpp;

class TemperatureService: public GenericService {
public:
	TemperatureService();

	void setTemperature(int32_t temperature);

	void loop();

	static TemperatureService& createService(Nrf51822BluetoothStack& stack);

protected:
	CharacteristicT<int32_t>* _temperatureCharacteristic;

	void addTemperatureCharacteristic();

};

#endif /* TEMPERATURESERVICE_H_ */
