/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "common/cs_Storage.h"
#include "services/cs_GeneralService.h"

#if MESHING==1 
#include <protocol/cs_Mesh.h>
#endif

using namespace BLEpp;

// needs to be the same command as defined in the bootloader
#define COMMAND_ENTER_RADIO_BOOTLOADER          1 

/* General Service for the Crownstone
 *
 * There are several characteristics that fit into the general service description. There is a characteristic
 * that measures the temperature, there are several characteristics that defines the crownstone, namely by
 * name, by type, or by location (room), and there is a characteristic to update its firmware.
 *
 * If meshing is enabled, it is also possible to send a message into the mesh network using a characteristic.
 */
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
		true,
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

/* Get a handle to the persistent storage struct and load it from FLASH.
 *
 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that writing less
 * than a minimal block strains the memory just as much as flashing the entire block. Hence, there is an
 * entire struct that can be filled and flashed at once.
 */
void GeneralService::loadPersistentStorage() {
	Storage::getInstance().getStruct(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

/* Save to FLASH.
 */
void GeneralService::savePersistentStorage() {
	Storage::getInstance().setStruct(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

/* Enable the temperature characteristic.
 */
void GeneralService::addTemperatureCharacteristic() {
	_temperatureCharacteristic = new CharacteristicT<int32_t>();
	_temperatureCharacteristic->setUUID(UUID(getUUID(), TEMPERATURE_UUID));
	_temperatureCharacteristic->setName("Temperature");
	_temperatureCharacteristic->setDefaultValue(0);
	_temperatureCharacteristic->setNotifies(true);

	addCharacteristic(_temperatureCharacteristic);
}

/* Enable the device type characteristic.
 */
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

/* Enable the room characteristic.
 *
 * The room needs to be set by the user. There is not yet functionality in place in the crownstone software to
 * figure this out for itself.
 */
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

/* Enable the firmware upgrade characteristic.
 */
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

/* Enable the mesh characteristic.
 */
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

/* Enable the change name characteristic.
 */
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

/* Retrieve the Bluetooth name from the object representing the BLE stack.
 */
std::string & GeneralService::getBLEName() {
	_name = "Unknown";
	if (_stack) {
		return _name = _stack->getDeviceName();
	}
	return _name;
}

/* Write the Bluetooth name to the object representing the BLE stack.
 *
 * This updates the Bluetooth name immediately, however, it does not update the name persistently. It has to
 * be written to FLASH in that case.
 */
void GeneralService::setBLEName(const std::string &name) {
	if (name.length() > 31) {
		log(ERROR, "Name is too long");
		return;
	}
	if (_stack) {
		_stack->updateDeviceName(name);
	}
}

/** Helper function to generate a GeneralService object
 */
GeneralService& GeneralService::createService(Nrf51822BluetoothStack& stack) {
	GeneralService* svc = new GeneralService(stack);
	stack.addService(svc);
	svc->GenericService::addSpecificCharacteristics();
	return *svc;
}

/* Update the temperature characteristic.
 */
void GeneralService::setTemperature(int32_t temperature) {
	*_temperatureCharacteristic = temperature;
}

/* Perform non urgent functionality every main loop.
 *
 * Every component has a "tick" function which is for non-urgent things. Urgent matters have to be resolved
 * immediately in interrupt service handlers. The temperature for example is updated every tick, because timing
 * is not important for this at all.
 */
void GeneralService::tick() {
	if (_temperatureCharacteristic) {
		int32_t temp;
		temp = getTemperature();
		setTemperature(temp);
	}
}
