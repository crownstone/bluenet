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

DeviceInformationService::DeviceInformationService()
{
	setUUID(UUID(BLE_UUID_DEVICE_INFORMATION_SERVICE));
	setName(BLE_SERVICE_DEVICE_INFORMATION);

}

void DeviceInformationService::createCharacteristics() {
	LOGi("Create device information service");

	LOGi("add hardware revision characteristic");
	addHardwareRevisionCharacteristic();

	LOGi("add firmware revision characteristic");
	addFirmwareRevisionCharacteristic();

#ifdef GIT_BRANCH
	LOGi("add software revision characteristic");
	addSoftwareRevisionCharacteristic();
#endif

	addCharacteristicsDone();
}

void DeviceInformationService::addHardwareRevisionCharacteristic() {
	BLEpp::Characteristic<std::string>* _hardwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_hardwareRevisionCharacteristic);
	_hardwareRevisionCharacteristic->setUUID(BLE_UUID_HARDWARE_REVISION_STRING_CHAR);
	_hardwareRevisionCharacteristic->setName(BLE_CHAR_HARDWARE_REVISION);
	_hardwareRevisionCharacteristic->setDefaultValue(get_hardware_revision());
	_hardwareRevisionCharacteristic->setWritable(false);
}

void DeviceInformationService::addFirmwareRevisionCharacteristic() {
	BLEpp::Characteristic<std::string>* _firmwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_firmwareRevisionCharacteristic);
	_firmwareRevisionCharacteristic->setUUID(BLE_UUID_FIRMWARE_REVISION_STRING_CHAR);
	_firmwareRevisionCharacteristic->setName(BLE_CHAR_FIRMWARE_REVISION);
	_firmwareRevisionCharacteristic->setWritable(false);
#ifdef GIT_HASH
	_firmwareRevisionCharacteristic->setDefaultValue(STRINGIFY(GIT_HASH));
#else
	_firmwareRevisionCharacteristic->setDefaultValue(STRINGIFY(FIRMWARE_VERSION));
#endif
}

void DeviceInformationService::addSoftwareRevisionCharacteristic() {
	BLEpp::Characteristic<std::string>* _softwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_softwareRevisionCharacteristic);
	_softwareRevisionCharacteristic->setUUID(BLE_UUID_SOFTWARE_REVISION_STRING_CHAR);
	_softwareRevisionCharacteristic->setName(BLE_CHAR_SOFTWARE_REVISION);
#ifdef GIT_BRANCH
	_softwareRevisionCharacteristic->setDefaultValue(STRINGIFY(GIT_BRANCH));
#else
//	_softwareRevisionCharacteristic->setDefaultValue(STRINGIFY(SOFTWARE_REVISION));
#endif
	_softwareRevisionCharacteristic->setWritable(false);
}
