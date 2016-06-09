/** Store settings in RAM or persistent memory
 *
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Sep 22, 2015
 * License: LGPLv3+
 */

#include "storage/cs_Settings.h"

#include <ble/cs_UUID.h>
#include <events/cs_EventDispatcher.h>
#include <util/cs_Utils.h>

using namespace BLEpp;

Settings::Settings() : _initialized(false), _storage(NULL) {
};

void Settings::init() {
	_storage = &Storage::getInstance();

	if (!_storage->isInitialized()) {
		LOGe("forgot to initialize Storage!");
		return;
	}

	_storage->getHandle(PS_ID_CONFIGURATION, _storageHandle);
	loadPersistentStorage();

	// need to initialize the flags if they are uninitialized, e.g. first time
	if (_storageStruct.flagsBit.flagsUninitialized) {
		initFlags();
	}

	_initialized = true;
}

//	void writeToStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
void Settings::writeToStorage(uint8_t type, uint8_t* payload, uint16_t length, bool persistent) {

	/////////////////////////////////////////////////
	//// SPECIAL CASES
	/////////////////////////////////////////////////
	switch(type) {
	case CONFIG_WIFI_SETTINGS: {
		LOGi("Temporarily store wifi settings");
		//! max length '{ "ssid": "32 bytes", "key": "32 bytes"}', 64+24 bytes = 88 bytes
		if (length > 88) {
			LOGe("Wifi settings string too long");
			break;
		}
		_wifiSettings = std::string((char*)payload, length);
		LOGi("Stored wifi settings [%i]: %s", length, _wifiSettings.c_str());
		return;
	}
	}

	/////////////////////////////////////////////////
	//// DEFAULT
	/////////////////////////////////////////////////
	if (verify(type, payload, length)) {
		set(type, payload, persistent, length);
		uint8_t* p_item = getStorageItem(type);
		EventDispatcher::getInstance().dispatch(type, payload, length);
	}
}

bool Settings::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {

	/////////////////////////////////////////////////
	//// SPECIAL CASES
	/////////////////////////////////////////////////
	switch(type) {
	case CONFIG_WIFI_SETTINGS: {
		LOGd("Read wifi settings. Does reset it.");
		//! copy string, because we clear it on read
		std::string str;
		if (_wifiSettings == "") {
			str = "{}";
		} else {
			str = _wifiSettings;
		}
		streamBuffer->fromString(str);
		streamBuffer->setType(type);
		_wifiSettings = "";
		LOGd("Wifi settings read");
		return true;
	}
	}

	/////////////////////////////////////////////////
	//// DEFAULT
	/////////////////////////////////////////////////
	uint16_t plen = getSettingsItemSize(type);
	if (plen > 0) {
		// todo: do we really want to reload the current working copy?
		//  or do we just want to return the value in the current working copy?
		loadPersistentStorage();
		uint8_t payload[plen];
		get(type, payload, plen);
		streamBuffer->setPayload(payload, plen);
		streamBuffer->setType(type);

		return true;
	} else {
		return false;
	}
}

