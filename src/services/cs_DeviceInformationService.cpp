/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "services/cs_DeviceInformationService.h"

//
#include <ble/cs_Nordic.h>
#include "cfg/cs_Boards.h"
#include "cfg/cs_UuidConfig.h"

#include <drivers/cs_Serial.h>
#include <util/cs_Utils.h>

using namespace BLEpp;

inline std::string get_hardware_revision(void) {
	char hardware_revision[32]; \
	sprintf(hardware_revision, "%X-%X", HARDWARE_BOARD, HARDWARE_VERSION);
	return std::string(hardware_revision);
}

DeviceInformationService::DeviceInformationService() :
		_hardwareRevisionCharacteristic(NULL),
		_firmwareRevisionCharacteristic(NULL)
{

	setUUID(UUID(BLE_UUID_DEVICE_INFORMATION_SERVICE));

	setName("Device Information");

	init();
}

void DeviceInformationService::init() {
	LOGi("Create device information service");

	LOGi("add hardware revision characteristic");
	addHardwareRevisionCharacteristic();

	LOGi("add firmware revision characteristic");
	addFirmwareRevisionCharacteristic();
}

void DeviceInformationService::addHardwareRevisionCharacteristic() {
	_hardwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_hardwareRevisionCharacteristic);
	_hardwareRevisionCharacteristic->setUUID(BLE_UUID_HARDWARE_REVISION_STRING_CHAR);
	_hardwareRevisionCharacteristic->setName("Hardware Revision");
	_hardwareRevisionCharacteristic->setDefaultValue(get_hardware_revision());
	_hardwareRevisionCharacteristic->setWritable(false);
}

void DeviceInformationService::addFirmwareRevisionCharacteristic() {
	_firmwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_firmwareRevisionCharacteristic);
	_firmwareRevisionCharacteristic->setUUID(BLE_UUID_FIRMWARE_REVISION_STRING_CHAR);
	_firmwareRevisionCharacteristic->setName("Firmware Revision");
	_firmwareRevisionCharacteristic->setDefaultValue(STRINGIFY(FIRMWARE_VERSION));
	_firmwareRevisionCharacteristic->setWritable(false);
}
