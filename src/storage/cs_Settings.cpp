/** Store settings in RAM or persistent memory.
 *
 * This files serves as a little indirection between loading data from RAM or FLASH.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 22, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "storage/cs_Settings.h"

#include <cfg/cs_Config.h>
#include <ble/cs_UUID.h>
#include <events/cs_EventDispatcher.h>
#include <util/cs_Utils.h>
#include <storage/cs_StorageHelper.h>

Settings::Settings() : _initialized(false), _storage(NULL), _boardsConfig(NULL) {
};

void Settings::init(boards_config_t* boardsConfig) {
	_storage = &Storage::getInstance();
	_boardsConfig = boardsConfig;

	if (!_storage->isInitialized()) {
		LOGe(FMT_NOT_INITIALIZED, "Storage");
		return;
	}

	loadPersistentStorage();

	_initialized = true;
}

ERR_CODE Settings::writeToStorage(uint8_t type, uint8_t* payload, uint16_t length, bool persistent) {

	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;
	/*
	error_code = verify(type, payload, length);
	if (SUCCESS(error_code)) {
		error_code = set(type, payload, persistent, length);
		EventDispatcher::getInstance().dispatch(type, payload, length);
	}
	*/
	return error_code;
}

ERR_CODE Settings::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {

	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;
	/*
	uint16_t plen = getSettingsItemSize(type);
	if (plen > 0) {
		// todo: do we really want to reload the current working copy?
		//  or do we just want to return the value in the current working copy?
		loadPersistentStorage();
		uint8_t payload[plen];
		error_code = get(type, payload, plen);
		if (SUCCESS(error_code)) {
			streamBuffer->setPayload(payload, plen);
			streamBuffer->setType(type);
		}
	} else {
		error_code = ERR_UNKNOWN_TYPE;
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
	}
	*/
	return error_code;
}

