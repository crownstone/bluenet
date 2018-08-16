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
#include <events/cs_EventTypes.h>
#include <protocol/cs_Defaults.h>
#include <storage/cs_State.h>
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

ERR_CODE State::writeToStorage(CS_TYPE type, uint8_t* data, size16_t size, PersistenceMode mode) {

	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;
	error_code = verify(type, data, size);
	if (SUCCESS(error_code)) {
		LOGi("Write to storage");
		error_code = set(type, data, size, mode);
		event_t event(type, data, size);
		EventDispatcher::getInstance().dispatch(event);
	}
	return error_code;
}

ERR_CODE State::readFromStorage(CS_TYPE type, StreamBuffer<uint8_t>* streamBuffer) {

	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;
	if (TypeSize(type) == 0) {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		error_code = ERR_UNKNOWN_TYPE;
		return error_code;
	}
	
	// temporarily put data on stack(?)
	size16_t plen = TypeSize(type);
	uint8_t payload[plen];
	error_code = get(type, payload, plen, PersistenceMode::FLASH);
	if (SUCCESS(error_code)) {
		streamBuffer->setPayload(payload, plen);
		uint8_t unsafe_type = +type;
		streamBuffer->setType(unsafe_type);
	}
	return error_code;
}

ERR_CODE State::verify(CS_TYPE type, uint8_t* payload, uint8_t size) {
	ERR_CODE error_code = ERR_NOT_IMPLEMENTED;

	size16_t check_size = TypeSize(type);
	if (check_size == 0) {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, type);
		error_code = ERR_UNKNOWN_TYPE;
		return error_code;
	}

	switch(type) {
		case CS_TYPE::CONFIG_MESH_ENABLED :
		case CS_TYPE::CONFIG_ENCRYPTION_ENABLED :
		case CS_TYPE::CONFIG_IBEACON_ENABLED :
		case CS_TYPE::CONFIG_SCANNER_ENABLED :
		case CS_TYPE::CONFIG_CONT_POWER_SAMPLER_ENABLED :
		case CS_TYPE::CONFIG_PWM_ALLOWED:
		case CS_TYPE::CONFIG_SWITCH_LOCKED:
		case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED: {
			LOGe("Write disabled. Use commands to enable/disable");
			error_code = ERR_WRITE_NOT_ALLOWED;
			return error_code;
		}
		default:
			// go on
			;
	}

	bool correct = false;
	if (check_size == size) {
		correct = true;
	} 
	// handle CONFIG_NAME separately, check_size can be size (which is just the maximum size)
	if ((check_size > size) && (type == CS_TYPE::CONFIG_NAME)) {
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

//size16_t State::getStateItemSize(const CS_TYPE type) {
//	return TypeSize(type);
//}

ERR_CODE State::get(const CS_TYPE type, void* target, const PersistenceMode mode) {
	size16_t size = 0;
	return get(type, target, size, mode);
}

ERR_CODE State::get(const CS_TYPE type, void* target, size16_t & size, const PersistenceMode mode) {
	ret_code_t ret_code = FDS_ERR_NOT_FOUND;
	
	PersistenceMode p_mode = mode;

	if (p_mode == PersistenceMode::DEFAULT_PERSISTENCE) {
			p_mode = DefaultPersistence(type);
	}
	switch(p_mode) {
		case PersistenceMode::FIRMWARE_DEFAULT:
			break;
		case PersistenceMode::RAM: {
			bool exist = false;
			//exist = loadFromRam(type, target, size);
			if (!exist) ret_code = FDS_ERR_NOT_FOUND;
			break;
		}
		case PersistenceMode::FLASH:
			st_file_data_t data;
			data.key = +type;
			data.value = (uint8_t*)target;
			ret_code =_storage->read(FILE_CONFIGURATION, data);
			size = data.size;
			break;
		default:
			LOGw("Unknown persistence p_mode");
	}

	if (ret_code == FDS_ERR_NOT_FOUND) {
		getDefaults(type, target, size);
		ret_code = ERR_SUCCESS;
	}
	return ret_code;
}

ERR_CODE State::storeInRam(const st_file_data_t & data) {
	// TODO: Check if enough RAM is available
	bool exist = false;
	for (size16_t i = 0; i < _not_persistent.size(); ++i) {
		if (_not_persistent[i].key == data.key) {
			_not_persistent[i].key = data.key;
			_not_persistent[i].value = data.value;
			_not_persistent[i].size = data.size;
			_not_persistent[i].persistent = data.persistent;
			exist = true;
			break;
		}
	}
	if (!exist) {
		_not_persistent.push_back(data);
	}
	return ERR_SUCCESS;
}

ERR_CODE State::loadFromRam(st_file_data_t & data) {
	bool exist = false;
	for (size16_t i = 0; i < _not_persistent.size(); ++i) {
		if (_not_persistent[i].key == data.key) {
			data.key = _not_persistent[i].key;
			data.value = _not_persistent[i].value;
			data.size = _not_persistent[i].size;
			data.persistent = _not_persistent[i].persistent;
			exist = true;
			break;
		}
	}
	return exist;
}

/**
 * For now always write all settings to persistent storage.
 */
ERR_CODE State::set(CS_TYPE type, void* target, size16_t size, const PersistenceMode mode) {
	st_file_data_t data;
	data.key = +type;
	data.value = (uint8_t*)target;
	data.size = size;
	//data.persistent = persistent;
	//if (!data.persistent) {
	//	return storeInRam(data);
	//}
	return _storage->write(FILE_CONFIGURATION, data);
}

bool State::updateFlag(CS_TYPE type, bool value, const PersistenceMode mode) {
	uint8_t tmp = value;
	st_file_data_t data;
	data.key = +type;
	data.value = &tmp;
	data.size = 1;
	//data.persistent = persistent;
	return _storage->write(FILE_CONFIGURATION, data);
}

bool State::readFlag(CS_TYPE type, bool& value) {
	ERR_CODE error_code;
	
	uint8_t tmp = value;
	st_file_data_t data;
	data.key = +type;
	data.value = &tmp;
	data.size = 1;
	
	error_code = _storage->read(FILE_CONFIGURATION, data);
	value = *data.value;
	return error_code;
}

bool State::isSet(CS_TYPE type) {
	bool enabled = false;
	switch(type) {
		case CS_TYPE::CONFIG_MESH_ENABLED:
		case CS_TYPE::CONFIG_ENCRYPTION_ENABLED:
		case CS_TYPE::CONFIG_IBEACON_ENABLED:
		case CS_TYPE::CONFIG_SCANNER_ENABLED:
		case CS_TYPE::CONFIG_CONT_POWER_SAMPLER_ENABLED:
		case CS_TYPE::CONFIG_DEFAULT_ON:
		case CS_TYPE::CONFIG_PWM_ALLOWED:
		case CS_TYPE::CONFIG_SWITCH_LOCKED:
		case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED: 
			readFlag(type, enabled);
		default: {}
	}
	return enabled;
}

bool State::isNotifying(CS_TYPE type) {
	std::vector<CS_TYPE>::iterator it = find(_notifyingStates.begin(), _notifyingStates.end(), type);
	return it != _notifyingStates.end();
}

void State::setNotify(CS_TYPE type, bool enable) {
	if (enable) {
		if (!isNotifying(type)) {
			_notifyingStates.push_back(type);
			_notifyingStates.shrink_to_fit();
		}
	} else {
		std::vector<CS_TYPE>::iterator it = find(_notifyingStates.begin(), _notifyingStates.end(), type);
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
