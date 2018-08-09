/** Store settings in RAM or persistent memory.
 *
 * This files serves as a little indirection between loading data from RAM or FLASH.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 22, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_Config.h>
#include <ble/cs_UUID.h>
#include <events/cs_EventDispatcher.h>
#include <protocol/cs_Defaults.h>
#include <util/cs_Utils.h>
#include <storage/cs_Settings.h>
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

	_initialized = true;
}

ERR_CODE Settings::writeToStorage(uint8_t type, uint8_t* data, size_t size, bool persistent) {

	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;
	error_code = verify(type, data, size);
	if (SUCCESS(error_code)) {
		error_code = set(type, data, size, persistent);
		EventDispatcher::getInstance().dispatch(type, data, size);
	}
	return error_code;
}

ERR_CODE Settings::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {

	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;
	if (type > sizeof(ConfigurationTypeSizes) || ConfigurationTypeSizes[type] == 0) {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		error_code = ERR_UNKNOWN_TYPE;
		return error_code;
	}
	
	// temporarily put data on stack(?)
	size_t plen = ConfigurationTypeSizes[type];
	uint8_t payload[plen];
	error_code = get(type, payload, plen);
	if (SUCCESS(error_code)) {
		streamBuffer->setPayload(payload, plen);
		streamBuffer->setType(type);
	}
	return error_code;
}

ERR_CODE Settings::verify(uint8_t type, uint8_t* payload, uint8_t length) {
	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;
	
	if (type > sizeof(ConfigurationTypeSizes)) {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		error_code = ERR_UNKNOWN_TYPE;
		return error_code;
	}

	switch(type) {
		case CONFIG_MESH_ENABLED :
		case CONFIG_ENCRYPTION_ENABLED :
		case CONFIG_IBEACON_ENABLED :
		case CONFIG_SCANNER_ENABLED :
		case CONFIG_CONT_POWER_SAMPLER_ENABLED :
		case CONFIG_PWM_ALLOWED:
		case CONFIG_SWITCH_LOCKED:
		case CONFIG_SWITCHCRAFT_ENABLED: {
			LOGe("Write disabled. Use commands to enable/disable");
			error_code = ERR_WRITE_NOT_ALLOWED;
			return error_code;
		}
	}

	uint8_t size = ConfigurationTypeSizes[type];
	if (size == 0) {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		error_code = ERR_UNKNOWN_TYPE;
		return error_code;
	}

	bool correct = false;
	if (size == length) {
		correct = true;
	} else if ((size > length) && (type == CONFIG_NAME)) {
		correct = true;
	}

	if (correct) {
		LOGi(FMT_SET_STR_TYPE_VAL, type, std::string((char*)payload, length).c_str());
		error_code = ERR_SUCCESS;
	} else {
		LOGw(FMT_ERR_EXPECTED, "size");
		error_code = ERR_WRONG_PAYLOAD_LENGTH;
	}
	return error_code;
}

size_t Settings::getSettingsItemSize(const uint8_t type) {
	if (type > sizeof(ConfigurationTypeSizes)) {
		return 0;
	}
	return ConfigurationTypeSizes[type];	
}

ERR_CODE Settings::get(uint8_t type, void* target, bool getDefaultValue) {
	size_t size = 0;
	return get(type, target, size, getDefaultValue);
}

ERR_CODE Settings::get(uint8_t type, void* target, size_t & size, bool getDefaultValue) {
	if (getDefaultValue || (!_storage->exists(Storage::FILE_CONFIGURATION, type))) {
		getDefaults(type, target, size);
		return ERR_SUCCESS;
	}
	Storage::file_data_t data;
	data.key = type;
	data.value = (uint8_t*)target;
	_storage->read(Storage::FILE_CONFIGURATION, data);
	size = data.size;
	return ERR_SUCCESS;
}

/**
 * For now always write all settings to persistent storage.
 */
ERR_CODE Settings::set(uint8_t type, void* target, size_t size, bool persistent) {
	Storage::file_data_t data;
	data.key = type;
	data.value = (uint8_t*)target;
	data.size = size;
	data.persistent = persistent;
	return _storage->write(Storage::FILE_CONFIGURATION, data);
}

bool Settings::updateFlag(uint8_t type, bool value, bool persistent) {
	uint8_t tmp = value;
	Storage::file_data_t data;
	data.key = type;
	data.value = &tmp;
	data.size = 1;
	data.persistent = persistent;
	return _storage->write(Storage::FILE_CONFIGURATION, data);
}

bool Settings::readFlag(uint8_t type, bool& value) {
	ERR_CODE error_code;
	
	uint8_t tmp = value;
	Storage::file_data_t data;
	data.key = type;
	data.value = &tmp;
	data.size = 1;
	
	error_code = _storage->read(Storage::FILE_CONFIGURATION, data);
	value = *data.value;
	return error_code;
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

void Settings::factoryReset(uint32_t resetCode) {
	if (resetCode != FACTORY_RESET_CODE) {
		LOGe("wrong reset code!");
		return;
	}

	// TODO:	_storage->clearStorage(PS_ID_CONFIGURATION);
	// TODO: memset(&_storageStruct, 0xFF, sizeof(_storageStruct));
}
