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

cs_ret_code_t State::verifySizeForGet(const cs_state_data_t & data) {
	size16_t typeSize = TypeSize(data.type);
	if (typeSize == 0) {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, data.type);
		return ERR_UNKNOWN_TYPE;
	}
	if (data.size < typeSize) {
		return ERR_BUFFER_TOO_SMALL;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t State::verifySizeForSet(const cs_state_data_t & data) {
	size16_t typeSize = TypeSize(data.type);
	if (typeSize == 0) {
		LOGw(FMT_CONFIGURATION_NOT_FOUND, data.type);
		return ERR_UNKNOWN_TYPE;
	}
	switch (data.type) {
	case CS_TYPE::CONFIG_NAME: {
		// Case where type size is the max size, but can be smaller.
		if (data.size < 1 || data.size > typeSize) {
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		break;
	}
	default: {
		// Case with fixed type size.
		if (data.size != typeSize) {
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		break;
	}
	}
	return ERR_SUCCESS;
}

cs_ret_code_t State::get(cs_state_data_t & data, const PersistenceMode mode) {
	ret_code_t ret_code = ERR_NOT_FOUND;
	if (data.size < TypeSize(data.type)) { // TODO: make this an assert?
		return ERR_BUFFER_TOO_SMALL;
	}

	switch(mode) {
		case PersistenceMode::FIRMWARE_DEFAULT:
			return getDefault(data);
		case PersistenceMode::RAM:
			return loadFromRam(data);
		case PersistenceMode::FLASH:
			return _storage->read(FILE_CONFIGURATION, data);
		case PersistenceMode::STRATEGY1: {
			// First check if it's already in ram.
			bool exists = loadFromRam(data);
			if (exists) {
				LOGnone("Loaded from RAM: %s", TypeName(data.type));
				return ERR_SUCCESS;
			}

			// See if we need to check flash.
			if (DefaultLocation(data.type) == PersistenceMode::RAM) {
				LOGnone("Loaded default: %s", TypeName(data.type));
				ret_code = getDefault(data);
				storeInRam(data);
				return ret_code;
			}

			// Check flash, else use default.
			ret_code = _storage->read(FILE_CONFIGURATION, data);
			switch(ret_code) {
				case ERR_SUCCESS: {
					LOGnone("Loaded from FLASH: %s", TypeName(data.type));
					storeInRam(data);
					return ret_code;
				}
				case ERR_NOT_FOUND: {
					LOGnone("Loaded default: %s", TypeName(data.type));
					ret_code = getDefault(data);
					storeInRam(data);
					return ret_code;
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

cs_ret_code_t State::storeInRam(const cs_state_data_t & data) {
	size16_t temp = 0;
	return storeInRam(data, temp);
}

/**
 * Store size of state variable, not of allocated size.
 */
cs_ret_code_t State::storeInRam(const cs_state_data_t & data, size16_t & index_in_ram) {
	// TODO: Check if enough RAM is available
	LOGd("storeInRam type=%u size=%u", to_underlying_type(data.type), data.size);
	bool exist = false;
	for (size16_t i = 0; i < _data_in_ram.size(); ++i) {
		if (_data_in_ram[i].type == data.type) {
			LOGnone("Update RAM");
			cs_state_data_t & ram_data = _data_in_ram[i];
			if (ram_data.size != data.size) {
				free(ram_data.value);
				ram_data.size = data.size;
				allocate(ram_data);
			}
			memcpy(ram_data.value, data.value, data.size);
			exist = true;
			index_in_ram = i;
			break;
		}
	}
	if (!exist) {
		LOGnone("Store in RAM type=%u", data.type);
		cs_state_data_t &ram_data = *(cs_state_data_t*)malloc(sizeof(cs_state_data_t));
		ram_data.type = data.type;
		ram_data.size = data.size;
		LOGnone("Allocate value array of size %u", data.size);
		allocate(ram_data);
		memcpy(ram_data.value, data.value, data.size);
		_data_in_ram.push_back(ram_data);
		LOGd("RAM buffer now of size %i", _data_in_ram.size());
		index_in_ram = _data_in_ram.size() - 1;
	}
	return ERR_SUCCESS;
}

/**
 * If memory is used for flash, then let storage do the allocation.
 */
cs_ret_code_t State::allocate(cs_state_data_t & data) {
	if (DefaultLocation(data.type) == PersistenceMode::FLASH) {
		size16_t tempSize = data.size;
		data.value = _storage->allocate(tempSize);
	}
	else {
		data.value = (uint8_t*)malloc(sizeof(uint8_t) * data.size);
	}
	return ERR_SUCCESS;
}

/**
 * Copies from ram to target buffer.
 *
 * Does not check if target buffer has a large enough size.
 */
cs_ret_code_t State::loadFromRam(cs_state_data_t & data) {
	for (size16_t i = 0; i < _data_in_ram.size(); ++i) {
		if (_data_in_ram[i].type == data.type) {
			cs_state_data_t & ram_data = _data_in_ram[i];
			data.size = ram_data.size;
			memcpy(data.value, ram_data.value, data.size);
			return ERR_SUCCESS;
		}
	}
	return ERR_NOT_FOUND;
}

// TODO: deleteFromRam to limit RAM usage

/**
 * There are three modes:
 *   RAM: store item in volatile memory
 *   FLASH: store item in persistent memory
 *   STRATEGY: store item depending on its default location, keep a copy in RAM.
 */
cs_ret_code_t State::set(const cs_state_data_t & data, const PersistenceMode mode) {
	LOGnone("Set value: %s: %i", TypeName(type), type);
	cs_ret_code_t ret_code = ERR_UNSPECIFIED;
	switch(mode) {
		case PersistenceMode::RAM: {
			return storeInRam(data);
		}
		case PersistenceMode::FLASH: {
			return ERR_NOT_AVAILABLE;
			// By the time the data is written to flash, the data pointer might be invalid.
			// There is also no guarantee that the data pointer is aligned.
			//return _storage->write(FILE_CONFIGURATION, data);
		}
		case PersistenceMode::STRATEGY1: {
			// first get if default location is RAM or FLASH
			switch(DefaultLocation(data.type)) {
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
			cs_state_data_t ram_data = _data_in_ram[index];
			LOGd("Storage write type=%u size=%u data=%p [0x%X,...]", ram_data.type, ram_data.size, ram_data.value, ram_data.value[0]);
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

cs_ret_code_t State::get(const CS_TYPE type, void *value, const size16_t size) {
	cs_state_data_t data(type, (uint8_t*)value, size);
	return get(data);
}

cs_ret_code_t State::set(const CS_TYPE type, void *value, const size16_t size) {
	cs_state_data_t data(type, (uint8_t*)value, size);
	return set(data);
}

bool State::isTrue(CS_TYPE type, const PersistenceMode mode) {
	TYPIFY(CONFIG_MESH_ENABLED) enabled = false;
	switch(type) {
		case CS_TYPE::CONFIG_MESH_ENABLED:
		case CS_TYPE::CONFIG_ENCRYPTION_ENABLED:
		case CS_TYPE::CONFIG_IBEACON_ENABLED:
		case CS_TYPE::CONFIG_SCANNER_ENABLED:
		case CS_TYPE::CONFIG_DEFAULT_ON:
		case CS_TYPE::CONFIG_PWM_ALLOWED:
		case CS_TYPE::CONFIG_SWITCH_LOCKED:
		case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED: {
			cs_state_data_t data(type, &enabled, sizeof(enabled));
			get(data);
			break;
		}
		default: {}
	}
	return enabled;
}

void State::factoryReset(uint32_t resetCode) {
	if (resetCode != FACTORY_RESET_CODE) {
		LOGe("Wrong reset code!");
		return;
	}
	LOGw("Perform factory reset!");
	_storage->remove(FILE_CONFIGURATION);
}
