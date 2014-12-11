/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <services/GeneralService.h>
#include <common/storage.h>

using namespace BLEpp;

GeneralService::GeneralService(Nrf51822BluetoothStack &stack) :
		_stack(&stack),
		_temperatureCharacteristic(NULL), _nameCharacteristic(NULL),
		_deviceTypeCharacteristic(NULL), _roomCharacteristic(NULL) {

	setUUID(UUID(GENERAL_UUID));
	setName("General Service");

	LOGi("Create general service");
	characStatus.reserve(4);

	characStatus.push_back( { "Temperature",
		TEMPERATURE_UUID,
		true,
		static_cast<addCharacteristicFunc>(&GeneralService::addTemperatureCharacteristic) });
	characStatus.push_back( { "Change Name",
		CHANGE_NAME_UUID,
		true,
		static_cast<addCharacteristicFunc>(&GeneralService::addChangeNameCharacteristic) });
	characStatus.push_back( { "Device Type",
		DEVICE_TYPE_UUID,
		true,
		static_cast<addCharacteristicFunc>(&GeneralService::addDeviceTypeCharactersitic) });
	characStatus.push_back( { "Room",
		ROOM_UUID,
		true,
		static_cast<addCharacteristicFunc>(&GeneralService::addRoomCharacteristic) });

	Storage::getInstance().getHandle(PS_ID_GENERAL_SERVICE, _storageHandle);
	loadPersistentStorage();
}

void GeneralService::loadPersistentStorage() {
	Storage::getInstance().getStruct(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void GeneralService::savePersistentStorage() {
	Storage::getInstance().setStruct(_storageHandle, &_storageStruct, sizeof(_storageStruct));
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
	_storageStruct.getString(_storageStruct.device_type, _type, "Unknown");

//	LOGd("create characteristic to read/write device type");
	_deviceTypeCharacteristic = createCharacteristicRef<std::string>();
	(*_deviceTypeCharacteristic)
		.setUUID(UUID(getUUID(), DEVICE_TYPE_UUID))
		.setName("Device Type")
		.setDefaultValue(_type)
		.setWritable(true)
		.onWrite([&](const std::string value) -> void {
			LOGi("set device type to: %s", value.c_str());
			_storageStruct.setString(value, _storageStruct.device_type);
			savePersistentStorage();
		});
}

void GeneralService::addRoomCharacteristic() {
	_storageStruct.getString(_storageStruct.room, _room, "Unknown");

//	LOGd("create characteristic to read/write room");
	_roomCharacteristic = createCharacteristicRef<std::string>();
	(*_roomCharacteristic)
		.setUUID(UUID(getUUID(), ROOM_UUID))
		.setName("Room")
		.setDefaultValue(_room)
		.setWritable(true)
		.onWrite([&](const std::string value) -> void {
			LOGi("set room to: %s", value.c_str());
			_storageStruct.setString(value, _storageStruct.room);
			savePersistentStorage();
		});
}

void GeneralService::addChangeNameCharacteristic() {
	_storageStruct.getString(_storageStruct.device_name, _name, getBLEName());
	setBLEName(_name);

	_nameCharacteristic = createCharacteristicRef<std::string>();
	(*_nameCharacteristic)
		.setUUID(UUID(getUUID(), CHANGE_NAME_UUID))
		.setName("Change Name")
		.setDefaultValue(_name)
		.setWritable(true)
		.onWrite([&](const std::string& value) -> void {
			LOGi("Set bluetooth name to: %s", value.c_str());
			setBLEName(value);
			_storageStruct.setString(value, _storageStruct.device_name);
			savePersistentStorage();
		});
}

std::string & GeneralService::getBLEName() {
	_name = "Unknown";
	if (_stack) {
		return _name = _stack->getDeviceName();
	}
	return _name;
}

void GeneralService::setBLEName(const std::string &name) {
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
