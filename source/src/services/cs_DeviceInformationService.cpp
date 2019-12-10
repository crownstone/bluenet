/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_HardwareVersions.h>
#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Serial.h>
#include <services/cs_DeviceInformationService.h>
#include <util/cs_Utils.h>

#ifdef GIT_BRANCH
#define SOFTWARE_REVISION_CHARACTERISTIC
#endif

union cs_variant_t {
	uint32_t u32;
	uint8_t  u8[4];
};

inline std::string get_hardware_revision(void) {

	// get Production run
	std::string production_run = "0000";

	// get Housing ID
	std::string housing_id = "0000";

	// reserved
	std::string reserved = "00000000";

	// AAB0 is stored as 0x41414230
	cs_variant_t variant;
	variant.u32 = BLEutil::convertEndian32(NRF_FICR->INFO.VARIANT);

	// get nordic chip version
	char nordic_chip_version[6];

	switch(NRF_FICR->INFO.PACKAGE) {
		case 0x2000:
			nordic_chip_version[0] = 'Q';
			nordic_chip_version[1] = 'F';
			break;
		case 0x2001:
			nordic_chip_version[0] = 'C';
			nordic_chip_version[1] = 'H';
			break;
		case 0x2002:
			nordic_chip_version[0] = 'C';
			nordic_chip_version[1] = 'I';
			break;
		case 0x2005:
			nordic_chip_version[0] = 'C';
			nordic_chip_version[1] = 'K';
			break;
		default:
			nordic_chip_version[0] = '?';
			nordic_chip_version[1] = '?';
	}

	for (uint8_t i = 0; i < 4; ++i) {
		nordic_chip_version[i+2] = variant.u8[i];
	}

	char hardware_revision[34];
	sprintf(hardware_revision, "%11.11s%.4s%.4s%.8s%.6s", get_hardware_version(), production_run.c_str(),
			housing_id.c_str(), reserved.c_str(), nordic_chip_version);
	return std::string(hardware_revision, 34);
}

DeviceInformationService::DeviceInformationService():
	_hardwareRevisionCharacteristic(NULL),
	_firmwareRevisionCharacteristic(NULL),
	_softwareRevisionCharacteristic(NULL)
{
	setUUID(UUID(BLE_UUID_DEVICE_INFORMATION_SERVICE));
	setName(BLE_SERVICE_DEVICE_INFORMATION);
}

void DeviceInformationService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, BLE_SERVICE_DEVICE_INFORMATION);

	addHardwareRevisionCharacteristic();
	addFirmwareRevisionCharacteristic();

#ifdef SOFTWARE_REVISION_CHARACTERISTIC
	addSoftwareRevisionCharacteristic();
#endif

	updatedCharacteristics();
}

void DeviceInformationService::addHardwareRevisionCharacteristic() {
	LOGi(FMT_CHAR_ADD, STR_CHAR_HARDWARE_REVISION);
	if (_hardwareRevisionCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_HARDWARE_REVISION);
		return;
	}

	_hardwareRevisionCharacteristic = new Characteristic<std::string>();
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
	LOGi(FMT_CHAR_ADD, STR_CHAR_FIRMWARE_REVISION);
	if (_firmwareRevisionCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_FIRMWARE_REVISION);
		return;
	}
	_firmwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_firmwareRevisionCharacteristic);

	std::string firmware_version;
	if (strcmp(g_BUILD_TYPE, "Release") == 0) {
		firmware_version = g_FIRMWARE_VERSION;
	} else {
		firmware_version = g_GIT_SHA1;
	}
	LOGd("Firmware version: %s", firmware_version.c_str());

	_firmwareRevisionCharacteristic->setUUID(BLE_UUID_FIRMWARE_REVISION_STRING_CHAR);
	_firmwareRevisionCharacteristic->setName(BLE_CHAR_FIRMWARE_REVISION);
	_firmwareRevisionCharacteristic->setWritable(false);
	_firmwareRevisionCharacteristic->setDefaultValue(firmware_version);
	_firmwareRevisionCharacteristic->setMaxStringLength(firmware_version.size());
	_firmwareRevisionCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_firmwareRevisionCharacteristic->setValueLength(firmware_version.size());
}

void DeviceInformationService::addSoftwareRevisionCharacteristic() {
	LOGi(FMT_CHAR_ADD, STR_CHAR_SOFTWARE_REVISION);
	if (_softwareRevisionCharacteristic != NULL) {
		LOGe(FMT_CHAR_EXISTS, STR_CHAR_SOFTWARE_REVISION);
		return;
	}
	_softwareRevisionCharacteristic = new Characteristic<std::string>();
	addCharacteristic(_softwareRevisionCharacteristic);
	_softwareRevisionCharacteristic->setUUID(BLE_UUID_SOFTWARE_REVISION_STRING_CHAR);
	_softwareRevisionCharacteristic->setName(BLE_CHAR_SOFTWARE_REVISION);
	_softwareRevisionCharacteristic->setMinAccessLevel(ENCRYPTION_DISABLED);
	_softwareRevisionCharacteristic->setDefaultValue(STRINGIFY(GIT_BRANCH));
	_softwareRevisionCharacteristic->setWritable(false);
}