bool Settings::verify(uint8_t type, uint8_t* payload, uint8_t length) {
	switch(type) {
	/////////////////////////////////////////////////
	//// UINT 8
	/////////////////////////////////////////////////
	case CONFIG_SCAN_FILTER:
	case CONFIG_CURRENT_LIMIT:
	case CONFIG_FLOOR: {
		if (length != 1) {
			LOGw("Expected uint8");
			return false;
		}
		LOGi("Set %u to %u", type, payload[0]);
		return true;
	}

	/////////////////////////////////////////////////
	//// INT 8
	/////////////////////////////////////////////////
	case CONFIG_MAX_CHIP_TEMP:
	case CONFIG_MAX_ENV_TEMP:
	case CONFIG_MIN_ENV_TEMP:
	case CONFIG_TX_POWER:
	case CONFIG_IBEACON_TXPOWER: {
		if (length != 1) {
			LOGw("Expected int8");
			return false;
		}
		LOGi("Set %u to %i", type, (int8_t)payload[0]);
		return true;
	}

	/////////////////////////////////////////////////
	//// UINT 16
	/////////////////////////////////////////////////
	case CONFIG_CROWNSTONE_ID:
	case CONFIG_SCAN_FILTER_SEND_FRACTION:
	case CONFIG_BOOT_DELAY:
	case CONFIG_SCAN_BREAK_DURATION:
	case CONFIG_SCAN_DURATION:
	case CONFIG_SCAN_SEND_DELAY:
	case CONFIG_ADV_INTERVAL:
	case CONFIG_IBEACON_MINOR:
	case CONFIG_IBEACON_MAJOR:
	case CONFIG_NEARBY_TIMEOUT: {
		if (length != 2) {
			LOGw("Expected uint16");
			return false;
		}
		LOGi("Set %u to %u", type, *(uint16_t*)payload);
		return true;
	}

	/////////////////////////////////////////////////
	//// BYTE ARRAY
	/////////////////////////////////////////////////
	case CONFIG_IBEACON_UUID: {
		if (length != 16) {
			LOGw("Expected 16 bytes for UUID, received: %d", length);
			return false;
		}
		log(INFO, "Set uuid to: "); BLEutil::printArray(payload, 16);
		return true;
	}
	case CONFIG_PASSKEY: {
		if (length != BLE_GAP_PASSKEY_LEN) {
			LOGw("Expected length %d for passkey", BLE_GAP_PASSKEY_LEN);
			return false;
		}
		LOGi("Set passkey to %s", std::string((char*)payload, length).c_str());
		return true;
	}
	case CONFIG_NAME: {
		if (length > MAX_STRING_STORAGE_SIZE) {
			LOGe(MSG_NAME_TOO_LONG);
			return false;
		}
		LOGi("Set name to: %s", std::string((char*)payload, length).c_str());
		return true;
	}

	/////////////////////////////////////////////////
	//// WRITE DISABLED
	/////////////////////////////////////////////////
	case CONFIG_MESH_ENABLED :
	case CONFIG_ENCRYPTION_ENABLED :
	case CONFIG_IBEACON_ENABLED :
	case CONFIG_SCANNER_ENABLED :
	case CONFIG_CONT_POWER_SAMPLER_ENABLED : {
//		updateFlag(type, payload[0] != 0, persistent);
		LOGe("Write disabled. Use commands to enable/disable");
		return false;
	}

	case CONFIG_ADC_SAMPLE_RATE:
	case CONFIG_POWER_SAMPLE_BURST_INTERVAL:
	case CONFIG_POWER_SAMPLE_CONT_INTERVAL:
	case CONFIG_POWER_SAMPLE_CONT_NUM_SAMPLES:
	default: {
		LOGw("There is no such configuration type (%u).", type);
		return false;
	}
	}
}

