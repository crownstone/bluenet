/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <services/GeneralService.h>

using namespace BLEpp;

// needs to be the same command as defined in the bootloader
#define COMMAND_ENTER_RADIO_BOOTLOADER          1 

GeneralService::GeneralService(Nrf51822BluetoothStack &stack) :
		_stack(&stack),
		_temperatureCharacteristic(NULL), _changeNameCharacteristic(NULL),
		_deviceTypeCharacteristic(NULL), _roomCharacteristic(NULL) {

	setUUID(UUID(GENERAL_UUID));
	setName("General Service");

	LOGi("Create general service");
	characStatus.reserve(5);

	characStatus.push_back( { "Temperature", 	TEMPERATURE_UUID,	true,
		static_cast<addCharacteristicFunc>(&GeneralService::addTemperatureCharacteristic) });
	characStatus.push_back( { "Change Name", 	CHANGE_NAME_UUID,	true,
		static_cast<addCharacteristicFunc>(&GeneralService::addChangeNameCharacteristic) });
	characStatus.push_back( { "Device Type", 	DEVICE_TYPE_UUID,	true,
		static_cast<addCharacteristicFunc>(&GeneralService::addDeviceTypeCharactersitic) });
	characStatus.push_back( { "Room",		ROOM_UUID, 		true,
		static_cast<addCharacteristicFunc>(&GeneralService::addRoomCharacteristic) });
	characStatus.push_back( { "Firmware",		FIRMWARE_UUID, 		true,
		static_cast<addCharacteristicFunc>(&GeneralService::addFirmwareCharacteristic) });
}

void GeneralService::addTemperatureCharacteristic() {
	_temperatureCharacteristic = new CharacteristicT<int32_t>();
	_temperatureCharacteristic->setUUID(UUID(getUUID(), TEMPERATURE_UUID));
	_temperatureCharacteristic->setName("Temperature");
	_temperatureCharacteristic->setDefaultValue(0);
	_temperatureCharacteristic->setNotifies(true);

	addCharacteristic(_temperatureCharacteristic);
}

void GeneralService::addDeviceTypeCharactersitic() {
//	LOGd("create characteristic to read/write device type");
	_deviceTypeCharacteristic = createCharacteristicRef<std::string>();
	(*_deviceTypeCharacteristic)
		.setUUID(UUID(getUUID(), DEVICE_TYPE_UUID))
		.setName("Device Type")
		.setDefaultValue("Unknown")
		.setWritable(true)
		.onWrite([&](const std::string value) -> void {
			LOGi("set device type to: %s", value.c_str());
			// TODO: implement persistent storage of device type
		});
}

void GeneralService::addRoomCharacteristic() {
//	LOGd("create characteristic to read/write room");
	_roomCharacteristic = createCharacteristicRef<std::string>();
	(*_roomCharacteristic)
		.setUUID(UUID(getUUID(), ROOM_UUID))
		.setName("Room")
		.setDefaultValue("Unknown")
		.setWritable(true)
		.onWrite([&](const std::string value) -> void {
			LOGi("set room to: %s", value.c_str());
			// TODO: implement persistent storage of room
		});
}

void GeneralService::addFirmwareCharacteristic() {
	_firmwareCharacteristic = createCharacteristicRef<int32_t>();
	(*_firmwareCharacteristic)
		.setUUID(UUID(getUUID(), FIRMWARE_UUID))
		.setName("Update firmware")
		.setDefaultValue(0)
		.setWritable(true)
		.onWrite([&](const int32_t & value) -> void {
			if (value != 0) {
				LOGi("Update firmware");
				uint8_t err_code;
				err_code = sd_power_gpregret_clr(0xFF);
				APP_ERROR_CHECK(err_code);
				uint32_t cmd = COMMAND_ENTER_RADIO_BOOTLOADER;
				err_code = sd_power_gpregret_set(cmd);
				APP_ERROR_CHECK(err_code);
				sd_nvic_SystemReset();				
			} else {
				LOGi("Update firmware? Write nonzero value");
			}
		});
}

void GeneralService::addChangeNameCharacteristic() {
	_changeNameCharacteristic = createCharacteristicRef<std::string>();
	(*_changeNameCharacteristic)
		.setUUID(UUID(getUUID(), CHANGE_NAME_UUID))
		.setName("Change Name")
		.setDefaultValue(getBLEName())
		.setWritable(true)
		.onWrite([&](const std::string& value) -> void {
			std::string name(value);
			LOGi("Set bluetooth name to: %s", name.c_str());
			setBLEName(name);
		})
		;
}

std::string & GeneralService::getBLEName() {
	_name = "not set";
	if (_stack) {
		_name = _stack->getDeviceName();
	}
	return _name;
}

void GeneralService::setBLEName(std::string &name) {
	if (name.length() > 31) {
		log(ERROR, "Name is too long");
		return;
	} 
	if (_stack) {
		_stack->updateDeviceName(name);
	}
}

GeneralService& GeneralService::createService(Nrf51822BluetoothStack& stack) {
	GeneralService* svc = new GeneralService(stack);
	stack.addService(svc);
	svc->GenericService::addSpecificCharacteristics();
	return *svc;
}

void GeneralService::setTemperature(int32_t temperature) {
	*_temperatureCharacteristic = temperature;
}

void GeneralService::loop() {
	if (_temperatureCharacteristic) {
		int32_t temp;
		temp = getTemperature();
		setTemperature(temp);
	}
}
