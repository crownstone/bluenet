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
#include "cfg/cs_HardwareVersions.h"
#include "cfg/cs_UuidConfig.h"

#include <drivers/cs_Serial.h>
#include <util/cs_Utils.h>

extern "C" {
#include "nrf52.h"
}

inline std::string get_hardware_revision(void) {

	// get Production run
	std::string production_run = "0000";

	// get Housing ID
	std::string housing_id = "0000";

	// reserved
	std::string reserved = "00000000";

	uint32_t variant = BLEutil::convertEndian32(NRF_FICR->INFO.VARIANT);

	// get nordic chip version
	char nordic_chip_version[6];
	if (NRF_FICR->INFO.PACKAGE == 0x2000) {
		sprintf(nordic_chip_version, "QF%.4s", (char*)&variant);
	} else if (NRF_FICR->INFO.PACKAGE == 0x2001) {
		sprintf(nordic_chip_version, "CI%.4s", (char*)&variant);
	} else {
		sprintf(nordic_chip_version, "??%.4s", (char*)&variant);
	}

	char hardware_revision[33];
	sprintf(hardware_revision, "%11.11s%.4s%.4s%.8s%.6s", get_hardware_version(), production_run.c_str(), housing_id.c_str(), reserved.c_str(), nordic_chip_version);
	return std::string(hardware_revision, 33);
}

DeviceInformationService::DeviceInformationService()
{
	setUUID(UUID(BLE_UUID_DEVICE_INFORMATION_SERVICE));
	setName(BLE_SERVICE_DEVICE_INFORMATION);

}

void DeviceInformationService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, BLE_SERVICE_DEVICE_INFORMATION);

	LOGi(FMT_CHAR_ADD, STR_CHAR_HARDWARE_REVISION);
	addHardwareRevisionCharacteristic();

	LOGi(FMT_CHAR_ADD, STR_CHAR_FIRMWARE_REVISION);
	addFirmwareRevisionCharacteristic();

#ifdef GIT_BRANCH
	LOGi(FMT_CHAR_ADD, STR_CHAR_SOFTWARE_REVISION);
	addSoftwareRevisionCharacteristic();
#endif

	addCharacteristicsDone();
}

void DeviceInformationService::addHardwareRevisionCharacteristic() {
	Characteristic<std::string>* _hardwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_hardwareRevisionCharacteristic);

	std::string hardware_revision = get_hardware_revision();

	_hardwareRevisionCharacteristic->setUUID(BLE_UUID_HARDWARE_REVISION_STRING_CHAR);
	_hardwareRevisionCharacteristic->setName(BLE_CHAR_HARDWARE_REVISION);
	_hardwareRevisionCharacteristic->setDefaultValue(hardware_revision);
	_hardwareRevisionCharacteristic->setWritable(false);
	_hardwareRevisionCharacteristic->setMaxStringLength(hardware_revision.size());
	_hardwareRevisionCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_hardwareRevisionCharacteristic->setValueLength(hardware_revision.size());
}

void DeviceInformationService::addFirmwareRevisionCharacteristic() {
	Characteristic<std::string>* _firmwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_firmwareRevisionCharacteristic);

#ifdef GIT_HASH
	std::string firmware_version = STRINGIFY(GIT_HASH);
#else
	std::string firmware_version = STRINGIFY(FIRMWARE_VERSION);
#endif

	_firmwareRevisionCharacteristic->setUUID(BLE_UUID_FIRMWARE_REVISION_STRING_CHAR);
	_firmwareRevisionCharacteristic->setName(BLE_CHAR_FIRMWARE_REVISION);
	_firmwareRevisionCharacteristic->setWritable(false);
	_firmwareRevisionCharacteristic->setDefaultValue(firmware_version);
	_firmwareRevisionCharacteristic->setMaxStringLength(firmware_version.size());
	_firmwareRevisionCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_firmwareRevisionCharacteristic->setValueLength(firmware_version.size());
}

void DeviceInformationService::addSoftwareRevisionCharacteristic() {
	Characteristic<std::string>* _softwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_softwareRevisionCharacteristic);
	_softwareRevisionCharacteristic->setUUID(BLE_UUID_SOFTWARE_REVISION_STRING_CHAR);
	_softwareRevisionCharacteristic->setName(BLE_CHAR_SOFTWARE_REVISION);
	_softwareRevisionCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
#ifdef GIT_BRANCH
	_softwareRevisionCharacteristic->setDefaultValue(STRINGIFY(GIT_BRANCH));
#else
//	_softwareRevisionCharacteristic->setDefaultValue(STRINGIFY(SOFTWARE_REVISION));
#endif
	_softwareRevisionCharacteristic->setWritable(false);
}