uint8_t* Settings::getStorageItem(uint8_t type) {
	switch(type) {
	case CONFIG_NAME: {
		return (uint8_t*)&_storageStruct.device_name;
	}
	case CONFIG_NEARBY_TIMEOUT: {
		return (uint8_t*)&_storageStruct.nearbyTimeout;
	}
	case CONFIG_FLOOR: {
		return (uint8_t*)&_storageStruct.floor;
	}
	case CONFIG_IBEACON_MAJOR: {
		return (uint8_t*)&_storageStruct.beacon.major;
	}
	case CONFIG_IBEACON_MINOR: {
		return (uint8_t*)&_storageStruct.beacon.minor;
	}
	case CONFIG_IBEACON_UUID: {
		return (uint8_t*)&_storageStruct.beacon.uuid.uuid128;
	}
	case CONFIG_IBEACON_TXPOWER: {
		return (uint8_t*)&_storageStruct.beacon.txPower;
	}
	case CONFIG_WIFI_SETTINGS: {
		return (uint8_t*)&_wifiSettings;
	}
	case CONFIG_TX_POWER: {
		return (uint8_t*)&_storageStruct.txPower;
	}
	case CONFIG_ADV_INTERVAL: {
		return (uint8_t*)&_storageStruct.advInterval;
	}
	case CONFIG_PASSKEY: {
		return (uint8_t*)&_storageStruct.passkey;
	}
	case CONFIG_MIN_ENV_TEMP: {
		return (uint8_t*)&_storageStruct.minEnvTemp;
	}
	case CONFIG_MAX_ENV_TEMP: {
		return (uint8_t*)&_storageStruct.maxEnvTemp;
	}
	case CONFIG_SCAN_DURATION: {
		return (uint8_t*)&_storageStruct.scanDuration;
	}
	case CONFIG_SCAN_SEND_DELAY: {
		return (uint8_t*)&_storageStruct.scanSendDelay;
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		return (uint8_t*)&_storageStruct.scanBreakDuration;
	}
	case CONFIG_BOOT_DELAY: {
		return (uint8_t*)&_storageStruct.bootDelay;
	}
	case CONFIG_MAX_CHIP_TEMP: {
		return (uint8_t*)&_storageStruct.maxChipTemp;
	}
	case CONFIG_SCAN_FILTER: {
		return (uint8_t*)&_storageStruct.scanFilter;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		return (uint8_t*)&_storageStruct.scanFilterSendFraction;
	}
	case CONFIG_CURRENT_LIMIT: {
		return (uint8_t*)&_storageStruct.currentLimit;
	}
	case CONFIG_CROWNSTONE_ID: {
		return (uint8_t*)&_storageStruct.crownstoneId;
	}
	case CONFIG_ADC_SAMPLE_RATE: {
	}
	case CONFIG_POWER_SAMPLE_BURST_INTERVAL: {
	}
	case CONFIG_POWER_SAMPLE_CONT_INTERVAL: {
	}
	case CONFIG_POWER_SAMPLE_CONT_NUM_SAMPLES: {
	}
	default: {
		LOGw("There is no such configuration type (%u).", type);
		return NULL;
	}
	}
}

uint16_t Settings::getSettingsItemSize(uint8_t type) {

	switch(type) {
	/////////////////////////////////////////////////
	//// UINT 8
	/////////////////////////////////////////////////
	case CONFIG_SCAN_FILTER:
	case CONFIG_CURRENT_LIMIT:
	case CONFIG_FLOOR: {
		return 1;
	}

	/////////////////////////////////////////////////
	//// INT 8
	/////////////////////////////////////////////////
	case CONFIG_MAX_CHIP_TEMP:
	case CONFIG_MAX_ENV_TEMP:
	case CONFIG_MIN_ENV_TEMP:
	case CONFIG_TX_POWER:
	case CONFIG_IBEACON_TXPOWER: {
		return 1;
	}

	/////////////////////////////////////////////////
	//// UINT 16
	/////////////////////////////////////////////////
	case CONFIG_CROWNSTONE_ID:
	case CONFIG_SCAN_FILTER_SEND_FRACTION:
	case CONFIG_BOOT_DELAY:
	case CONFIG_SCAN_BREAK_DURATION:
	case CONFIG_SCAN_DURATION:
	case CONFIG_SCAN_SEND_DELAY:
	case CONFIG_ADV_INTERVAL:
	case CONFIG_IBEACON_MINOR:
	case CONFIG_IBEACON_MAJOR:
	case CONFIG_NEARBY_TIMEOUT: {
		return 2;
	}

	/////////////////////////////////////////////////
	//// BYTE ARRAY
	/////////////////////////////////////////////////
	case CONFIG_IBEACON_UUID: {
		return 16;
	}
	case CONFIG_PASSKEY: {
		return BLE_GAP_PASSKEY_LEN;
	}
	case CONFIG_NAME: {
		return MAX_STRING_STORAGE_SIZE+1;
	}

	/////////////////////////////////////////////////
	//// FLAGS
	/////////////////////////////////////////////////
	case CONFIG_MESH_ENABLED :
	case CONFIG_ENCRYPTION_ENABLED :
	case CONFIG_IBEACON_ENABLED :
	case CONFIG_SCANNER_ENABLED :
	case CONFIG_CONT_POWER_SAMPLER_ENABLED : {
		return 1;
	}

	case CONFIG_ADC_SAMPLE_RATE:
	case CONFIG_POWER_SAMPLE_BURST_INTERVAL:
	case CONFIG_POWER_SAMPLE_CONT_INTERVAL:
	case CONFIG_POWER_SAMPLE_CONT_NUM_SAMPLES:
	default:
		LOGw("There is no such configuration type (%u).", type);
		return 0;
	}
}

