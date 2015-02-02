/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <services/GeneralService.h>
#include <common/storage.h>

#if MESHING==1 
#include <protocol/mesh.h>
#endif

using namespace BLEpp;

// needs to be the same command as defined in the bootloader
#define COMMAND_ENTER_RADIO_BOOTLOADER          1 

GeneralService::GeneralService(Nrf51822BluetoothStack &stack) :
		_stack(&stack),
		_temperatureCharacteristic(NULL), _nameCharacteristic(NULL),
		_deviceTypeCharacteristic(NULL), _roomCharacteristic(NULL),
		_firmwareCharacteristic(NULL) {

	setUUID(UUID(GENERAL_UUID));
	setName("General Service");

	LOGi("Create general service");
	characStatus.reserve(5);

	characStatus.push_back( { "Temperature",
		TEMPERATURE_UUID,
		true,
		static_cast<addCharacteristicFunc>(&GeneralService::addTemperatureCharacteristic) });
	characStatus.push_back( { "Change Name",
		CHANGE_NAME_UUID,
		false,
		static_cast<addCharacteristicFunc>(&GeneralService::addChangeNameCharacteristic) });
	characStatus.push_back( { "Device Type",
		DEVICE_TYPE_UUID,
		false,
		static_cast<addCharacteristicFunc>(&GeneralService::addDeviceTypeCharacteristic) });
	characStatus.push_back( { "Room",
		ROOM_UUID,
		false,
		static_cast<addCharacteristicFunc>(&GeneralService::addRoomCharacteristic) });
	characStatus.push_back( { "Firmware",
		FIRMWARE_UUID, 
		true,
		static_cast<addCharacteristicFunc>(&GeneralService::addFirmwareCharacteristic) });
#if MESHING==1
	characStatus.push_back( { "Mesh",
		MESH_UUID, 
		true,
		static_cast<addCharacteristicFunc>(&GeneralService::addMeshCharacteristic) });
#endif

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

void GeneralService::addDeviceTypeCharacteristic() {
	Storage::getString(_storageStruct.device_type, _type, "Unknown");

//	LOGd("create characteristic to read/write device type");
	_deviceTypeCharacteristic = createCharacteristicRef<std::string>();
	(*_deviceTypeCharacteristic)
		.setUUID(UUID(getUUID(), DEVICE_TYPE_UUID))
		.setName("Device Type")
		.setDefaultValue(_type)
		.setWritable(true)
		.onWrite([&](const std::string value) -> void {
			LOGi("set device type to: %s", value.c_str());
			Storage::setString(value, _storageStruct.device_type);
			savePersistentStorage();
		});
}

void GeneralService::addRoomCharacteristic() {
	Storage::getString(_storageStruct.room, _room, "Unknown");

//	LOGd("create characteristic to read/write room");
	_roomCharacteristic = createCharacteristicRef<std::string>();
	(*_roomCharacteristic)
		.setUUID(UUID(getUUID(), ROOM_UUID))
		.setName("Room")
		.setDefaultValue(_room)
		.setWritable(true)
		.onWrite([&](const std::string value) -> void {
			LOGi("set room to: %s", value.c_str());
			Storage::setString(value, _storageStruct.room);
			savePersistentStorage();
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

#if MESHING==1
void GeneralService::addMeshCharacteristic() {
	_meshCharacteristic = createCharacteristicRef<MeshMessage>();
	(*_meshCharacteristic)
		.setUUID(UUID(getUUID(), MESH_UUID))
		.setName("Mesh")
		.setWritable(true)
		.onWrite([&](const MeshMessage & value) -> void {
			LOGi("Send mesh message");
			//uint8_t id = value.id(); // not used
			uint8_t handle = value.handle();
			uint8_t val = value.value();
			CMesh &mesh = CMesh::getInstance();
			mesh.send(handle, val);
		});
}
#endif

void GeneralService::addChangeNameCharacteristic() {
	Storage::getString(_storageStruct.device_name, _name, getBLEName());
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
			Storage::setString(value, _storageStruct.device_name);
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

void GeneralService::tick() {
	if (_temperatureCharacteristic) {
		int32_t temp;
		temp = getTemperature();
		setTemperature(temp);
	}
}
