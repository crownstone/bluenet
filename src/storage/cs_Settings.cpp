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

Settings::Settings() : _initialized(false), _storage(NULL) {
};

void Settings::init() {
	_storage = &Storage::getInstance();

	if (!_storage->isInitialized()) {
		LOGe(FMT_NOT_INITIALIZED, "Storage");
		return;
	}

	_storage->getHandle(PS_ID_CONFIGURATION, _storageHandle);
	loadPersistentStorage();

	// need to initialize the flags if they are uninitialized, e.g. first time
//	if (_storageStruct.flagsBit.flagsUninitialized) {
//		initFlags();
//	}

	_initialized = true;
}

//	void writeToStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
ERR_CODE Settings::writeToStorage(uint8_t type, uint8_t* payload, uint16_t length, bool persistent) {

	/////////////////////////////////////////////////
	//// SPECIAL CASES
	/////////////////////////////////////////////////
	switch(type) {
//	case CONFIG_WIFI_SETTINGS: {
//		LOGi("Temporarily store wifi settings");
//		//! max length '{ "ssid": "32 bytes", "key": "32 bytes"}', 64+24 bytes = 88 bytes
//		if (length > 88) {
//			LOGe("Wifi settings string too long");
//			return ERR_WRONG_PAYLOAD_LENGTH;
//		}
//		_wifiSettings = std::string((char*)payload, length);
//		LOGi("Stored wifi settings [%i]: %s", length, _wifiSettings.c_str());
//		return ERR_SUCCESS;
//	}

	// todo: if we want to disable write access for encryption keys outside the setup mode
//	/////////////////////////////////////////////////
//	//// WRITE DISABLED
//	/////////////////////////////////////////////////
//	case CONFIG_KEY_OWNER :
//	case CONFIG_KEY_MEMBER :
//	case CONFIG_KEY_GUEST : {
//		uint8_t opMode;
//		State::getInstance().get(STATE_OPERATION_MODE, opMode);
//		if (opMode != OPERATION_MODE_SETUP) {
//			LOGw("Reading encryption keys only available in setup mode!");
//			return false;
//		}
//	}
	}

	/////////////////////////////////////////////////
	//// DEFAULT
	/////////////////////////////////////////////////
	ERR_CODE error_code;
	error_code = verify(type, payload, length);
	if (SUCCESS(error_code)) {
		error_code = set(type, payload, persistent, length);
		EventDispatcher::getInstance().dispatch(type, payload, length);
	}
	return error_code;
}

ERR_CODE Settings::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {

	/////////////////////////////////////////////////
	//// SPECIAL CASES
	/////////////////////////////////////////////////
	switch(type) {
//	case CONFIG_WIFI_SETTINGS: {
//		LOGd("Read wifi settings. Does reset it.");
//		//! copy string, because we clear it on read
//		std::string str;
//		if (_wifiSettings == "") {
//			str = "{}";
//		} else {
//			str = _wifiSettings;
//		}
//		streamBuffer->fromString(str);
//		streamBuffer->setType(type);
//		_wifiSettings = "";
//		LOGd("Wifi settings read");
//		return ERR_SUCCESS;
//	}

	// todo: if we want to disable read access for encryption keys outside the setup mode
//	/////////////////////////////////////////////////
//	//// READ DISABLED
//	/////////////////////////////////////////////////
//	case CONFIG_KEY_OWNER :
//	case CONFIG_KEY_MEMBER :
//	case CONFIG_KEY_GUEST : {
//		uint8_t opMode;
//		State::getInstance().get(STATE_OPERATION_MODE, opMode);
//		if (opMode != OPERATION_MODE_SETUP) {
//			LOGw("Reading encryption keys only avilable in setup mode!");
//			return false;
//		}
//	}

	}

	/////////////////////////////////////////////////
	//// DEFAULT
	/////////////////////////////////////////////////
	ERR_CODE error_code;
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
		return error_code;
	} else {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		return ERR_CONFIG_NOT_FOUND;
	}
}