void Settings::initFlags() {
	// set all flags to their default value
	_storageStruct.flagsBit.meshDisabled = !MESHING;
	_storageStruct.flagsBit.encryptionDisabled = !ENCRYPTION;
	_storageStruct.flagsBit.iBeaconDisabled = !IBEACON;
	_storageStruct.flagsBit.scannerDisabled = !INTERVAL_SCANNER_ENABLED;
	_storageStruct.flagsBit.continuousPowerSamplerDisabled = !CONTINUOUS_POWER_SAMPLER;
	_storageStruct.flagsBit.flagsUninitialized = false;
}

bool Settings::updateFlag(uint8_t type, bool value, bool persistent) {

	// should not happen, but better to check
	if (_storageStruct.flagsBit.flagsUninitialized) {
		// before updating a flag, we need to initialize all flags to their default
		// value, otherwise they will be wrongly read after the update
		initFlags();
	}

	switch(type) {
		case CONFIG_MESH_ENABLED : {
		_storageStruct.flagsBit.meshDisabled = !value;
		break;
	}
	case CONFIG_ENCRYPTION_ENABLED : {
		_storageStruct.flagsBit.encryptionDisabled = !value;
		break;
	}
	case CONFIG_IBEACON_ENABLED : {
		_storageStruct.flagsBit.iBeaconDisabled = !value;
		break;
	}
	case CONFIG_SCANNER_ENABLED : {
		_storageStruct.flagsBit.scannerDisabled = !value;
		break;
	}
	case CONFIG_CONT_POWER_SAMPLER_ENABLED : {
		_storageStruct.flagsBit.continuousPowerSamplerDisabled = !value;
		break;
	}
	default: {
		return false;
	}
	}

	if (persistent) {
		savePersistentStorageItem(&_storageStruct.flags);
	}
	EventDispatcher::getInstance().dispatch(type, &value, 1);
	return true;
}

bool Settings::readFlag(uint8_t type, bool& value) {

	bool default_value;
	switch(type) {
		case CONFIG_MESH_ENABLED : {
		value = !_storageStruct.flagsBit.meshDisabled;
		default_value = MESHING;
		break;
	}
	case CONFIG_ENCRYPTION_ENABLED : {
		value = !_storageStruct.flagsBit.encryptionDisabled;
		default_value = ENCRYPTION;
		break;
	}
	case CONFIG_IBEACON_ENABLED : {
		value = !_storageStruct.flagsBit.iBeaconDisabled;
		default_value = IBEACON;
		break;
	}
	case CONFIG_SCANNER_ENABLED : {
		value = !_storageStruct.flagsBit.scannerDisabled;
		default_value = INTERVAL_SCANNER_ENABLED;
		break;
	}
	case CONFIG_CONT_POWER_SAMPLER_ENABLED : {
		value = !_storageStruct.flagsBit.continuousPowerSamplerDisabled;
		default_value = CONTINUOUS_POWER_SAMPLER;
		break;
	}
	default:
		return false;
	}

	if (_storageStruct.flagsBit.flagsUninitialized) {
		value = default_value;
		return true;
	}

	return true;
}

bool Settings::isEnabled(uint8_t type) {

	switch(type) {
	case CONFIG_MESH_ENABLED :
	case CONFIG_ENCRYPTION_ENABLED :
	case CONFIG_IBEACON_ENABLED :
	case CONFIG_SCANNER_ENABLED :
	case CONFIG_CONT_POWER_SAMPLER_ENABLED : {
		bool enabled;
		readFlag(type, enabled);
		return enabled;
	}
	default:
		return false;
	}
}

