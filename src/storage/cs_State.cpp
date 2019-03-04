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
		uint8_t unsafe_type = to_underlying_type(type);
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
	size16_t size = TypeSize(type);
	return get(type, target, size, mode);
}

/** Get value from FLASH or RAM.
 *
 * The implementation casts type to the underlying type. The pointer to target is stored in data.value and the
 * size is written in data.size. 
 *
 * Here it assumed that the pointer *target is already allocated to the right size...
 */
cs_ret_code_t State::get(const CS_TYPE type, void* target, size16_t & size, const PersistenceMode mode) {
	cs_file_data_t data;
	data.type = type;
	data.size = size;
	data.value = (uint8_t*)target;
	ret_code_t ret_code = get(data, mode);
	// TODO: size check: stored size might differ from given size?
	LOGnone("Got type %s (%i) of size %i", TypeName(data.type), data.type, data.size);
	memcpy(target, data.value, data.size);
	size = data.size;
	return ret_code;
}

cs_ret_code_t State::get(cs_file_data_t & data, const PersistenceMode mode) {
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
				LOGnone("Loaded from RAM: %s", TypeName(data.type));
				return ERR_SUCCESS;
			}

			ret_code = _storage->read(FILE_CONFIGURATION, data);
			switch(ret_code) {
				case FDS_SUCCESS: {
					LOGnone("Loaded from FLASH: %s", TypeName(data.type));
					storeInRam(data);
					return ret_code;
				}
				case FDS_ERR_NOT_FOUND: {
					LOGnone("Loaded default: %s", TypeName(data.type));
					getDefault(data);
					storeInRam(data);
					return ERR_SUCCESS;
				}
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

/**
 * Store item in RAM. This will copy everything into a new struct. The original can be adjusted immediately after
 * you have called this function. Note that every record occupies RAM. If things have to persist, but do not need
 * to be stored in RAM, and when timing is not important, use set() with the FLASH mode argument.
 */
cs_ret_code_t State::storeInRam(const cs_file_data_t & data) {
	size16_t temp = 0;
	return storeInRam(data, temp);
}

/**
 * Maybe we should check if data is stored at right boundary.
 *
 * if ((uint32_t)data.value % 4u != 0) {
 *		LOGe("Unaligned type: %s: %p", TypeName(type), data.value);
 *	}
 */
cs_ret_code_t State::storeInRam(const cs_file_data_t & data, size16_t & index_in_ram) {
	// TODO: Check if enough RAM is available
	LOGd("storeInRam type=%u size=%u", to_underlying_type(data.type), data.size);
	bool exist = false;
	for (size16_t i = 0; i < _data_in_ram.size(); ++i) {
		if (_data_in_ram[i].type == data.type) {
			LOGnone("Update RAM");
			cs_file_data_t & ram_data = _data_in_ram[i];
			if (ram_data.size != data.size) { 
				free(ram_data.value);
				ram_data.value = (uint8_t*)malloc(sizeof(uint8_t) * data.size); // TODO: don't malloc when size is similar?
			}
			ram_data.size = data.size;
			memcpy(ram_data.value, data.value, data.size);
			exist = true;
			index_in_ram = i;
			break;
		}
	}
	if (!exist) {
		LOGnone("Store %i in RAM", data.type);
		cs_file_data_t &ram_data = *(cs_file_data_t*)malloc(sizeof(cs_file_data_t));
		ram_data.type = data.type;
		ram_data.size = data.size;
		LOGnone("Allocate value array of size %i", data.size);
		ram_data.value = (uint8_t*)malloc(sizeof(uint8_t) * data.size);
		memcpy(ram_data.value, data.value, data.size);
		_data_in_ram.push_back(ram_data);
		LOGd("RAM buffer now of size %i", _data_in_ram.size());
		index_in_ram = _data_in_ram.size() - 1;
	}
	return ERR_SUCCESS;
}

/**
 * Load from RAM. It is assumed that the caller does not know the data length. Hence, what is returned is a copy of
 * the data and allocation is done within loadFromRam.
 */
cs_ret_code_t State::loadFromRam(cs_file_data_t & data) {
	bool exist = false;
	for (size16_t i = 0; i < _data_in_ram.size(); ++i) {
		if (_data_in_ram[i].type == data.type) {
			cs_file_data_t & ram_data = _data_in_ram[i];
			data.size = ram_data.size;
			data.value = (uint8_t*)malloc(sizeof(uint8_t) * data.size);
			memcpy(data.value, ram_data.value, data.size);
			exist = true;
			break;
		}
	}
	return exist;
}

// TODO: deleteFromRam to limit RAM usage

/**
 * There are three modes:
 *   RAM: store item in volatile memory
 *   FLASH: store item in persistent memory
 *   STRATEGY: store item depending on its default location, keep a copy in RAM.
 */
cs_ret_code_t State::set(CS_TYPE type, void* target, size16_t size, const PersistenceMode mode) {
	cs_file_data_t data;
	data.type = type;
	data.value = (uint8_t*)target;
	LOGnone("Set value: %s: %i", TypeName(type), type);
	data.size = size;
	cs_ret_code_t ret_code = ERR_UNSPECIFIED;
	switch(mode) {
		case PersistenceMode::RAM: {
			return storeInRam(data);
		}
		case PersistenceMode::FLASH: {
			// TODO: By the time the data is written to flash, the data pointer might be invalid.
			return _storage->write(FILE_CONFIGURATION, data);
		}
		case PersistenceMode::STRATEGY1: {
			// first get if default location is RAM or FLASH
			switch(DefaultLocation(type)) {
				case PersistenceMode::RAM:
					return storeInRam(data);
				case PersistenceMode::FLASH:
					// fall-through
					break;
				default:
					LOGe("PM not implemented");
					return ERR_NOT_IMPLEMENTED;
			}
			// we first store the data in RAM
			size16_t index = 0;
			ret_code = storeInRam(data, index);
			LOGnone("Item stored in RAM: %i", index);
			if (ret_code != ERR_SUCCESS) {
				LOGw("Failure to store in RAM, will still try to store in FLASH");
			}
			// now we have a duplicate of our data we can safely store it to FLASH asynchronously
			if (index >= _data_in_ram.size()) {
				LOGe("RAM corrupted");
				ret_code = ERR_WRITE_NOT_ALLOWED;
				break;
			}
			cs_file_data_t ram_data = _data_in_ram[index];		
			LOGd("Storage write type=%u size=%u data=[0x%X,...]", ram_data.type, ram_data.size, ram_data.value[0]);
			return _storage->write(FILE_CONFIGURATION, ram_data);
		}
		case PersistenceMode::FIRMWARE_DEFAULT: {
			LOGe("Default cannot be written");
			ret_code = ERR_WRITE_NOT_ALLOWED;
			break;
		}
	}
	return ret_code;
}

cs_ret_code_t State::remove(CS_TYPE type, const PersistenceMode mode) {
	cs_ret_code_t ret_code = ERR_UNSPECIFIED;
	switch(mode) {
		case PersistenceMode::FLASH: {
			return _storage->remove(FILE_CONFIGURATION, type);
		}
		default:
			LOGe("Not implemented");
			return ERR_NOT_IMPLEMENTED;
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
