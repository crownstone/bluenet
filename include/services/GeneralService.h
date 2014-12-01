/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef GENERALSERVICE_H_
#define GENERALSERVICE_H_

#include <processing/temperature.h>
#include "BluetoothLE.h"
#include <vector>
#include <characteristics/charac_config.h>

#define GENERAL_UUID "f5f90000-59f9-11e4-aa15-123b93f75cba"

class GeneralService: public BLEpp::GenericService {
public:
	GeneralService(BLEpp::Nrf51822BluetoothStack &stack);

	void addSpecificCharacteristics();

	void setTemperature(int32_t temperature);

	void loop();

	static GeneralService& createService(BLEpp::Nrf51822BluetoothStack& stack);

protected:
	BLEpp::Nrf51822BluetoothStack* _stack;

	// References to characteristics
	BLEpp::CharacteristicT<int32_t>* _temperatureCharacteristic;
	BLEpp::Characteristic<std::string>* _changeNameCharacteristic; 
	BLEpp::Characteristic<std::string>* _deviceTypeCharacteristic;
	BLEpp::Characteristic<std::string>* _roomCharacteristic;

	// Functions to add the characteristics
	void addTemperatureCharacteristic();
	void addChangeNameCharacteristic();
	void addDeviceTypeCharactersitic();
	void addRoomCharacteristic();

	// Helper functions
	std::string & getBLEName();
	void setBLEName(std::string &name);
private:
	std::string _name;
};

#endif /* GENERALSERVICE_H_ */