bool Settings::get(uint8_t type, void* target) {
	uint16_t size = 0;
	return get(type, target, size);
}

bool Settings::get(uint8_t type, void* target, uint16_t& size) {
	switch(type) {
	case CONFIG_NAME: {
		char default_name[32];
		sprintf(default_name, "%s_%s", STRINGIFY(BLUETOOTH_NAME), STRINGIFY(COMPILATION_TIME));
		Storage::getString(_storageStruct.device_name, (char*) target, default_name, size);
		return true;
	}
	case CONFIG_FLOOR: {
		Storage::getUint8(_storageStruct.floor, (uint8_t*)target, 0);
		return true;
	}
	case CONFIG_NEARBY_TIMEOUT: {
		Storage::getUint16(_storageStruct.nearbyTimeout, (uint16_t*)target, TRACKDEVICE_DEFAULT_TIMEOUT_COUNT);
		return true;
	}
	case CONFIG_IBEACON_MAJOR: {
		Storage::getUint16(_storageStruct.beacon.major, (uint16_t*)target, BEACON_MAJOR);
		return true;
	}
	case CONFIG_IBEACON_MINOR: {
		Storage::getUint16(_storageStruct.beacon.minor, (uint16_t*)target, BEACON_MINOR);
		return true;
	}
	case CONFIG_IBEACON_UUID: {
		Storage::getArray<uint8_t>(_storageStruct.beacon.uuid.uuid128, (uint8_t*) target, ((ble_uuid128_t)UUID(BEACON_UUID)).uuid128, 16);
		return true;
	}
	case CONFIG_IBEACON_TXPOWER: {
		Storage::getInt8(_storageStruct.beacon.txPower, (int8_t*)target, BEACON_RSSI);
		return true;
	}
	case CONFIG_WIFI_SETTINGS: {
		//! copy string, because we clear it on read
		std::string* p_str = (std::string*) target;
		if (_wifiSettings == "") {
			*p_str = "{}";
		} else {
			*p_str = _wifiSettings;
		}
		return true;
	}
	case CONFIG_TX_POWER: {
		Storage::getInt8(_storageStruct.txPower, (int8_t*)target, TX_POWER);
		return true;
	}
	case CONFIG_ADV_INTERVAL: {
		Storage::getUint16(_storageStruct.advInterval, (uint16_t*)target, ADVERTISEMENT_INTERVAL);
		return true;
	}
	case CONFIG_PASSKEY: {
		Storage::getArray<uint8_t>(_storageStruct.passkey, (uint8_t*) target, (uint8_t*)STATIC_PASSKEY, 6);
		return true;
	}
	case CONFIG_MIN_ENV_TEMP: {
		Storage::getInt8(_storageStruct.minEnvTemp, (int8_t*)target, MIN_ENV_TEMP);
		return true;
	}
	case CONFIG_MAX_ENV_TEMP: {
		Storage::getInt8(_storageStruct.maxEnvTemp, (int8_t*)target, MAX_ENV_TEMP);
		return true;
	}
	case CONFIG_SCAN_DURATION: {
		Storage::getUint16(_storageStruct.scanDuration, (uint16_t*)target, SCAN_DURATION);
		return true;
	}
	case CONFIG_SCAN_SEND_DELAY: {
		Storage::getUint16(_storageStruct.scanSendDelay, (uint16_t*)target, SCAN_SEND_DELAY);
		return true;
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		Storage::getUint16(_storageStruct.scanBreakDuration, (uint16_t*)target, SCAN_BREAK_DURATION);
		return true;
	}
	case CONFIG_BOOT_DELAY: {
		Storage::getUint16(_storageStruct.bootDelay, (uint16_t*)target, BOOT_DELAY);
		return true;
	}
	case CONFIG_MAX_CHIP_TEMP: {
		Storage::getInt8(_storageStruct.maxChipTemp, (int8_t*)target, MAX_CHIP_TEMP);
		return true;
	}
	case CONFIG_SCAN_FILTER: {
		Storage::getUint8(_storageStruct.scanFilter, (uint8_t*)target, SCAN_FILTER);
		return true;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		Storage::getUint16(_storageStruct.scanFilterSendFraction, (uint16_t*)target, SCAN_FILTER_SEND_FRACTION);
		return true;
	}
	case CONFIG_CURRENT_LIMIT: {
		Storage::getUint8(_storageStruct.currentLimit, (uint8_t*)target, CURRENT_LIMIT);
		return true;
	}
	case CONFIG_CROWNSTONE_ID: {
		Storage::getUint16(_storageStruct.crownstoneId, (uint16_t*)target, 0);
		return true;
	}
	case CONFIG_ADC_SAMPLE_RATE: {
//		return true;
	}
	case CONFIG_POWER_SAMPLE_BURST_INTERVAL: {
//		return true;
	}
	case CONFIG_POWER_SAMPLE_CONT_INTERVAL: {
//		return true;
	}
	case CONFIG_POWER_SAMPLE_CONT_NUM_SAMPLES: {
//		return true;
	}
	default: {
		LOGw("There is no such configuration type (%u).", type);
	}
	}
	return false;
}