ERR_CODE Settings::verify(uint8_t type, uint8_t* payload, uint8_t length) {
	switch(type) {
	/////////////////////////////////////////////////
	//// UINT 8
	/////////////////////////////////////////////////
	case CONFIG_SCAN_FILTER:
	case CONFIG_FLOOR:
	case CONFIG_MESH_CHANNEL:
	case CONFIG_UART_ENABLED: {
		if (length != 1) {
			LOGw(FMT_ERR_EXPECTED, "uint8");
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		LOGi(FMT_SET_INT_TYPE_VAL, type, payload[0]);
		return ERR_SUCCESS;
	}

	/////////////////////////////////////////////////
	//// INT 8
	/////////////////////////////////////////////////
	case CONFIG_LOW_TX_POWER:
	case CONFIG_MAX_CHIP_TEMP:
	case CONFIG_MAX_ENV_TEMP:
	case CONFIG_MIN_ENV_TEMP:
	case CONFIG_TX_POWER:
	case CONFIG_IBEACON_TXPOWER: {
		if (length != 1) {
			LOGw(FMT_ERR_EXPECTED, "int8");
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		LOGi(FMT_SET_INT_TYPE_VAL, type, (int8_t)payload[0]);
		return ERR_SUCCESS;
	}

	/////////////////////////////////////////////////
	//// UINT 16
	/////////////////////////////////////////////////
	case CONFIG_SCAN_INTERVAL:
	case CONFIG_SCAN_WINDOW:
	case CONFIG_RELAY_HIGH_DURATION:
	case CONFIG_CROWNSTONE_ID:
	case CONFIG_SCAN_FILTER_SEND_FRACTION:
	case CONFIG_BOOT_DELAY:
	case CONFIG_SCAN_BREAK_DURATION:
	case CONFIG_SCAN_DURATION:
	case CONFIG_SCAN_SEND_DELAY:
	case CONFIG_ADV_INTERVAL:
	case CONFIG_IBEACON_MINOR:
	case CONFIG_IBEACON_MAJOR:
	case CONFIG_NEARBY_TIMEOUT:
	case CONFIG_POWER_ZERO_AVG_WINDOW:
	case CONFIG_SOFT_FUSE_CURRENT_THRESHOLD:
	case CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM: {
		if (length != 2) {
			LOGw(FMT_ERR_EXPECTED, "uint16");
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		LOGi(FMT_SET_INT_TYPE_VAL, type, *(uint16_t*)payload);
		return ERR_SUCCESS;
	}

	/////////////////////////////////////////////////
	//// UINT 32
	/////////////////////////////////////////////////
	case CONFIG_PWM_PERIOD:
	case CONFIG_MESH_ACCESS_ADDRESS: {
	if (length != 4) {
			LOGw(FMT_ERR_EXPECTED, "uint32");
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		LOGi(FMT_SET_INT_TYPE_VAL, type, *(uint32_t*)payload);
		return ERR_SUCCESS;
	}

	/////////////////////////////////////////////////
	//// INT 32
	/////////////////////////////////////////////////
	case CONFIG_VOLTAGE_ZERO:
	case CONFIG_CURRENT_ZERO:
	case CONFIG_POWER_ZERO: {
		if (length != 4) {
			LOGw(FMT_ERR_EXPECTED, "int32");
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		LOGi(FMT_SET_INT_TYPE_VAL, type, *(int32_t*)payload);
		return ERR_SUCCESS;
	}

	/////////////////////////////////////////////////
	//// FLOAT
	/////////////////////////////////////////////////
	case CONFIG_VOLTAGE_MULTIPLIER:
	case CONFIG_CURRENT_MULTIPLIER:
	case CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP:
	case CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN:
	case CONFIG_SWITCHCRAFT_THRESHOLD: {
		if (length != 4) {
			LOGw(FMT_ERR_EXPECTED, "float");
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		LOGi(FMT_SET_FLOAT_TYPE_VAL, type, *(float*)payload);
		return ERR_SUCCESS;
	}

	/////////////////////////////////////////////////
	//// BYTE ARRAY
	/////////////////////////////////////////////////
	case CONFIG_IBEACON_UUID: {
		if (length != 16) {
			LOGw(FMT_ERR_EXPECTED, "16 bytes");
//			LOGw("Expected 16 bytes for UUID, received: %d", length);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		log(SERIAL_INFO, FMT_SET_STR_TYPE_VAL, type, "");
		log(SERIAL_INFO, "Set uuid to: "); BLEutil::printArray(payload, 16);
		return ERR_SUCCESS;
	}
	case CONFIG_PASSKEY: {
		if (length != BLE_GAP_PASSKEY_LEN) {
			LOGw(FMT_ERR_EXPECTED_RECEIVED, BLE_GAP_PASSKEY_LEN, length);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		LOGi(FMT_SET_STR_TYPE_VAL, type, std::string((char*)payload, length).c_str());
		return ERR_SUCCESS;
	}
	case CONFIG_NAME: {
		if (length > MAX_STRING_STORAGE_SIZE) {
			LOGe(MSG_NAME_TOO_LONG);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		LOGi(FMT_SET_STR_TYPE_VAL, type, std::string((char*)payload, length).c_str());
		return ERR_SUCCESS;
	}
	case CONFIG_KEY_ADMIN :
	case CONFIG_KEY_MEMBER :
	case CONFIG_KEY_GUEST : {
		if (length != ENCRYPTION_KEY_LENGTH) {
			LOGe(FMT_ERR_EXPECTED_RECEIVED, ENCRYPTION_KEY_LENGTH, length);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		log(SERIAL_INFO, FMT_SET_STR_TYPE_VAL, type, "");
		BLEutil::printArray((uint8_t*)payload, length);
		return ERR_SUCCESS;
	}

	/////////////////////////////////////////////////
	//// WRITE DISABLED
	/////////////////////////////////////////////////
	case CONFIG_MESH_ENABLED :
	case CONFIG_ENCRYPTION_ENABLED :
	case CONFIG_IBEACON_ENABLED :
	case CONFIG_SCANNER_ENABLED :
	case CONFIG_CONT_POWER_SAMPLER_ENABLED :
	case CONFIG_PWM_ALLOWED:
	case CONFIG_SWITCH_LOCKED:
	case CONFIG_SWITCHCRAFT_ENABLED: {
//		updateFlag(type, payload[0] != 0, persistent);
		LOGe("Write disabled. Use commands to enable/disable");
		return ERR_WRITE_NOT_ALLOWED;
	}

	default: {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		return ERR_UNKNOWN_TYPE;
	}
	}
}

uint16_t Settings::getSettingsItemSize(uint8_t type) {

	switch(type) {
	/////////////////////////////////////////////////
	//// UINT 8
	/////////////////////////////////////////////////
	case CONFIG_SCAN_FILTER:
	case CONFIG_FLOOR:
	case CONFIG_MESH_CHANNEL:
	case CONFIG_UART_ENABLED: {
		return 1;
	}

	/////////////////////////////////////////////////
	//// INT 8
	/////////////////////////////////////////////////
	case CONFIG_LOW_TX_POWER:
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
//	case CONFIG_ADC_BURST_SAMPLE_RATE:
//	case CONFIG_POWER_SAMPLE_BURST_INTERVAL:
//	case CONFIG_POWER_SAMPLE_CONT_INTERVAL:
//	case CONFIG_ADC_CONT_SAMPLE_RATE:
	case CONFIG_SCAN_INTERVAL:
	case CONFIG_SCAN_WINDOW:
	case CONFIG_RELAY_HIGH_DURATION:
	case CONFIG_CROWNSTONE_ID:
	case CONFIG_SCAN_FILTER_SEND_FRACTION:
	case CONFIG_BOOT_DELAY:
	case CONFIG_SCAN_BREAK_DURATION:
	case CONFIG_SCAN_DURATION:
	case CONFIG_SCAN_SEND_DELAY:
	case CONFIG_ADV_INTERVAL:
	case CONFIG_IBEACON_MINOR:
	case CONFIG_IBEACON_MAJOR:
	case CONFIG_NEARBY_TIMEOUT:
	case CONFIG_POWER_ZERO_AVG_WINDOW:
	case CONFIG_SOFT_FUSE_CURRENT_THRESHOLD:
	case CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM: {
		return 2;
	}

	/////////////////////////////////////////////////
	//// UINT 32
	/////////////////////////////////////////////////
	case CONFIG_PWM_PERIOD:
	case CONFIG_MESH_ACCESS_ADDRESS: {
		return 4;
	}

	/////////////////////////////////////////////////
	//// INT 32
	/////////////////////////////////////////////////
	case CONFIG_VOLTAGE_ZERO:
	case CONFIG_CURRENT_ZERO:
	case CONFIG_POWER_ZERO: {
		return 4;
	}

	/////////////////////////////////////////////////
	//// FLOAT
	/////////////////////////////////////////////////
	case CONFIG_VOLTAGE_MULTIPLIER:
	case CONFIG_CURRENT_MULTIPLIER:
	case CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP:
	case CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN:
	case CONFIG_SWITCHCRAFT_THRESHOLD: {
		return 4;
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
	case CONFIG_KEY_ADMIN :
	case CONFIG_KEY_MEMBER :
	case CONFIG_KEY_GUEST : {
		return ENCRYPTION_KEY_LENGTH;
	}

	/////////////////////////////////////////////////
	//// FLAGS
	/////////////////////////////////////////////////
	case CONFIG_MESH_ENABLED :
	case CONFIG_ENCRYPTION_ENABLED :
	case CONFIG_IBEACON_ENABLED :
	case CONFIG_SCANNER_ENABLED :
	case CONFIG_CONT_POWER_SAMPLER_ENABLED :
	case CONFIG_PWM_ALLOWED:
	case CONFIG_SWITCH_LOCKED:
	case CONFIG_SWITCHCRAFT_ENABLED: {
		return 1;
	}

	default:
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		return 0;
	}
}

ERR_CODE Settings::get(uint8_t type, void* target, bool getDefaultValue) {
	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;
	return error_code;
/*	uint16_t size = 0;
	return get(type, target, size, getDefaultValue);
	*/
}

ERR_CODE Settings::get(uint8_t type, void* target, uint16_t& size, bool getDefaultValue) {
	/*
#ifdef PRINT_DEBUG
	LOGd("get %d", type);
#endif
	switch(type) {
	case CONFIG_NAME: {
		char default_name[32];
		sprintf(default_name, "%s", STRINGIFY(BLUETOOTH_NAME));
		StorageHelper::getString(_storageStruct.device_name, (char*) target, default_name, size, getDefaultValue);
		break;
	}
	case CONFIG_FLOOR: {
		StorageHelper::getUint8(_storageStruct.floor, (uint8_t*)target, 0, getDefaultValue);
		break;
	}
	case CONFIG_NEARBY_TIMEOUT: {
		StorageHelper::getUint16(_storageStruct.nearbyTimeout, (uint16_t*)target, TRACKDEVICE_DEFAULT_TIMEOUT_COUNT, getDefaultValue);
		break;
	}
	case CONFIG_IBEACON_MAJOR: {
		StorageHelper::getUint16(_storageStruct.beacon.major, (uint16_t*)target, BEACON_MAJOR, getDefaultValue);
		break;
	}
	case CONFIG_IBEACON_MINOR: {
		StorageHelper::getUint16(_storageStruct.beacon.minor, (uint16_t*)target, BEACON_MINOR, getDefaultValue);
		break;
	}
	case CONFIG_IBEACON_UUID: {
		StorageHelper::getArray<uint8_t>(_storageStruct.beacon.uuid.uuid128, (uint8_t*) target, ((ble_uuid128_t)UUID(BEACON_UUID)).uuid128, 16, getDefaultValue);
		break;
	}
	case CONFIG_IBEACON_TXPOWER: {
		StorageHelper::getInt8(_storageStruct.beacon.txPower, (int8_t*)target, BEACON_RSSI, getDefaultValue);
		break;
	}
//	case CONFIG_WIFI_SETTINGS: {
//		//! copy string, because we clear it on read
//		std::string* p_str = (std::string*) target;
//		if (_wifiSettings == "") {
//			*p_str = "{}";
//		} else {
//			*p_str = _wifiSettings;
//		}
//		break;
//	}
	case CONFIG_TX_POWER: {
		StorageHelper::getInt8(_storageStruct.txPower, (int8_t*)target, TX_POWER, getDefaultValue);
		break;
	}
	case CONFIG_ADV_INTERVAL: {
		StorageHelper::getUint16(_storageStruct.advInterval, (uint16_t*)target, ADVERTISEMENT_INTERVAL, getDefaultValue);
		break;
	}
	case CONFIG_PASSKEY: {
		StorageHelper::getArray<uint8_t>(_storageStruct.passkey, (uint8_t*) target, (uint8_t*)STATIC_PASSKEY, BLE_GAP_PASSKEY_LEN, getDefaultValue);
		break;
	}
	case CONFIG_MIN_ENV_TEMP: {
		StorageHelper::getInt8(_storageStruct.minEnvTemp, (int8_t*)target, MIN_ENV_TEMP, getDefaultValue);
		break;
	}
	case CONFIG_MAX_ENV_TEMP: {
		StorageHelper::getInt8(_storageStruct.maxEnvTemp, (int8_t*)target, MAX_ENV_TEMP, getDefaultValue);
		break;
	}
	case CONFIG_SCAN_DURATION: {
		StorageHelper::getUint16(_storageStruct.scanDuration, (uint16_t*)target, SCAN_DURATION, getDefaultValue);
		break;
	}
	case CONFIG_SCAN_SEND_DELAY: {
		StorageHelper::getUint16(_storageStruct.scanSendDelay, (uint16_t*)target, SCAN_SEND_DELAY, getDefaultValue);
		break;
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		StorageHelper::getUint16(_storageStruct.scanBreakDuration, (uint16_t*)target, SCAN_BREAK_DURATION, getDefaultValue);
		break;
	}
	case CONFIG_BOOT_DELAY: {
		StorageHelper::getUint16(_storageStruct.bootDelay, (uint16_t*)target, BOOT_DELAY, getDefaultValue);
		break;
	}
	case CONFIG_MAX_CHIP_TEMP: {
		StorageHelper::getInt8(_storageStruct.maxChipTemp, (int8_t*)target, MAX_CHIP_TEMP, getDefaultValue);
		break;
	}
	case CONFIG_SCAN_FILTER: {
		StorageHelper::getUint8(_storageStruct.scanFilter, (uint8_t*)target, SCAN_FILTER, getDefaultValue);
		break;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		StorageHelper::getUint16(_storageStruct.scanFilterSendFraction, (uint16_t*)target, SCAN_FILTER_SEND_FRACTION, getDefaultValue);
		break;
	}
	case CONFIG_CROWNSTONE_ID: {
		StorageHelper::getUint16(_storageStruct.crownstoneId, (uint16_t*)target, 0, getDefaultValue);
		break;
	}
	case CONFIG_KEY_ADMIN : {
		StorageHelper::getArray<uint8_t>(_storageStruct.encryptionKeys.owner, (uint8_t*)target, NULL, ENCRYPTION_KEY_LENGTH, getDefaultValue);
		break;
	}
	case CONFIG_KEY_MEMBER : {
		StorageHelper::getArray<uint8_t>(_storageStruct.encryptionKeys.member, (uint8_t*)target, NULL, ENCRYPTION_KEY_LENGTH, getDefaultValue);
		break;
	}
	case CONFIG_KEY_GUEST : {
		StorageHelper::getArray<uint8_t>(_storageStruct.encryptionKeys.guest, (uint8_t*)target, NULL, ENCRYPTION_KEY_LENGTH, getDefaultValue);
		break;
	}
	case CONFIG_SCAN_INTERVAL: {
		StorageHelper::getUint16(_storageStruct.scanInterval, (uint16_t*)target, SCAN_INTERVAL, getDefaultValue);
		break;
	}
	case CONFIG_SCAN_WINDOW: {
		StorageHelper::getUint16(_storageStruct.scanWindow, (uint16_t*)target, SCAN_WINDOW, getDefaultValue);
		break;
	}
	case CONFIG_RELAY_HIGH_DURATION: {
		StorageHelper::getUint16(_storageStruct.relayHighDuration, (uint16_t*)target, RELAY_HIGH_DURATION, getDefaultValue);
		break;
	}
	case CONFIG_LOW_TX_POWER: {
		StorageHelper::getInt8(_storageStruct.lowTxPower, (int8_t*)target, _boardsConfig->minTxPower, getDefaultValue);
		break;
	}
	case CONFIG_VOLTAGE_MULTIPLIER: {
		StorageHelper::getFloat(_storageStruct.voltageMultiplier, (float*)target, _boardsConfig->voltageMultiplier, getDefaultValue);
		break;
	}
	case CONFIG_CURRENT_MULTIPLIER: {
		StorageHelper::getFloat(_storageStruct.currentMultiplier, (float*)target, _boardsConfig->currentMultiplier, getDefaultValue);
		break;
	}
	case CONFIG_VOLTAGE_ZERO: {
		StorageHelper::getInt32(_storageStruct.voltageZero, (int32_t*)target, _boardsConfig->voltageZero, getDefaultValue);
		break;
	}
	case CONFIG_CURRENT_ZERO: {
		StorageHelper::getInt32(_storageStruct.currentZero, (int32_t*)target, _boardsConfig->currentZero, getDefaultValue);
		break;
	}
	case CONFIG_POWER_ZERO: {
		StorageHelper::getInt32(_storageStruct.powerZero, (int32_t*)target, _boardsConfig->powerZero, getDefaultValue);
		break;
	}
//	case CONFIG_POWER_ZERO_AVG_WINDOW: {
//		StorageHelper::getUint16(_storageStruct.powerZeroAvgWindow, (uint16_t*)target, POWER_ZERO_AVG_WINDOW, getDefaultValue);
//		break;
//	}
	case CONFIG_MESH_ACCESS_ADDRESS: {
		StorageHelper::getUint32(_storageStruct.meshAccessAddress, (uint32_t*)target, MESH_ACCESS_ADDRESS, getDefaultValue);
		break;
	}
	case CONFIG_PWM_PERIOD: {
		StorageHelper::getUint32(_storageStruct.pwmInterval, (uint32_t*)target, PWM_PERIOD, getDefaultValue);
		break;
	}
	case CONFIG_SOFT_FUSE_CURRENT_THRESHOLD: {
		StorageHelper::getUint16(_storageStruct.currentThreshold, (uint16_t*)target, CURRENT_USAGE_THRESHOLD, getDefaultValue);
		break;
	}
	case CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM: {
		StorageHelper::getUint16(_storageStruct.currentThresholdPwm, (uint16_t*)target, CURRENT_USAGE_THRESHOLD_PWM, getDefaultValue);
		break;
	}
	case CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP: {
		StorageHelper::getFloat(_storageStruct.pwmTempVoltageThresholdUp, (float*)target, _boardsConfig->pwmTempVoltageThreshold, getDefaultValue);
		break;
	}
	case CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN: {
		StorageHelper::getFloat(_storageStruct.pwmTempVoltageThresholdDown, (float*)target, _boardsConfig->pwmTempVoltageThresholdDown, getDefaultValue);
		break;
	}
	case CONFIG_SWITCHCRAFT_THRESHOLD: {
		StorageHelper::getFloat(_storageStruct.switchcraftThreshold, (float*)target, SWITCHCRAFT_THRESHOLD, getDefaultValue);
		break;
	}
	case CONFIG_MESH_CHANNEL: {
		StorageHelper::getUint8(_storageStruct.meshChannel, (uint8_t*)target, MESH_CHANNEL, getDefaultValue);
		break;
	}
	case CONFIG_UART_ENABLED: {
		StorageHelper::getUint8(_storageStruct.uartEnabled, (uint8_t*)target, 0, getDefaultValue);
		break;
	}
	default: {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		return ERR_UNKNOWN_TYPE;
	}
	}*/
	return ERR_SUCCESS;
}

ERR_CODE Settings::set(uint8_t type, void* target, bool persistent, uint16_t size) {
	/*
	uint8_t* p_item;

	switch(type) {
	case CONFIG_NAME: {
		p_item = (uint8_t*)&_storageStruct.device_name;
		StorageHelper::setString(std::string((char*)target, size), _storageStruct.device_name);
		break;
	}
	case CONFIG_NEARBY_TIMEOUT: {
		p_item = (uint8_t*)&_storageStruct.nearbyTimeout;
		StorageHelper::setUint16(*((uint16_t*)target), (uint32_t&)_storageStruct.nearbyTimeout);
		break;
	}
	case CONFIG_FLOOR: {
		p_item = (uint8_t*)&_storageStruct.floor;
		StorageHelper::setUint8(*((uint8_t*)target), _storageStruct.floor);
		break;
	}
	case CONFIG_IBEACON_MAJOR: {
		p_item = (uint8_t*)&_storageStruct.beacon.major;
		StorageHelper::setUint16(*((uint16_t*)target), (uint32_t&)_storageStruct.beacon.major);
		break;
	}
	case CONFIG_IBEACON_MINOR: {
		p_item = (uint8_t*)&_storageStruct.beacon.minor;
		StorageHelper::setUint16(*((uint16_t*)target),(uint32_t&) _storageStruct.beacon.minor);
		break;
	}
	case CONFIG_IBEACON_UUID: {
		p_item = (uint8_t*)&_storageStruct.beacon.uuid.uuid128;
		StorageHelper::setArray<uint8_t>((uint8_t*) target, _storageStruct.beacon.uuid.uuid128, 16);
		break;
	}
	case CONFIG_IBEACON_TXPOWER: {
		p_item = (uint8_t*)&_storageStruct.beacon.txPower;
		StorageHelper::setInt8(*((int8_t*)target), (int32_t&)_storageStruct.beacon.txPower);
		break;
	}
//	case CONFIG_WIFI_SETTINGS: {
//		p_item = (uint8_t*)&_wifiSettings;
//		if (size > 0) {
//			_wifiSettings = std::string((char*)target, size);
//		}
//		break;
//	}
	case CONFIG_TX_POWER: {
		p_item = (uint8_t*)&_storageStruct.txPower;
		StorageHelper::setInt8(*((int8_t*)target), _storageStruct.txPower);
		break;
	}
	case CONFIG_ADV_INTERVAL: {
		p_item = (uint8_t*)&_storageStruct.advInterval;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.advInterval);
		break;
	}
	case CONFIG_PASSKEY: {
		p_item = (uint8_t*)&_storageStruct.passkey;
		StorageHelper::setArray<uint8_t>((uint8_t*) target, _storageStruct.passkey, BLE_GAP_PASSKEY_LEN);
		break;
	}
	case CONFIG_MIN_ENV_TEMP: {
		p_item = (uint8_t*)&_storageStruct.minEnvTemp;
		StorageHelper::setInt8(*((int8_t*)target), _storageStruct.minEnvTemp);
		break;
	}
	case CONFIG_MAX_ENV_TEMP: {
		p_item = (uint8_t*)&_storageStruct.maxEnvTemp;
		StorageHelper::setInt8(*((int8_t*)target), _storageStruct.maxEnvTemp);
		break;
	}
	case CONFIG_SCAN_DURATION: {
		p_item = (uint8_t*)&_storageStruct.scanDuration;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.scanDuration);
		break;
	}
	case CONFIG_SCAN_SEND_DELAY: {
		p_item = (uint8_t*)&_storageStruct.scanSendDelay;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.scanSendDelay);
		break;
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		p_item = (uint8_t*)&_storageStruct.scanBreakDuration;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.scanBreakDuration);
		break;
	}
	case CONFIG_BOOT_DELAY: {
		p_item = (uint8_t*)&_storageStruct.bootDelay;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.bootDelay);
		break;
	}
	case CONFIG_MAX_CHIP_TEMP: {
		p_item = (uint8_t*)&_storageStruct.maxChipTemp;
		StorageHelper::setInt8(*((int8_t*)target), _storageStruct.maxChipTemp);
		break;
	}
	case CONFIG_SCAN_FILTER: {
		p_item = (uint8_t*)&_storageStruct.scanFilter;
		StorageHelper::setUint8(*((uint8_t*)target), _storageStruct.scanFilter);
		break;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		p_item = (uint8_t*)&_storageStruct.scanFilterSendFraction;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.scanFilterSendFraction);
		break;
	}
	case CONFIG_CROWNSTONE_ID: {
		p_item = (uint8_t*)&_storageStruct.crownstoneId;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.crownstoneId);
		break;
	}
	case CONFIG_KEY_ADMIN : {
		p_item = (uint8_t*)&_storageStruct.encryptionKeys.owner;
		StorageHelper::setArray<uint8_t>((uint8_t*)target, _storageStruct.encryptionKeys.owner, ENCRYPTION_KEY_LENGTH);
		break;
	}
	case CONFIG_KEY_MEMBER : {
		p_item = (uint8_t*)&_storageStruct.encryptionKeys.member;
		StorageHelper::setArray<uint8_t>((uint8_t*)target, _storageStruct.encryptionKeys.member, ENCRYPTION_KEY_LENGTH);
		break;
	}
	case CONFIG_KEY_GUEST : {
		p_item = (uint8_t*)&_storageStruct.encryptionKeys.guest;
		StorageHelper::setArray<uint8_t>((uint8_t*)target, _storageStruct.encryptionKeys.guest, ENCRYPTION_KEY_LENGTH);
		break;
	}
//	case CONFIG_ADC_BURST_SAMPLE_RATE: {
//		p_item = (uint8_t*)&_storageStruct.adcBurstSampleRate;
//		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.adcBurstSampleRate);
//		break;
//	}
//	case CONFIG_POWER_SAMPLE_BURST_INTERVAL: {
//		p_item = (uint8_t*)&_storageStruct.powerSampleBurstInterval;
//		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.powerSampleBurstInterval);
//		break;
//	}
//	case CONFIG_POWER_SAMPLE_CONT_INTERVAL: {
//		p_item = (uint8_t*)&_storageStruct.powerSampleContInterval;
//		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.powerSampleContInterval);
//		break;
//	}
//	case CONFIG_ADC_CONT_SAMPLE_RATE: {
//		p_item = (uint8_t*)&_storageStruct.adcContSampleRate;
//		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.adcContSampleRate);
//		break;
//	}
	case CONFIG_SCAN_INTERVAL: {
		p_item = (uint8_t*)&_storageStruct.scanInterval;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.scanInterval);
		break;
	}
	case CONFIG_SCAN_WINDOW: {
		p_item = (uint8_t*)&_storageStruct.scanWindow;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.scanWindow);
		break;
	}
	case CONFIG_RELAY_HIGH_DURATION: {
		p_item = (uint8_t*)&_storageStruct.relayHighDuration;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.relayHighDuration);
		break;
	}
	case CONFIG_LOW_TX_POWER: {
		p_item = (uint8_t*)&_storageStruct.lowTxPower;
		StorageHelper::setInt8(*((int8_t*)target), _storageStruct.lowTxPower);
		break;
	}
	case CONFIG_VOLTAGE_MULTIPLIER: {
		p_item = (uint8_t*)&_storageStruct.voltageMultiplier;
		StorageHelper::setFloat(*((float*)target), _storageStruct.voltageMultiplier);
		break;
	}
	case CONFIG_CURRENT_MULTIPLIER: {
		p_item = (uint8_t*)&_storageStruct.currentMultiplier;
		StorageHelper::setFloat(*((float*)target), _storageStruct.currentMultiplier);
		break;
	}
	case CONFIG_VOLTAGE_ZERO: {
		p_item = (uint8_t*)&_storageStruct.voltageZero;
		StorageHelper::setInt32(*((int32_t*)target), _storageStruct.voltageZero);
		break;
	}
	case CONFIG_CURRENT_ZERO: {
		p_item = (uint8_t*)&_storageStruct.currentZero;
		StorageHelper::setInt32(*((int32_t*)target), _storageStruct.currentZero);
		break;
	}
	case CONFIG_POWER_ZERO: {
		p_item = (uint8_t*)&_storageStruct.powerZero;
		StorageHelper::setInt32(*((int32_t*)target), _storageStruct.powerZero);
		break;
	}
	case CONFIG_POWER_ZERO_AVG_WINDOW:{
		p_item = (uint8_t*)&_storageStruct.powerZeroAvgWindow;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.powerZeroAvgWindow);
		break;
	}
	case CONFIG_MESH_ACCESS_ADDRESS:{
		p_item = (uint8_t*)&_storageStruct.meshAccessAddress;
		StorageHelper::setUint32(*((uint32_t*)target), _storageStruct.meshAccessAddress);
		break;
	}
	case CONFIG_PWM_PERIOD:{
		p_item = (uint8_t*)&_storageStruct.pwmInterval;
		StorageHelper::setUint32(*((uint32_t*)target), _storageStruct.pwmInterval);
		break;
	}
	case CONFIG_SOFT_FUSE_CURRENT_THRESHOLD: {
		p_item = (uint8_t*)&_storageStruct.currentThreshold;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.currentThreshold);
		break;
	}
	case CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM: {
		p_item = (uint8_t*)&_storageStruct.currentThresholdPwm;
		StorageHelper::setUint16(*((uint16_t*)target), _storageStruct.currentThresholdPwm);
		break;
	}
	case CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP: {
		p_item = (uint8_t*)&_storageStruct.pwmTempVoltageThresholdUp;
		StorageHelper::setFloat(*((float*)target), _storageStruct.pwmTempVoltageThresholdUp);
		break;
	}
	case CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN: {
		p_item = (uint8_t*)&_storageStruct.pwmTempVoltageThresholdDown;
		StorageHelper::setFloat(*((float*)target), _storageStruct.pwmTempVoltageThresholdDown);
		break;
	}
	case CONFIG_SWITCHCRAFT_THRESHOLD: {
		p_item = (uint8_t*)&_storageStruct.switchcraftThreshold;
		StorageHelper::setFloat(*((float*)target), _storageStruct.switchcraftThreshold);
		break;
	}
	case CONFIG_MESH_CHANNEL: {
		p_item = (uint8_t*)&_storageStruct.meshChannel;
		StorageHelper::setUint8(*((uint8_t*)target), _storageStruct.meshChannel);
		break;
	}
	case CONFIG_UART_ENABLED: {
		p_item = (uint8_t*)&_storageStruct.uartEnabled;
		StorageHelper::setUint8(*((uint8_t*)target), _storageStruct.uartEnabled);
		break;
	}
	default: {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		return ERR_UNKNOWN_TYPE;
	}
	}

	if (persistent) {
		size = getSettingsItemSize(type);
		// minimum item size we can store is 4
		if (size < 4) {
			size = 4;
		}
		savePersistentStorageItem(p_item, size);
	}*/
	return ERR_SUCCESS;
}

bool Settings::updateFlag(uint8_t type, bool value, bool persistent) {
	bool default_value = false;

/*
	// as long as persistent flag storage is disabled, do not do any checks, but still
	// update the struct so that other functions can check if the flag is set or not
#if PERSISTENT_FLAGS_DISABLED==0
	if (value == isSet(type)) {
		LOGi("flag is already set");
		return true;
	}
#endif

	__attribute__((unused)) uint8_t* p_item;

	switch(type) {
	case CONFIG_MESH_ENABLED : {
		p_item = (uint8_t*)&_storageStruct.meshEnabled;
		StorageHelper::setUint8(value, _storageStruct.meshEnabled);
//		_storageStruct.flagsBit.meshDisabled = !value;
		break;
	}
	case CONFIG_ENCRYPTION_ENABLED : {
		p_item = (uint8_t*)&_storageStruct.encryptionEnabled;
		StorageHelper::setUint8(value, _storageStruct.encryptionEnabled);
//		_storageStruct.flagsBit.encryptionDisabled = !value;
		break;
	}
	case CONFIG_IBEACON_ENABLED : {
		p_item = (uint8_t*)&_storageStruct.iBeaconEnabled;
		StorageHelper::setUint8(value, _storageStruct.iBeaconEnabled);
//		_storageStruct.flagsBit.iBeaconDisabled = !value;
		break;
	}
	case CONFIG_SCANNER_ENABLED : {
		p_item = (uint8_t*)&_storageStruct.scannerEnabled;
		StorageHelper::setUint8(value, _storageStruct.scannerEnabled);
//		_storageStruct.flagsBit.scannerDisabled = !value;
		break;
	}
	case CONFIG_CONT_POWER_SAMPLER_ENABLED : {
		p_item = (uint8_t*)&_storageStruct.continuousPowerSamplerEnabled;
		StorageHelper::setUint8(value, _storageStruct.continuousPowerSamplerEnabled);
//		_storageStruct.flagsBit.continuousPowerSamplerDisabled = !value;
		break;
	}
	case CONFIG_DEFAULT_ON: {
		p_item = (uint8_t*)&_storageStruct.defaultOff;
		StorageHelper::setUint8(value, _storageStruct.defaultOff);
//		_storageStruct.flagsBit.defaultOff = !value;
		break;
	}
	case CONFIG_PWM_ALLOWED: {
		p_item = (uint8_t*)&_storageStruct.pwmAllowed;
		StorageHelper::setUint8(value, _storageStruct.pwmAllowed);
		break;
	}
	case CONFIG_SWITCH_LOCKED: {
		p_item = (uint8_t*)&_storageStruct.switchLocked;
		StorageHelper::setUint8(value, _storageStruct.switchLocked);
		break;
	}
	case CONFIG_SWITCHCRAFT_ENABLED: {
		p_item = (uint8_t*)&_storageStruct.switchcraftEnabled;
		StorageHelper::setUint8(value, _storageStruct.switchcraftEnabled);
		break;
	}
	default: {
		return false;
	}
	}

	// if persistent storage is disable for flags, do not update pstorage
#if PERSISTENT_FLAGS_DISABLED==0
	if (persistent) {
		savePersistentStorageItem(p_item, 4);
//		savePersistentStorageItem(&_storageStruct.flags);
	}
#else
	LOGw("persistent storage for flags disabled!!");
#endif
	*/
	return default_value;
}

bool Settings::readFlag(uint8_t type, bool& value) {
	bool default_value = false;
	/*
#ifdef PRINT_DEBUG
	LOGd("read flag %d", type);
#endif
	switch(type) {
	case CONFIG_MESH_ENABLED: {
		StorageHelper::getUint8(_storageStruct.meshEnabled, (uint8_t*)&value, MESHING, false);
//		value = !_storageStruct.flagsBit.meshDisabled;
//		default_value = MESHING;
		break;
	}
	case CONFIG_ENCRYPTION_ENABLED: {
		StorageHelper::getUint8(_storageStruct.encryptionEnabled, (uint8_t*)&value, ENCRYPTION, false);
//		value = !_storageStruct.flagsBit.encryptionDisabled;
//		default_value = ENCRYPTION;
		break;
	}
	case CONFIG_IBEACON_ENABLED: {
		StorageHelper::getUint8(_storageStruct.iBeaconEnabled, (uint8_t*)&value, IBEACON, false);
//		value = !_storageStruct.flagsBit.iBeaconDisabled;
//		default_value = IBEACON;
		break;
	}
	case CONFIG_SCANNER_ENABLED: {
		StorageHelper::getUint8(_storageStruct.scannerEnabled, (uint8_t*)&value, INTERVAL_SCANNER_ENABLED, false);
//		value = !_storageStruct.flagsBit.scannerDisabled;
//		default_value = INTERVAL_SCANNER_ENABLED;
		break;
	}
	case CONFIG_CONT_POWER_SAMPLER_ENABLED: {
		StorageHelper::getUint8(_storageStruct.continuousPowerSamplerEnabled, (uint8_t*)&value, CONTINUOUS_POWER_SAMPLER, false);
//		value = !_storageStruct.flagsBit.continuousPowerSamplerDisabled;
//		default_value = CONTINUOUS_POWER_SAMPLER;
		break;
	}
	case CONFIG_DEFAULT_ON: {
		StorageHelper::getUint8(_storageStruct.defaultOff, (uint8_t*)&value, DEFAULT_ON, false);
//		value = !_storageStruct.flagsBit.defaultOff;
//		default_value = DEFAULT_ON;
		break;
	}
	case CONFIG_PWM_ALLOWED: {
		StorageHelper::getUint8(_storageStruct.pwmAllowed, (uint8_t*)&value, 0, false);
		break;
	}
	case CONFIG_SWITCH_LOCKED: {
		StorageHelper::getUint8(_storageStruct.switchLocked, (uint8_t*)&value, 0, false);
		break;
	}
	case CONFIG_SWITCHCRAFT_ENABLED: {
		StorageHelper::getUint8(_storageStruct.switchcraftEnabled, (uint8_t*)&value, 0, false);
		break;
	}
	default:
		return false;
	}

//	if (_storageStruct.flagsBit.flagsUninitialized) {
//		value = default_value;
//		return true;
//	}
	*/

	return default_value;
}

bool Settings::isSet(uint8_t type) {
	bool enabled = false;
	switch(type) {
	case CONFIG_MESH_ENABLED :
	case CONFIG_ENCRYPTION_ENABLED :
	case CONFIG_IBEACON_ENABLED :
	case CONFIG_SCANNER_ENABLED :
	case CONFIG_CONT_POWER_SAMPLER_ENABLED :
	case CONFIG_DEFAULT_ON:
	case CONFIG_PWM_ALLOWED:
	case CONFIG_SWITCH_LOCKED:
	case CONFIG_SWITCHCRAFT_ENABLED: 
		readFlag(type, enabled);
	default: {}
	}
	return enabled;
}

/** Get a handle to the persistent storage struct and load it from FLASH.
 *
 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
 * writing less than a minimal block strains the memory just as much as flashing the entire block.
 * Hence, there is an entire struct that can be filled and flashed at once.
 */
void Settings::loadPersistentStorage() {
	//TODO: _storage->readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}


void Settings::savePersistentStorageItem(uint8_t* item, uint8_t size) {
	// TODO: uint32_t offset = StorageHelper::getOffset(&_storageStruct, item);
	// _storage->writeItem(_storageHandle, offset, item, size);
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
	// TODO: _storage->writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void Settings::saveIBeaconPersistent() {
	// TODO: savePersistentStorageItem((uint8_t*)&_storageStruct.beacon, sizeof(_storageStruct.beacon));
}

void Settings::factoryReset(uint32_t resetCode) {
	if (resetCode != FACTORY_RESET_CODE) {
		LOGe("wrong reset code!");
		return;
	}

	// TODO:	_storage->clearStorage(PS_ID_CONFIGURATION);
	// TODO: memset(&_storageStruct, 0xFF, sizeof(_storageStruct));
}