ERR_CODE Settings::verify(uint8_t type, uint8_t* payload, uint8_t length) {
	switch(type) {
	/////////////////////////////////////////////////
	//// UINT 8
	/////////////////////////////////////////////////
	case CONFIG_SCAN_FILTER:
	case CONFIG_CURRENT_LIMIT:
	case CONFIG_FLOOR: {
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
	case CONFIG_ADC_BURST_SAMPLE_RATE:
	case CONFIG_POWER_SAMPLE_BURST_INTERVAL:
	case CONFIG_POWER_SAMPLE_CONT_INTERVAL:
	case CONFIG_ADC_CONT_SAMPLE_RATE:
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
	case CONFIG_POWER_ZERO_AVG_WINDOW: {
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
	//// FLOAT
	/////////////////////////////////////////////////
	case CONFIG_VOLTAGE_MULTIPLIER:
	case CONFIG_CURRENT_MULTIPLIER:
	case CONFIG_VOLTAGE_ZERO:
	case CONFIG_CURRENT_ZERO:
	case CONFIG_POWER_ZERO: {
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
	case CONFIG_KEY_AMIN :
	case CONFIG_KEY_MEMBER :
	case CONFIG_KEY_GUEST : {
		if (length != ENCYRPTION_KEY_LENGTH) {
			LOGe(FMT_ERR_EXPECTED_RECEIVED, ENCYRPTION_KEY_LENGTH, length);
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
	case CONFIG_CONT_POWER_SAMPLER_ENABLED : {
//		updateFlag(type, payload[0] != 0, persistent);
		LOGe("Write disabled. Use commands to enable/disable");
		return ERR_WRITE_CONFIG_DISABLED;
	}

	default: {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		return ERR_CONFIG_NOT_FOUND;
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
	case CONFIG_ADC_BURST_SAMPLE_RATE:
	case CONFIG_POWER_SAMPLE_BURST_INTERVAL:
	case CONFIG_POWER_SAMPLE_CONT_INTERVAL:
	case CONFIG_ADC_CONT_SAMPLE_RATE:
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
	case CONFIG_POWER_ZERO_AVG_WINDOW: {
		return 2;
	}

	/////////////////////////////////////////////////
	//// UINT 32
	/////////////////////////////////////////////////
	case CONFIG_PWM_PERIOD:
	case CONFIG_MESH_ACCESS_ADDRESS:
		return 4;

	/////////////////////////////////////////////////
	//// FLOAT
	/////////////////////////////////////////////////
	case CONFIG_VOLTAGE_MULTIPLIER:
	case CONFIG_CURRENT_MULTIPLIER:
	case CONFIG_VOLTAGE_ZERO:
	case CONFIG_CURRENT_ZERO:
	case CONFIG_POWER_ZERO: {
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
	case CONFIG_KEY_AMIN :
	case CONFIG_KEY_MEMBER :
	case CONFIG_KEY_GUEST : {
		return ENCYRPTION_KEY_LENGTH;
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

	default:
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		return 0;
	}
}

ERR_CODE Settings::get(uint8_t type, void* target) {
	uint16_t size = 0;
	return get(type, target, size);
}

ERR_CODE Settings::get(uint8_t type, void* target, uint16_t& size) {
	switch(type) {
	case CONFIG_NAME: {
		char default_name[32];
		sprintf(default_name, "%s_%s", STRINGIFY(BLUETOOTH_NAME), STRINGIFY(COMPILATION_TIME));
		Storage::getString(_storageStruct.device_name, (char*) target, default_name, size);
		break;
	}
	case CONFIG_FLOOR: {
		Storage::getUint8(_storageStruct.floor, (uint8_t*)target, 0);
		break;
	}
	case CONFIG_NEARBY_TIMEOUT: {
		Storage::getUint16(_storageStruct.nearbyTimeout, (uint16_t*)target, TRACKDEVICE_DEFAULT_TIMEOUT_COUNT);
		break;
	}
	case CONFIG_IBEACON_MAJOR: {
		Storage::getUint16(_storageStruct.beacon.major, (uint16_t*)target, BEACON_MAJOR);
		break;
	}
	case CONFIG_IBEACON_MINOR: {
		Storage::getUint16(_storageStruct.beacon.minor, (uint16_t*)target, BEACON_MINOR);
		break;
	}
	case CONFIG_IBEACON_UUID: {
		Storage::getArray<uint8_t>(_storageStruct.beacon.uuid.uuid128, (uint8_t*) target, ((ble_uuid128_t)UUID(BEACON_UUID)).uuid128, 16);
		break;
	}
	case CONFIG_IBEACON_TXPOWER: {
		Storage::getInt8(_storageStruct.beacon.txPower, (int8_t*)target, BEACON_RSSI);
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
		Storage::getInt8(_storageStruct.txPower, (int8_t*)target, TX_POWER);
		break;
	}
	case CONFIG_ADV_INTERVAL: {
		Storage::getUint16(_storageStruct.advInterval, (uint16_t*)target, ADVERTISEMENT_INTERVAL);
		break;
	}
	case CONFIG_PASSKEY: {
		Storage::getArray<uint8_t>(_storageStruct.passkey, (uint8_t*) target, (uint8_t*)STATIC_PASSKEY, BLE_GAP_PASSKEY_LEN);
		break;
	}
	case CONFIG_MIN_ENV_TEMP: {
		Storage::getInt8(_storageStruct.minEnvTemp, (int8_t*)target, MIN_ENV_TEMP);
		break;
	}
	case CONFIG_MAX_ENV_TEMP: {
		Storage::getInt8(_storageStruct.maxEnvTemp, (int8_t*)target, MAX_ENV_TEMP);
		break;
	}
	case CONFIG_SCAN_DURATION: {
		Storage::getUint16(_storageStruct.scanDuration, (uint16_t*)target, SCAN_DURATION);
		break;
	}
	case CONFIG_SCAN_SEND_DELAY: {
		Storage::getUint16(_storageStruct.scanSendDelay, (uint16_t*)target, SCAN_SEND_DELAY);
		break;
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		Storage::getUint16(_storageStruct.scanBreakDuration, (uint16_t*)target, SCAN_BREAK_DURATION);
		break;
	}
	case CONFIG_BOOT_DELAY: {
		Storage::getUint16(_storageStruct.bootDelay, (uint16_t*)target, BOOT_DELAY);
		break;
	}
	case CONFIG_MAX_CHIP_TEMP: {
		Storage::getInt8(_storageStruct.maxChipTemp, (int8_t*)target, MAX_CHIP_TEMP);
		break;
	}
	case CONFIG_SCAN_FILTER: {
		Storage::getUint8(_storageStruct.scanFilter, (uint8_t*)target, SCAN_FILTER);
		break;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		Storage::getUint16(_storageStruct.scanFilterSendFraction, (uint16_t*)target, SCAN_FILTER_SEND_FRACTION);
		break;
	}
	case CONFIG_CURRENT_LIMIT: {
		Storage::getUint8(_storageStruct.currentLimit, (uint8_t*)target, CURRENT_LIMIT);
		break;
	}
	case CONFIG_CROWNSTONE_ID: {
		Storage::getUint16(_storageStruct.crownstoneId, (uint16_t*)target, 0);
		break;
	}
	case CONFIG_KEY_AMIN : {
		Storage::getArray<uint8_t>(_storageStruct.encryptionKeys.owner, (uint8_t*)target, NULL, ENCYRPTION_KEY_LENGTH);
		break;
	}
	case CONFIG_KEY_MEMBER : {
		Storage::getArray<uint8_t>(_storageStruct.encryptionKeys.member, (uint8_t*)target, NULL, ENCYRPTION_KEY_LENGTH);
		break;
	}
	case CONFIG_KEY_GUEST : {
		Storage::getArray<uint8_t>(_storageStruct.encryptionKeys.guest, (uint8_t*)target, NULL, ENCYRPTION_KEY_LENGTH);
		break;
	}
	case CONFIG_ADC_BURST_SAMPLE_RATE: {
		Storage::getUint16(_storageStruct.adcBurstSampleRate, (uint16_t*)target, CS_ADC_SAMPLE_RATE);
		break;
	}
	case CONFIG_POWER_SAMPLE_BURST_INTERVAL: {
		Storage::getUint16(_storageStruct.powerSampleBurstInterval, (uint16_t*)target, POWER_SAMPLE_BURST_INTERVAL);
		break;
	}
	case CONFIG_POWER_SAMPLE_CONT_INTERVAL: {
		Storage::getUint16(_storageStruct.powerSampleContInterval, (uint16_t*)target, POWER_SAMPLE_CONT_INTERVAL);
		break;
	}
	case CONFIG_ADC_CONT_SAMPLE_RATE: {
		Storage::getUint16(_storageStruct.adcContSampleRate, (uint16_t*)target, CS_ADC_SAMPLE_RATE);
		break;
	}
	case CONFIG_SCAN_INTERVAL: {
		Storage::getUint16(_storageStruct.scanInterval, (uint16_t*)target, SCAN_INTERVAL);
		break;
	}
	case CONFIG_SCAN_WINDOW: {
		Storage::getUint16(_storageStruct.scanWindow, (uint16_t*)target, SCAN_WINDOW);
		break;
	}
	case CONFIG_RELAY_HIGH_DURATION: {
		Storage::getUint16(_storageStruct.relayHighDuration, (uint16_t*)target, RELAY_HIGH_DURATION);
		break;
	}
	case CONFIG_LOW_TX_POWER: {
		Storage::getInt8(_storageStruct.lowTxPower, (int8_t*)target, LOW_TX_POWER);
		break;
	}
	case CONFIG_VOLTAGE_MULTIPLIER: {
		Storage::getFloat(_storageStruct.voltageMultiplier, (float*)target, VOLTAGE_MULTIPLIER);
		break;
	}
	case CONFIG_CURRENT_MULTIPLIER: {
		Storage::getFloat(_storageStruct.currentMultiplier, (float*)target, CURRENT_MULTIPLIER);
		break;
	}
	case CONFIG_VOLTAGE_ZERO: {
		Storage::getFloat(_storageStruct.voltageZero, (float*)target, VOLTAGE_ZERO);
		break;
	}
	case CONFIG_CURRENT_ZERO: {
		Storage::getFloat(_storageStruct.currentZero, (float*)target, CURRENT_ZERO);
		break;
	}
	case CONFIG_POWER_ZERO: {
		Storage::getFloat(_storageStruct.powerZero, (float*)target, POWER_ZERO);
		break;
	}
	case CONFIG_POWER_ZERO_AVG_WINDOW: {
		Storage::getUint16(_storageStruct.powerZeroAvgWindow, (uint16_t*)target, POWER_ZERO_AVG_WINDOW);
		break;
	}
	case CONFIG_MESH_ACCESS_ADDRESS: {
		Storage::getUint32(_storageStruct.meshAccessAddress, (uint32_t*)target, MESH_ACCESS_ADDRESS);
		break;
	}
	case CONFIG_PWM_PERIOD: {
		Storage::getUint32(_storageStruct.pwmInterval, (uint32_t*)target, PWM_PERIOD);
		break;
	}
	default: {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		return ERR_CONFIG_NOT_FOUND;
	}
	}
	return ERR_SUCCESS;
}

ERR_CODE Settings::set(uint8_t type, void* target, bool persistent, uint16_t size) {

	uint8_t* p_item;

	switch(type) {
	case CONFIG_NAME: {
		p_item = (uint8_t*)&_storageStruct.device_name;
		Storage::setString(std::string((char*)target, size), _storageStruct.device_name);
		break;
	}
	case CONFIG_NEARBY_TIMEOUT: {
		p_item = (uint8_t*)&_storageStruct.nearbyTimeout;
		Storage::setUint16(*((uint16_t*)target), (uint32_t&)_storageStruct.nearbyTimeout);
		break;
	}
	case CONFIG_FLOOR: {
		p_item = (uint8_t*)&_storageStruct.floor;
		Storage::setUint8(*((uint8_t*)target), _storageStruct.floor);
		break;
	}
	case CONFIG_IBEACON_MAJOR: {
		p_item = (uint8_t*)&_storageStruct.beacon.major;
		Storage::setUint16(*((uint16_t*)target), (uint32_t&)_storageStruct.beacon.major);
		break;
	}
	case CONFIG_IBEACON_MINOR: {
		p_item = (uint8_t*)&_storageStruct.beacon.minor;
		Storage::setUint16(*((uint16_t*)target),(uint32_t&) _storageStruct.beacon.minor);
		break;
	}
	case CONFIG_IBEACON_UUID: {
		p_item = (uint8_t*)&_storageStruct.beacon.uuid.uuid128;
		Storage::setArray<uint8_t>((uint8_t*) target, _storageStruct.beacon.uuid.uuid128, 16);
		break;
	}
	case CONFIG_IBEACON_TXPOWER: {
		p_item = (uint8_t*)&_storageStruct.beacon.txPower;
		Storage::setInt8(*((int8_t*)target), (int32_t&)_storageStruct.beacon.txPower);
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
		Storage::setInt8(*((int8_t*)target), _storageStruct.txPower);
		break;
	}
	case CONFIG_ADV_INTERVAL: {
		p_item = (uint8_t*)&_storageStruct.advInterval;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.advInterval);
		break;
	}
	case CONFIG_PASSKEY: {
		p_item = (uint8_t*)&_storageStruct.passkey;
		Storage::setArray<uint8_t>((uint8_t*) target, _storageStruct.passkey, BLE_GAP_PASSKEY_LEN);
		break;
	}
	case CONFIG_MIN_ENV_TEMP: {
		p_item = (uint8_t*)&_storageStruct.minEnvTemp;
		Storage::setInt8(*((int8_t*)target), _storageStruct.minEnvTemp);
		break;
	}
	case CONFIG_MAX_ENV_TEMP: {
		p_item = (uint8_t*)&_storageStruct.maxEnvTemp;
		Storage::setInt8(*((int8_t*)target), _storageStruct.maxEnvTemp);
		break;
	}
	case CONFIG_SCAN_DURATION: {
		p_item = (uint8_t*)&_storageStruct.scanDuration;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanDuration);
		break;
	}
	case CONFIG_SCAN_SEND_DELAY: {
		p_item = (uint8_t*)&_storageStruct.scanSendDelay;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanSendDelay);
		break;
	}
	case CONFIG_SCAN_BREAK_DURATION: {
		p_item = (uint8_t*)&_storageStruct.scanBreakDuration;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanBreakDuration);
		break;
	}
	case CONFIG_BOOT_DELAY: {
		p_item = (uint8_t*)&_storageStruct.bootDelay;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.bootDelay);
		break;
	}
	case CONFIG_MAX_CHIP_TEMP: {
		p_item = (uint8_t*)&_storageStruct.maxChipTemp;
		Storage::setInt8(*((int8_t*)target), _storageStruct.maxChipTemp);
		break;
	}
	case CONFIG_SCAN_FILTER: {
		p_item = (uint8_t*)&_storageStruct.scanFilter;
		Storage::setUint8(*((uint8_t*)target), _storageStruct.scanFilter);
		break;
	}
	case CONFIG_SCAN_FILTER_SEND_FRACTION: {
		p_item = (uint8_t*)&_storageStruct.scanFilterSendFraction;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanFilterSendFraction);
		break;
	}
	case CONFIG_CURRENT_LIMIT: {
		p_item = (uint8_t*)&_storageStruct.currentLimit;
		Storage::setUint8(*((uint8_t*)target), _storageStruct.currentLimit);
		break;
	}
	case CONFIG_CROWNSTONE_ID: {
		p_item = (uint8_t*)&_storageStruct.crownstoneId;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.crownstoneId);
		break;
	}
	case CONFIG_KEY_AMIN : {
		p_item = (uint8_t*)&_storageStruct.encryptionKeys.owner;
		Storage::setArray<uint8_t>((uint8_t*)target, _storageStruct.encryptionKeys.owner, ENCYRPTION_KEY_LENGTH);
		break;
	}
	case CONFIG_KEY_MEMBER : {
		p_item = (uint8_t*)&_storageStruct.encryptionKeys.member;
		Storage::setArray<uint8_t>((uint8_t*)target, _storageStruct.encryptionKeys.member, ENCYRPTION_KEY_LENGTH);
		break;
	}
	case CONFIG_KEY_GUEST : {
		p_item = (uint8_t*)&_storageStruct.encryptionKeys.guest;
		Storage::setArray<uint8_t>((uint8_t*)target, _storageStruct.encryptionKeys.guest, ENCYRPTION_KEY_LENGTH);
		break;
	}
	case CONFIG_ADC_BURST_SAMPLE_RATE: {
		p_item = (uint8_t*)&_storageStruct.adcBurstSampleRate;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.adcBurstSampleRate);
		break;
	}
	case CONFIG_POWER_SAMPLE_BURST_INTERVAL: {
		p_item = (uint8_t*)&_storageStruct.powerSampleBurstInterval;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.powerSampleBurstInterval);
		break;
	}
	case CONFIG_POWER_SAMPLE_CONT_INTERVAL: {
		p_item = (uint8_t*)&_storageStruct.powerSampleContInterval;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.powerSampleContInterval);
		break;
	}
	case CONFIG_ADC_CONT_SAMPLE_RATE: {
		p_item = (uint8_t*)&_storageStruct.adcContSampleRate;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.adcContSampleRate);
		break;
	}
	case CONFIG_SCAN_INTERVAL: {
		p_item = (uint8_t*)&_storageStruct.scanInterval;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanInterval);
		break;
	}
	case CONFIG_SCAN_WINDOW: {
		p_item = (uint8_t*)&_storageStruct.scanWindow;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.scanWindow);
		break;
	}
	case CONFIG_RELAY_HIGH_DURATION: {
		p_item = (uint8_t*)&_storageStruct.relayHighDuration;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.relayHighDuration);
		break;
	}
	case CONFIG_LOW_TX_POWER: {
		p_item = (uint8_t*)&_storageStruct.lowTxPower;
		Storage::setInt8(*((int8_t*)target), _storageStruct.lowTxPower);
		break;
	}
	case CONFIG_VOLTAGE_MULTIPLIER: {
		p_item = (uint8_t*)&_storageStruct.voltageMultiplier;
		Storage::setFloat(*((float*)target), _storageStruct.voltageMultiplier);
		break;
	}
	case CONFIG_CURRENT_MULTIPLIER: {
		p_item = (uint8_t*)&_storageStruct.currentMultiplier;
		Storage::setFloat(*((float*)target), _storageStruct.currentMultiplier);
		break;
	}
	case CONFIG_VOLTAGE_ZERO: {
		p_item = (uint8_t*)&_storageStruct.voltageZero;
		Storage::setFloat(*((float*)target), _storageStruct.voltageZero);
		break;
	}
	case CONFIG_CURRENT_ZERO: {
		p_item = (uint8_t*)&_storageStruct.currentZero;
		Storage::setFloat(*((float*)target), _storageStruct.currentZero);
		break;
	}
	case CONFIG_POWER_ZERO: {
		p_item = (uint8_t*)&_storageStruct.powerZero;
		Storage::setFloat(*((float*)target), _storageStruct.powerZero);
		break;
	}
	case CONFIG_POWER_ZERO_AVG_WINDOW:{
		p_item = (uint8_t*)&_storageStruct.powerZeroAvgWindow;
		Storage::setUint16(*((uint16_t*)target), _storageStruct.powerZeroAvgWindow);
		break;
	}
	case CONFIG_MESH_ACCESS_ADDRESS:{
		p_item = (uint8_t*)&_storageStruct.meshAccessAddress;
		Storage::setUint32(*((uint32_t*)target), _storageStruct.meshAccessAddress);
		break;
	}
	case CONFIG_PWM_PERIOD:{
		p_item = (uint8_t*)&_storageStruct.pwmInterval;
		Storage::setUint32(*((uint32_t*)target), _storageStruct.pwmInterval);
		break;
	}
	default: {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		return ERR_CONFIG_NOT_FOUND;
	}
	}

	if (persistent) {
		size = getSettingsItemSize(type);
		// minimum item size we can store is 4
		if (size < 4) {
			size = 4;
		}
		savePersistentStorageItem(p_item, size);
	}
	return ERR_SUCCESS;
}