bool Settings::set(uint8_t type, void* target, bool persistent, uint16_t size) {
	switch(type) {
	case CONFIG_NAME: {
		Storage::setString(std::string((char*)target, size), _storageStruct.device_name);
		break;
	}
	case CONFIG_NEARBY_TIMEOUT: {
		Storage::setUint16(*((uint16_t*)target), (uint32_t&)_storageStruct.nearbyTimeout);
		break;
	}
	case CONFIG_FLOOR: {
		Storage::setUint8(*((uint8_t*)target), _storageStruct.floor);
		break;
	}
	case CONFIG_IBEACON_MAJOR: {
		Storage::setUint16(*((uint16_t*)target), (uint32_t&)_storageStruct.beacon.major);
		break;
	}
	case CONFIG_IBEACON_MINOR: {
		Storage::setUint16(*((uint16_t*)target),(uint32_t&) _storageStruct.beacon.minor);
		break;
	}
	case CONFIG_IBEACON_UUID: {
		Storage::setArray<uint8_t>((uint8_t*) target, _storageStruct.beacon.uuid.uuid128, 16);
		break;
	}
	case CONFIG_IBEACON_TXPOWER: {
		Storage::setInt8(*((int8_t*)target), (int32_t&)_storageStruct.beacon.txPower);
		break;
	}
	case CONFIG_WIFI_SETTINGS: {
		if (size > 0) {
			_wifiSettings = std::string((char*)target, size);
		}
		break;
	}
	case CONFIG_TX_POWER: {
		Storage::setInt8(*((int8_t*)target), _storageStruct.txPower);
		break;
	}
	case CONFIG_ADV_INTERVAL: {
		Storage::setUint16(*((uint16_t*)target), _storageStruct.advInterval);
		break;
	}
	case CONFIG_PASSKEY: {
		Storage::setArray<uint8_t>((uint8_t*) target, _storageStruct.passkey, BLE_GAP_PASSKEY_LEN);
		break;
	}
	case CONFIG_MIN_ENV_TEMP: {
		Storage::setInt8(*((int8_t*)target), _storageStruct.minEnvTemp);
		break;
	}
	case CONFIG_MAX_ENV_TEMP: {
		Storage::setInt8(*((int8_t*)target), _storageStruct.maxEnvTemp);
		break;
	}
	case CONFIG_SCAN_DURATION: {
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanDuration);
		break;
	}
	case CONFIG_SCAN_SEND_DELAY: {
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanSendDelay);
		break;
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanBreakDuration);
		break;
	}
	case CONFIG_BOOT_DELAY: {
		Storage::setUint16(*((uint16_t*)target), _storageStruct.bootDelay);
		break;
	}
	case CONFIG_MAX_CHIP_TEMP: {
		Storage::setInt8(*((int8_t*)target), _storageStruct.maxChipTemp);
		break;
	}
	case CONFIG_SCAN_FILTER: {
		Storage::setUint8(*((uint8_t*)target), _storageStruct.scanFilter);
		break;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanFilterSendFraction);
		break;
	}
	case CONFIG_CURRENT_LIMIT: {
		Storage::setUint8(*((uint8_t*)target), _storageStruct.currentLimit);
		break;
	}
	case CONFIG_CROWNSTONE_ID: {
		Storage::setUint16(*((uint16_t*)target), _storageStruct.crownstoneId);
		break;
	}
	case CONFIG_ADC_SAMPLE_RATE: {
//		break;
	}
	case CONFIG_POWER_SAMPLE_BURST_INTERVAL: {
//		break;
	}
	case CONFIG_POWER_SAMPLE_CONT_INTERVAL: {
//		break;
	}
	case CONFIG_POWER_SAMPLE_CONT_NUM_SAMPLES: {
//		break;
	}
	default: {
		LOGw("There is no such configuration type (%u).", type);
		return false;
	}
	}

	uint8_t* p_item = getStorageItem(type);
	if (persistent) {
		size = getSettingsItemSize(type);
		// minimum item size we can store is 4
		if (size < 4) {
			size = 4;
		}
		savePersistentStorageItem(p_item, size);
	}
	return true;
}

