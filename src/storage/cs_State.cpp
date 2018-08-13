/** Store settings in RAM or persistent memory.
 *
 * This files serves as a little indirection between loading data from RAM or FLASH.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 22, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <algorithm>
#include <ble/cs_UUID.h>
#include <cfg/cs_Config.h>
#include <events/cs_EventDispatcher.h>
#include <protocol/cs_Defaults.h>
#include <storage/cs_State.h>
#include <storage/cs_StorageHelper.h>
#include <util/cs_Utils.h>

State::State() : _initialized(false), _storage(NULL), _boardsConfig(NULL) {
};

void State::init(boards_config_t* boardsConfig) {
	_storage = &Storage::getInstance();
	_boardsConfig = boardsConfig;

	if (!_storage->isInitialized()) {
		LOGe(FMT_NOT_INITIALIZED, "Storage");
		return;
	}

	_initialized = true;
}

ERR_CODE State::writeToStorage(uint8_t type, uint8_t* data, size_t size, bool persistent) {

	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;
	error_code = verify(type, data, size);
	if (SUCCESS(error_code)) {
		LOGi("Write to storage");
		error_code = set(type, data, size, persistent);
		EventDispatcher::getInstance().dispatch(type, data, size);
	}
	return error_code;
}

ERR_CODE State::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {

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

ERR_CODE State::verify(uint8_t type, uint8_t* payload, uint8_t size) {
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

	uint8_t check_size = ConfigurationTypeSizes[type];
	if (check_size == 0) {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		error_code = ERR_UNKNOWN_TYPE;
		return error_code;
	}

	bool correct = false;
	if (check_size == size) {
		correct = true;
	} else if ((check_size > size) && (type == CONFIG_NAME)) {
		correct = true;
	}

	if (correct) {
		if (size == 1) {
			LOGi(FMT_SET_INT_TYPE_VAL, type, (int)payload[0]);
		} else {
			LOGi(FMT_SET_STR_TYPE_VAL, type, std::string((char*)payload, size).c_str());
		}
		error_code = ERR_SUCCESS;
	} else {
		LOGw(FMT_ERR_EXPECTED, "check_size");
		error_code = ERR_WRONG_PAYLOAD_LENGTH;
	}
	return error_code;
}

size_t State::getStateItemSize(const uint8_t type) {
	if (type > sizeof(ConfigurationTypeSizes)) {
		return 0;
	}
	return ConfigurationTypeSizes[type];	
}

ERR_CODE State::get(uint8_t type, void* target, bool getDefaultValue) {
	size_t size = 0;
	return get(type, target, size, getDefaultValue);
}

ERR_CODE State::get(uint8_t type, void* target, size_t & size, bool getDefaultValue) {
	if (getDefaultValue || (!_storage->exists(FILE_CONFIGURATION, type))) {
		LOGd("Get default %i", type);
		getDefaults(type, target, size);
		return ERR_SUCCESS;
	}
	st_file_data_t data;
	data.key = type;
	data.value = (uint8_t*)target;
	LOGd("Get type %i", type);
	_storage->read(FILE_CONFIGURATION, data);
	size = data.size;
	return ERR_SUCCESS;
}

/**
 * For now always write all settings to persistent storage.
 */
ERR_CODE State::set(uint8_t type, void* target, size_t size, bool persistent) {
	st_file_data_t data;
	data.key = type;
	data.value = (uint8_t*)target;
	data.size = size;
	data.persistent = persistent;
	return _storage->write(FILE_CONFIGURATION, data);
}

bool State::updateFlag(uint8_t type, bool value, bool persistent) {
	uint8_t tmp = value;
	st_file_data_t data;
	data.key = type;
	data.value = &tmp;
	data.size = 1;
	data.persistent = persistent;
	return _storage->write(FILE_CONFIGURATION, data);
}

bool State::readFlag(uint8_t type, bool& value) {
	ERR_CODE error_code;
	
	uint8_t tmp = value;
	st_file_data_t data;
	data.key = type;
	data.value = &tmp;
	data.size = 1;
	
	error_code = _storage->read(FILE_CONFIGURATION, data);
	value = *data.value;
	return error_code;
}

bool State::isSet(uint8_t type) {
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

bool State::isNotifying(uint8_t type) {
	std::vector<uint8_t>::iterator it = find(_notifyingStates.begin(), _notifyingStates.end(), type);
	return it != _notifyingStates.end();
}

void State::setNotify(uint8_t type, bool enable) {
	if (enable) {
		if (!isNotifying(type)) {
			_notifyingStates.push_back(type);
			_notifyingStates.shrink_to_fit();
		}
	} else {
		std::vector<uint8_t>::iterator it = find(_notifyingStates.begin(), _notifyingStates.end(), type);
		if (it != _notifyingStates.end()) {
			_notifyingStates.erase(it);
			_notifyingStates.shrink_to_fit();
		}
	}
}

void State::disableNotifications() {
	_notifyingStates.clear();
}

void State::factoryReset(uint32_t resetCode) {
	if (resetCode != FACTORY_RESET_CODE) {
		LOGe("wrong reset code!");
		return;
	}

	// TODO:	_storage->clearStorage(PS_ID_CONFIGURATION);
	// TODO: memset(&_storageStruct, 0xFF, sizeof(_storageStruct));
}