//void Settings::initFlags() {
//	// set all flags to their default value
//	_storageStruct.flagsBit.meshDisabled = !MESHING;
//	_storageStruct.flagsBit.encryptionDisabled = !ENCRYPTION;
//	_storageStruct.flagsBit.iBeaconDisabled = !IBEACON;
//	_storageStruct.flagsBit.scannerDisabled = !INTERVAL_SCANNER_ENABLED;
//	_storageStruct.flagsBit.continuousPowerSamplerDisabled = !CONTINUOUS_POWER_SAMPLER;
//	_storageStruct.flagsBit.defaultOff = !DEFAULT_ON;
//	_storageStruct.flagsBit.flagsUninitialized = false;
//}

bool Settings::updateFlag(uint8_t type, bool value, bool persistent) {

	// should not happen, but better to check
//	if (_storageStruct.flagsBit.flagsUninitialized) {
//		// before updating a flag, we need to initialize all flags to their default
//		// value, otherwise they will be wrongly read after the update
//		initFlags();
//	}

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
		Storage::setUint8(value, _storageStruct.meshEnabled);
//		_storageStruct.flagsBit.meshDisabled = !value;
		break;
	}
	case CONFIG_ENCRYPTION_ENABLED : {
		p_item = (uint8_t*)&_storageStruct.encryptionEnabled;
		Storage::setUint8(value, _storageStruct.encryptionEnabled);
//		_storageStruct.flagsBit.encryptionDisabled = !value;
		break;
	}
	case CONFIG_IBEACON_ENABLED : {
		p_item = (uint8_t*)&_storageStruct.iBeaconEnabled;
		Storage::setUint8(value, _storageStruct.iBeaconEnabled);
//		_storageStruct.flagsBit.iBeaconDisabled = !value;
		break;
	}
	case CONFIG_SCANNER_ENABLED : {
		p_item = (uint8_t*)&_storageStruct.scannerEnabled;
		Storage::setUint8(value, _storageStruct.scannerEnabled);
//		_storageStruct.flagsBit.scannerDisabled = !value;
		break;
	}
	case CONFIG_CONT_POWER_SAMPLER_ENABLED : {
		p_item = (uint8_t*)&_storageStruct.continuousPowerSamplerEnabled;
		Storage::setUint8(value, _storageStruct.continuousPowerSamplerEnabled);
//		_storageStruct.flagsBit.continuousPowerSamplerDisabled = !value;
		break;
	}
	case CONFIG_DEFAULT_ON: {
		p_item = (uint8_t*)&_storageStruct.defaultOff;
		Storage::setUint8(value, _storageStruct.defaultOff);
//		_storageStruct.flagsBit.defaultOff = !value;
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

	return true;
}