//ps_configuration_t& Settings::getConfig() {
//	return _storageStruct;
//}

/** Get a handle to the persistent storage struct and load it from FLASH.
 *
 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
 * writing less than a minimal block strains the memory just as much as flashing the entire block.
 * Hence, there is an entire struct that can be filled and flashed at once.
 */
void Settings::loadPersistentStorage() {
	_storage->readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}


void Settings::savePersistentStorageItem(uint8_t* item, uint8_t size) {
	uint32_t offset = Storage::getOffset(&_storageStruct, item);
	_storage->writeItem(_storageHandle, offset, item, size);
}

void Settings::savePersistentStorageItem(int32_t* item) {
	savePersistentStorageItem((uint8_t*)item, sizeof(int32_t));
}

void Settings::savePersistentStorageItem(uint32_t* item) {
	savePersistentStorageItem((uint8_t*)item, sizeof(uint32_t));
}

/** Save the whole configuration struct to FLASH.
 *  try to avoid using this function and store each item individually with the
 *  savePersistentStorageItem functions
 */
void Settings::savePersistentStorage() {
	_storage->writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void Settings::saveIBeaconPersistent() {
	savePersistentStorageItem((uint8_t*)&_storageStruct.beacon, sizeof(_storageStruct.beacon));
}

//	void ConfigHelper::enable(ps_storage_id id, uint16_t size) {
//		_storage->getHandle(id, _storageHandles[id]);
//		loadPersistentStorage(_storageHandles[id], );
//	}

/** Retrieve the Bluetooth name from the object representing the BLE stack.
 *
 * @return name of the device
 */
//std::string & Settings::getBLEName() {
//	loadPersistentStorage();
//	std::string& _name = BLEpp::Nrf51822BluetoothStack::getInstance().getDeviceName();
//	Storage::getString(_storageStruct.device_name, _name, _name);
//	return _name;
//}

/** Write the Bluetooth name to the object representing the BLE stack.
 *
 * This updates the Bluetooth name immediately, however, it does not update the name persistently. It
 * has to be written to FLASH in that case.
 */
//void Settings::setBLEName(const std::string &name, bool persistent) {
//	if (name.length() > 31) {
//		LOGe(MSG_NAME_TOO_LONG);
//		return;
//	}
//	BLEpp::Nrf51822BluetoothStack::getInstance().updateDeviceName(name);
//	Storage::setString(name, _storageStruct.device_name);
//	if (persistent) {
//		savePersistentStorageItem((uint8_t*)_storageStruct.device_name, sizeof(_storageStruct.device_name));
//	}
//}

void Settings::factoryReset(uint32_t resetCode) {
	if (resetCode != FACTORY_RESET_CODE) {
		LOGe("wrong reset code!");
		return;
	}

	_storage->clearStorage(PS_ID_CONFIGURATION);
	memset(&_storageStruct, 0xFF, sizeof(_storageStruct));
	initFlags();
//	loadPersistentStorage();
}
