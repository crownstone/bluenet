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
#include <drivers/cs_Serial.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <util/cs_Utils.h>

State::State() : _storage(NULL), _boardsConfig(NULL) {
};

void State::init(boards_config_t* boardsConfig) {
	LOGi(FMT_INIT, "board config");
	_storage = &Storage::getInstance();
	_boardsConfig = boardsConfig;

	if (!_storage->isInitialized()) {
		LOGe(FMT_NOT_INITIALIZED, "Storage");
		return;
	}

	setInitialized();
}

cs_ret_code_t State::writeToStorage(CS_TYPE type, uint8_t* data, size16_t size, PersistenceMode mode) {

	cs_ret_code_t error_code = ERR_NOT_IMPLEMENTED;
	error_code = verify(type, data, size);
	if (SUCCESS(error_code)) {
		LOGi("Write to storage");
		error_code = set(type, data, size, mode);
		event_t event(type, data, size);
		EventDispatcher::getInstance().dispatch(event);
	}
	return error_code;
}

cs_ret_code_t State::readFromStorage(CS_TYPE type, StreamBuffer<uint8_t>* streamBuffer) {

	cs_ret_code_t error_code = ERR_NOT_IMPLEMENTED;
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

cs_ret_code_t State::verify(CS_TYPE type, uint8_t* payload, uint8_t size) {

	cs_ret_code_t error_code = ERR_NOT_IMPLEMENTED;

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

cs_ret_code_t State::get(const CS_TYPE type, void* target, const PersistenceMode mode) {
	size16_t size = 0;
	return get(type, target, size, mode);
}

/** Get value from FLASH or RAM.
 *
 * The implementation casts type to the underlying type. The pointer to target is stored in data.value and the
 * size is 
 */
cs_ret_code_t State::get(const CS_TYPE type, void* target, size16_t & size, const PersistenceMode mode) {
	st_file_data_t data;
	data.type = type;
	data.value = (uint8_t*)target;
	data.size = size;
	ret_code_t ret_code = get(data, mode);
	size = data.size;
	return ret_code;
}

cs_ret_code_t State::get(st_file_data_t & data, const PersistenceMode mode) {
	ret_code_t ret_code = FDS_ERR_NOT_FOUND;
	switch(mode) {
		case PersistenceMode::FIRMWARE_DEFAULT:
			getDefault(data);
			break;
		case PersistenceMode::RAM: {
			bool exist = loadFromRam(data);
			return exist ? ERR_SUCCESS : ERR_NOT_FOUND;
		}
		case PersistenceMode::FLASH:
			return _storage->read(FILE_CONFIGURATION, data);
		case PersistenceMode::STRATEGY1: {
			switch(DefaultLocation(data.type)) {
				case PersistenceMode::RAM: {
					bool exist = loadFromRam(data);
					return exist ? ERR_SUCCESS : ERR_NOT_FOUND;
				}
				case PersistenceMode::FLASH:
					break;
				default:
					LOGe("PM not implemented");
					return ERR_NOT_IMPLEMENTED;
			}
			bool exist = loadFromRam(data);
			if (exist) {
				return ERR_SUCCESS;
			}

			ret_code = _storage->read(FILE_CONFIGURATION, data);
			switch(ret_code) {
				case FDS_SUCCESS:
					LOGnone("Move from FLASH to RAM");
					storeInRam(data);
					return ret_code;
				case FDS_ERR_NOT_FOUND:
					getDefault(data);
					return ERR_SUCCESS;
				default: {
					LOGe("Should not happen");
				}
			}

			break;
		}
		default:
			LOGw("Unknown persistence p_mode");
	}
	return ret_code;
}

cs_ret_code_t State::storeInRam(const st_file_data_t & data) {
	// TODO: Check if enough RAM is available
	bool exist = false;
	for (size16_t i = 0; i < _not_persistent.size(); ++i) {
		if (_not_persistent[i].type == data.type) {
			_not_persistent[i].value = data.value;
			_not_persistent[i].size = data.size;
			exist = true;
			break;
		}
	}
	if (!exist) {
		_not_persistent.push_back(data);
	}
	return ERR_SUCCESS;
}

cs_ret_code_t State::loadFromRam(st_file_data_t & data) {
	bool exist = false;
	for (size16_t i = 0; i < _not_persistent.size(); ++i) {
		if (_not_persistent[i].type == data.type) {
			data.value = _not_persistent[i].value;
			data.size = _not_persistent[i].size;
			exist = true;
			break;
		}
	}
	return exist;
}

/**
 * For now always write all settings to persistent storage. 
 */
cs_ret_code_t State::set(CS_TYPE type, void* target, size16_t size, const PersistenceMode mode) {
	st_file_data_t data;
	data.type = type;
	data.value = (uint8_t*)target;
	if ((uint32_t)data.value % 4u != 0) {
		LOGe("Unaligned type: %s: %p", TypeName(type), data.value);
	}
	data.size = size;
	cs_ret_code_t ret_code = ERR_UNSPECIFIED;
	switch(mode) {
		case PersistenceMode::RAM: {
			return storeInRam(data);
		}
		case PersistenceMode::FLASH: {
			return _storage->write(FILE_CONFIGURATION, data);
		}
		case PersistenceMode::STRATEGY1: {
			// first get if default location is RAM or FLASH
			switch(DefaultLocation(type)) {
				case PersistenceMode::RAM:
					return storeInRam(data);
				case PersistenceMode::FLASH:
					break;
				default:
					LOGe("PM not implemented");
					return ERR_NOT_IMPLEMENTED;
			}
			// the following is general, but it is not a fast implementation
			// it is better to directly compare with the default without copying to a local variable
			st_file_data_t data_compare;
			data_compare.type = type;
			data_compare.value = (uint8_t*)malloc(size);
			data_compare.size = size;
			ret_code = get(data_compare, PersistenceMode::FIRMWARE_DEFAULT);
			if (data == data_compare) {
				LOGd("Same value as default");
				// TODO: No, we cannot assume it is fine. We might reset FLASH to firmware default
				// in that case we should remove the written value or at least write it as well to FLASH
//				return ERR_SUCCESS; // do nothing	
			}
			// we cannot assume that the value is already in RAM, it might be the first time it is accessed
			ret_code = get(data_compare, PersistenceMode::FLASH);
			if ((ret_code == ERR_SUCCESS) && (data == data_compare)) {
				LOGd("Same value as flash");
				return ERR_SUCCESS; // do nothing
			}
			// either value is not present in FLASH, or it is different, update it!	
			ret_code = storeInRam(data);
			if (ret_code != ERR_SUCCESS) {
				LOGw("Failure to store in RAM, will still try to store in FLASH");
			}
			LOGd("Write 0x%x", data.value[0]);
			return _storage->write(FILE_CONFIGURATION, data);
		}
		case PersistenceMode::FIRMWARE_DEFAULT: {
			LOGe("Default cannot be written");
			ret_code = ERR_WRITE_NOT_ALLOWED;
			break;
		}
	}
	return ret_code;
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
		case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED: {
			size16_t size = sizeof(enabled);
			get(type, (void*)&enabled, size, PersistenceMode::STRATEGY1);
		}
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
		LOGe("Wrong reset code!");
		return;
	}
	LOGw("Perform factory reset!");

	_storage->remove(FILE_CONFIGURATION);
}