bool Settings::readFlag(uint8_t type, bool& value) {

//	bool default_value;

	switch(type) {
	case CONFIG_MESH_ENABLED: {
		Storage::getUint8(_storageStruct.meshEnabled, (uint8_t*)&value, MESHING);
//		value = !_storageStruct.flagsBit.meshDisabled;
//		default_value = MESHING;
		break;
	}
	case CONFIG_ENCRYPTION_ENABLED: {
		Storage::getUint8(_storageStruct.encryptionEnabled, (uint8_t*)&value, ENCRYPTION);
//		value = !_storageStruct.flagsBit.encryptionDisabled;
//		default_value = ENCRYPTION;
		break;
	}
	case CONFIG_IBEACON_ENABLED: {
		Storage::getUint8(_storageStruct.iBeaconEnabled, (uint8_t*)&value, IBEACON);
//		value = !_storageStruct.flagsBit.iBeaconDisabled;
//		default_value = IBEACON;
		break;
	}
	case CONFIG_SCANNER_ENABLED: {
		Storage::getUint8(_storageStruct.scannerEnabled, (uint8_t*)&value, INTERVAL_SCANNER_ENABLED);
//		value = !_storageStruct.flagsBit.scannerDisabled;
//		default_value = INTERVAL_SCANNER_ENABLED;
		break;
	}
	case CONFIG_CONT_POWER_SAMPLER_ENABLED: {
		Storage::getUint8(_storageStruct.continuousPowerSamplerEnabled, (uint8_t*)&value, CONTINUOUS_POWER_SAMPLER);
//		value = !_storageStruct.flagsBit.continuousPowerSamplerDisabled;
//		default_value = CONTINUOUS_POWER_SAMPLER;
		break;
	}
	case CONFIG_DEFAULT_ON: {
		Storage::getUint8(_storageStruct.defaultOff, (uint8_t*)&value, DEFAULT_ON);
//		value = !_storageStruct.flagsBit.defaultOff;
//		default_value = DEFAULT_ON;
		break;
	}
	default:
		return false;
	}

//	if (_storageStruct.flagsBit.flagsUninitialized) {
//		value = default_value;
//		return true;
//	}

	return true;
}

bool Settings::isSet(uint8_t type) {

	switch(type) {
	case CONFIG_DEFAULT_ON:
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

void Settings::factoryReset(uint32_t resetCode) {
	if (resetCode != FACTORY_RESET_CODE) {
		LOGe("wrong reset code!");
		return;
	}

	_storage->clearStorage(PS_ID_CONFIGURATION);
	memset(&_storageStruct, 0xFF, sizeof(_storageStruct));
//	initFlags();
//	loadPersistentStorage();
}
