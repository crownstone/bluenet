/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef GENERALSERVICE_H_
#define GENERALSERVICE_H_

#include "BluetoothLE.h"
#include <processing/temperature.h>
#include <vector>
#include <characteristics/charac_config.h>
#include <common/storage.h>

#define GENERAL_UUID "f5f90000-59f9-11e4-aa15-123b93f75cba"

class GeneralService: public BLEpp::GenericService {
public:
	GeneralService(BLEpp::Nrf51822BluetoothStack &stack);

	void setTemperature(int32_t temperature);

	void loop();

	static GeneralService& createService(BLEpp::Nrf51822BluetoothStack& stack);

protected:
	BLEpp::Nrf51822BluetoothStack* _stack;

	// References to characteristics
	BLEpp::CharacteristicT<int32_t>* _temperatureCharacteristic;
	BLEpp::Characteristic<std::string>* _nameCharacteristic;
	BLEpp::Characteristic<std::string>* _deviceTypeCharacteristic;
	BLEpp::Characteristic<std::string>* _roomCharacteristic;
	BLEpp::Characteristic<int32_t>* _firmwareCharacteristic;

	// Functions to add the characteristics
	void addTemperatureCharacteristic();
	void addChangeNameCharacteristic();
	void addDeviceTypeCharactersitic();
	void addRoomCharacteristic();
	void addFirmwareCharacteristic();

	// Helper functions
	std::string&  getBLEName();
	void setBLEName(const std::string &name);

	void loadPersistentStorage();
	void savePersistentStorage();
private:
	std::string _name;
	std::string _room;
	std::string _type;

	pstorage_handle_t _storageHandle;
	ps_general_service_t _storageStruct;
};

#endif /* GENERALSERVICE_H_ */
