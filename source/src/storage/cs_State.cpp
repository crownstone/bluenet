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
#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <util/cs_Utils.h>

#if TICK_INTERVAL_MS > STATE_RETRY_STORE_DELAY_MS
#error "TICK_INTERVAL_MS must not be larger than STATE_RETRY_STORE_DELAY_MS"
#endif

// Define as LOGd to get debug logs.
#define LOGStateDebug LOGnone

void storageErrorCallback(cs_storage_operation_t operation, cs_file_id_t fileId, CS_TYPE type) {
	State::getInstance().handleStorageError(operation, fileId, type);
}

State::State() : _storage(NULL), _boardsConfig(NULL) {
}

void State::init(boards_config_t* boardsConfig) {
	LOGi(FMT_INIT, "board config");
	_storage = &Storage::getInstance();
	_boardsConfig = boardsConfig;

	if (!_storage->isInitialized()) {
		LOGe(FMT_NOT_INITIALIZED, "Storage");
		return;
	}
	_storage->setErrorCallback(storageErrorCallback);
	EventDispatcher::getInstance().addListener(this);
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

cs_ret_code_t State::findInRam(const CS_TYPE & type, size16_t & index_in_ram) {
	for (size16_t i = 0; i < _ram_data_register.size(); ++i) {
		if (_ram_data_register[i].type == type) {
			index_in_ram = i;
			return ERR_SUCCESS;
		}
	}
	return ERR_NOT_FOUND;
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
	LOGStateDebug("storeInRam type=%u size=%u", to_underlying_type(data.type), data.size);
	cs_ret_code_t ret_code = findInRam(data.type, index_in_ram);
	if (ret_code == ERR_SUCCESS) {
		LOGStateDebug("Update RAM");
		cs_state_data_t & ram_data = _ram_data_register[index_in_ram];
		if (ram_data.size != data.size) {
			LOGe("Should not happen: ram_data.size=%u data.size=%u", ram_data.size, data.size);
			free(ram_data.value);
			ram_data.size = data.size;
			allocate(ram_data);
		}
		memcpy(ram_data.value, data.value, data.size);
	}
	else {
		LOGStateDebug("Store in RAM type=%u", data.type);
		cs_state_data_t & ram_data = addToRam(data.type, data.size);
		memcpy(ram_data.value, data.value, data.size);
		index_in_ram = _ram_data_register.size() - 1;
	}
	return ERR_SUCCESS;
}

cs_state_data_t & State::addToRam(const CS_TYPE & type, size16_t size) {
	cs_state_data_t & data = *(cs_state_data_t*)malloc(sizeof(cs_state_data_t));
	data.type = type;
	data.size = size;
	allocate(data);
	_ram_data_register.push_back(data);
	LOGStateDebug("Added type=%u size=%u val=%p", data.type, data.size, data.value);
	LOGStateDebug("RAM index now of size %i", _ram_data_register.size());
	return data;
}

cs_ret_code_t State::removeFromRam(const CS_TYPE & type) {
	LOGStateDebug("removeFromRam type=%u", to_underlying_type(type));
	size16_t index_in_ram;
	cs_ret_code_t ret_code = findInRam(type, index_in_ram);
	if (ret_code == ERR_SUCCESS) {
		LOGStateDebug("Remove from RAM");
		cs_state_data_t* ram_data = &(_ram_data_register[index_in_ram]);
		free(ram_data->value);
		_ram_data_register.erase(_ram_data_register.begin() + index_in_ram);
		free(ram_data);
	}
	return ERR_SUCCESS;
}

/**
 * Let storage do the allocation, so that it's of the correct size and alignment.
 */
cs_ret_code_t State::allocate(cs_state_data_t & data) {
	LOGStateDebug("Allocate value array of size %u", data.size);
	size16_t tempSize = data.size;
	data.value = _storage->allocate(tempSize);
	LOGStateDebug("Actually allocated %u", tempSize);
	return ERR_SUCCESS;
}

/**
 * Copies from ram to target buffer.
 *
 * Does not check if target buffer has a large enough size.
 */
cs_ret_code_t State::loadFromRam(cs_state_data_t & data) {
	size16_t index_in_ram;
	cs_ret_code_t ret_code = findInRam(data.type, index_in_ram);
	if (ret_code != ERR_SUCCESS) {
		return ret_code;
	}
	cs_state_data_t & ram_data = _ram_data_register[index_in_ram];
	data.size = ram_data.size;
	memcpy(data.value, ram_data.value, data.size);
	return ERR_SUCCESS;
}

/**
 * Since this function is called while iterating over the queue, don't automatically add things to queue here.
 * Instead, just return ERR_BUSY, and let the called put it in queue..
 */
cs_ret_code_t State::storeInFlash(size16_t & index_in_ram) {
	if (!_startedWritingToFlash) {
		return ERR_BUSY;
	}
	if (index_in_ram >= _ram_data_register.size()) {
		LOGe("Invalid index");
		return ERR_WRITE_NOT_ALLOWED;
	}
	cs_state_data_t ram_data = _ram_data_register[index_in_ram];
	LOGStateDebug("Storage write type=%u size=%u data=%p [0x%X,...]", ram_data.type, ram_data.size, ram_data.value, ram_data.value[0]);
	cs_ret_code_t ret_code = _storage->write(getFileId(ram_data.type), ram_data);
	switch (ret_code) {
	case ERR_BUSY: {
		return ERR_BUSY;
	}
	case ERR_NO_SPACE: {
		// TODO: remove things from flash..
		return ERR_NO_SPACE;
	}
	default:
		return ret_code;
	}
}

cs_ret_code_t State::removeFromFlash(const CS_TYPE & type) {
	if (!_startedWritingToFlash) {
		return ERR_BUSY;
	}
	cs_ret_code_t ret_code = _storage->remove(getFileId(type), type);
	switch(ret_code) {
	case ERR_SUCCESS:
	case ERR_NOT_FOUND:
		return ERR_SUCCESS;
	default:
		return ret_code;
	}
}

cs_ret_code_t State::get(cs_state_data_t & data, const PersistenceMode mode) {
	ret_code_t ret_code = ERR_NOT_FOUND;
	CS_TYPE type = data.type;
	size16_t typeSize = TypeSize(type);
	if (typeSize == 0) {
		return ERR_UNKNOWN_TYPE;
	}
	if (data.size < typeSize) {
		return ERR_BUFFER_TOO_SMALL;
	}

	switch(mode) {
		case PersistenceMode::FIRMWARE_DEFAULT:
			return getDefaultValue(data);
		case PersistenceMode::RAM:
			return loadFromRam(data);
		case PersistenceMode::FLASH:
			return _storage->read(getFileId(type), data);
		case PersistenceMode::STRATEGY1: {
			// First check if it's already in ram.
			ret_code = loadFromRam(data);
			if (ret_code == ERR_SUCCESS) {
				LOGStateDebug("Loaded from RAM: %s", TypeName(data.type));
				return ERR_SUCCESS;
			}
			// Else we're going to add a new type to the ram data.
			cs_state_data_t & ram_data = addToRam(type, typeSize);

			// See if we need to check flash.
			if (DefaultLocation(type) == PersistenceMode::RAM) {
				LOGStateDebug("Load default: %s", TypeName(ram_data.type));
				ret_code = getDefaultValue(ram_data);
				if (ret_code != ERR_SUCCESS) {
					return ret_code;
				}
			}
			else {
				ret_code = _storage->read(getFileId(type), ram_data);
				switch(ret_code) {
					case ERR_SUCCESS: {
						break;
					}
					case ERR_NOT_FOUND:
					default: {
						LOGStateDebug("Load default: %s", TypeName(ram_data.type));
						ret_code = getDefaultValue(ram_data);
						if (ret_code != ERR_SUCCESS) {
							return ret_code;
						}
						break;
					}
				}
			}
			// Finally, copy data from ram to user data.
			data.size = ram_data.size;
			memcpy(data.value, ram_data.value, ram_data.size);
			break;
		}
	}
	return ret_code;
}

cs_ret_code_t State::getDefaultValue(cs_state_data_t & data) {
	return getDefault(data, *_boardsConfig);
}

// TODO: deleteFromRam to limit RAM usage

/**
 * There are three modes:
 *   RAM: store item in volatile memory
 *   FLASH: store item in persistent memory
 *   STRATEGY: store item depending on its default location, keep a copy in RAM.
 */
cs_ret_code_t State::setInternal(const cs_state_data_t & data, const PersistenceMode mode) {
	LOGStateDebug("Set value: %s: %u", TypeName(data.type), data.type);
	cs_ret_code_t ret_code = ERR_UNSPECIFIED;
	CS_TYPE type = data.type;
	size16_t typeSize = TypeSize(type);
	if (typeSize == 0) {
		LOGw("Wrong type %u", data.type)
		return ERR_UNKNOWN_TYPE;
	}
	if (data.size < typeSize) {
		return ERR_BUFFER_TOO_SMALL;
	}
	switch(mode) {
		case PersistenceMode::RAM: {
			return storeInRam(data);
		}
		case PersistenceMode::FLASH: {
			return ERR_NOT_AVAILABLE;
			// By the time the data is written to flash, the data pointer might be invalid.
			// There is also no guarantee that the data pointer is aligned.
			//return _storage->write(getFileId(type), data);
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
			LOGStateDebug("Item stored in RAM: %i", index);
			if (ret_code != ERR_SUCCESS) {
				LOGw("Failed to store in RAM");
				return ret_code;
			}
			// now we have a duplicate of our data we can safely store it to FLASH asynchronously
			ret_code = storeInFlash(index);
			if (ret_code == ERR_BUSY) {
				return addToQueue(CS_STATE_QUEUE_OP_WRITE, getFileId(type), type, STATE_RETRY_STORE_DELAY_MS);
			}
			break;
		}
		case PersistenceMode::FIRMWARE_DEFAULT: {
			LOGe("Default cannot be written");
			ret_code = ERR_WRITE_NOT_ALLOWED;
			break;
		}
	}
	return ret_code;
}

cs_ret_code_t State::removeInternal(const CS_TYPE & type, const PersistenceMode mode) {
	LOGStateDebug("Remove value: %s", TypeName(type));
	cs_ret_code_t ret_code = ERR_UNSPECIFIED;
	switch(mode) {
	case PersistenceMode::RAM: {

		break;
	}
	case PersistenceMode::FLASH: {
		return ERR_NOT_IMPLEMENTED;
	}
	case PersistenceMode::STRATEGY1: {
		switch(DefaultLocation(type)) {
		case PersistenceMode::RAM:
			return removeFromRam(type);
		case PersistenceMode::FLASH:
			// fall-through
			break;
		default:
			LOGe("PM not implemented");
			return ERR_NOT_IMPLEMENTED;
		}
		// First remove from ram, this should always succeed.
		ret_code = removeFromRam(type);
		if (ret_code != ERR_SUCCESS) {
			LOGw("Failed to remove from RAM");
			return ret_code;
		}
		// Then remove from flash asynchronously.
		ret_code = removeFromFlash(type);
		if (ret_code == ERR_BUSY) {
			return addToQueue(CS_STATE_QUEUE_OP_REM, getFileId(type), type, STATE_RETRY_STORE_DELAY_MS);
		}
		break;
	}
	case PersistenceMode::FIRMWARE_DEFAULT: {
		LOGe("Default cannot be removed");
		return ERR_NOT_AVAILABLE;
		break;
	}
	}
	return ret_code;
}

/**
 * Always first store to ram, use set() for this so that data struct is already validated.
 * Check if type is already queued. If so, overwrite the counter, so that the write to storage is pushed forward in
 * time, thus avoiding multiple writes to storage.
 */
cs_ret_code_t State::setDelayed(const cs_state_data_t & data, uint8_t delay) {
	if (delay == 0) {
		return ERR_WRONG_PARAMETER;
	}
	cs_ret_code_t ret_code = set(data, PersistenceMode::RAM);
	if (ret_code != ERR_SUCCESS) {
		return ret_code;
	}
	uint32_t delayMs = 1000 * delay;
	return addToQueue(CS_STATE_QUEUE_OP_WRITE, getFileId(data.type), data.type, delayMs);
}

cs_ret_code_t State::addToQueue(cs_state_queue_op_t operation, cs_file_id_t fileId, const CS_TYPE & type, uint32_t delayMs) {
	uint32_t delayTicks = delayMs / TICK_INTERVAL_MS;
	LOGStateDebug("Add to queue op=%u fileId=%u type=%s delayMs=%u delayTicks=%u", operation, fileId, TypeName(type), delayMs, delayTicks);
	bool found = false;
	switch (operation) {
	case CS_STATE_QUEUE_OP_WRITE:
	case CS_STATE_QUEUE_OP_REM:{
		for (size_t i=0; i<_store_queue.size(); ++i) {
			// A write operation replaces a remove operation, and vice versa.
			if ((_store_queue[i].type == type) &&
					(_store_queue[i].operation == CS_STATE_QUEUE_OP_WRITE || _store_queue[i].operation == CS_STATE_QUEUE_OP_REM)
					) {
				_store_queue[i].counter = delayTicks;
				found = true;
				break;
			}
		}
		break;
	}
	case CS_STATE_QUEUE_OP_REM_FILE:{
		for (size_t i=0; i<_store_queue.size(); ++i) {
			if ((_store_queue[i].operation == CS_STATE_QUEUE_OP_REM_FILE) && (_store_queue[i].fileId == fileId)) {
				_store_queue[i].counter = delayTicks;
				found = true;
				break;
			}
		}
		break;
	}
	case CS_STATE_QUEUE_OP_GC:
		break;
	}
	if (!found) {
		cs_state_store_queue_t item;
		item.operation = operation;
		item.fileId = fileId;
		item.type = type;
		item.counter = delayTicks;
		_store_queue.push_back(item);
	}
	LOGStateDebug("queue is now of size %u", _store_queue.size());
	return ERR_SUCCESS;
}

/**
 * Each tick, decrease the counter of all items.
 * If a counter is 0, store that item, and remove it from the list.
 * But if storage is busy, retry later by not removing item from queue, and setting counter again.
 */
void State::delayedStoreTick() {
	if (!_store_queue.empty()) {
		LOGStateDebug("delayedStoreTick");
	}
	cs_ret_code_t ret_code;
	size16_t index_in_ram;
	for (auto it = _store_queue.begin(); it != _store_queue.end(); /*it++*/) {
		if (it->counter == 0) {
			bool removeItem = true;
			switch (it->operation) {
			case CS_STATE_QUEUE_OP_WRITE: {
				ret_code = findInRam(it->type, index_in_ram);
				if (ret_code == ERR_SUCCESS) {
					ret_code = storeInFlash(index_in_ram);
					if (ret_code == ERR_BUSY) {
						removeItem = false;
					}
				}
				break;
			}
			case CS_STATE_QUEUE_OP_REM: {
				ret_code = removeFromFlash(it->type);
				if (ret_code == ERR_BUSY) {
					removeItem = false;
				}
				break;
			}
			case CS_STATE_QUEUE_OP_REM_FILE: {
				ret_code = _storage->remove(it->fileId);
				if (ret_code == ERR_BUSY) {
					removeItem = false;
				}
				break;
			}
			case CS_STATE_QUEUE_OP_GC: {
				ret_code = _storage->garbageCollect();
				if (ret_code == ERR_BUSY) {
					removeItem = false;
				}
				break;
			}
			}
			if (removeItem) {
				it = _store_queue.erase(it);
			}
			else {
				// Add to queue again.
				it->counter = STATE_RETRY_STORE_DELAY_MS / TICK_INTERVAL_MS;
				it++;
			}
		}
		else {
			it->counter--;
			it++;
		}
	}
}

cs_ret_code_t State::get(const CS_TYPE type, void *value, const size16_t size) {
	cs_state_data_t data(type, (uint8_t*)value, size);
	return get(data);
}

cs_ret_code_t State::set(const CS_TYPE type, void *value, const size16_t size) {
	cs_state_data_t data(type, (uint8_t*)value, size);
	return set(data);
}

cs_ret_code_t State::set(const cs_state_data_t & data, const PersistenceMode mode) {
	cs_ret_code_t retVal = setInternal(data, mode);
	if (retVal == ERR_SUCCESS) {
		event_t event(data.type, data.value, data.size);
		EventDispatcher::getInstance().dispatch(event);
	}
	else {
		LOGe("failed to set %u", data.type);
	}
	return retVal;
}

cs_ret_code_t State::remove(const CS_TYPE & type, const PersistenceMode mode) {
	return removeInternal(type, mode);
	// 31-10-2019 TODO: send event?
}

bool State::isTrue(CS_TYPE type, const PersistenceMode mode) {
	TYPIFY(CONFIG_MESH_ENABLED) enabled = false;
	switch(type) {
		case CS_TYPE::CONFIG_MESH_ENABLED:
		case CS_TYPE::CONFIG_ENCRYPTION_ENABLED:
		case CS_TYPE::CONFIG_IBEACON_ENABLED:
		case CS_TYPE::CONFIG_SCANNER_ENABLED:
		case CS_TYPE::CONFIG_PWM_ALLOWED:
		case CS_TYPE::CONFIG_SWITCH_LOCKED:
		case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED:
		case CS_TYPE::CONFIG_TAP_TO_TOGGLE_ENABLED: {
			cs_state_data_t data(type, &enabled, sizeof(enabled));
			get(data);
			break;
		}
		default: {}
	}
	return enabled;
}

void State::startWritesToFlash() {
	LOGd("startWritesToFlash");
	_startedWritingToFlash = true;
}

void State::factoryReset() {
	LOGw("Perform factory reset!");
	cs_ret_code_t retCode = _storage->remove(FILE_CONFIGURATION);
	switch (retCode) {
	case ERR_BUSY:
		// Retry again later.
		addToQueue(CS_STATE_QUEUE_OP_REM_FILE, FILE_CONFIGURATION, CS_TYPE::CONFIG_DO_NOT_USE, STATE_RETRY_STORE_DELAY_MS);
		break;
	default:
		break;
	}
}

void State::handleStorageError(cs_storage_operation_t operation, cs_file_id_t fileId, CS_TYPE type) {
	switch (operation) {
	case CS_STORAGE_OP_WRITE:
		LOGw("error writing type=%u, retrying later", type);
		addToQueue(CS_STATE_QUEUE_OP_WRITE, fileId, type, STATE_RETRY_STORE_DELAY_MS);
		break;
	case CS_STORAGE_OP_READ:
		break;
	case CS_STORAGE_OP_REMOVE:
		LOGw("error removing error type=%u", type);
		addToQueue(CS_STATE_QUEUE_OP_REM, fileId, type, STATE_RETRY_STORE_DELAY_MS);
		break;
	case CS_STORAGE_OP_REMOVE_FILE:
		LOGw("error removing error file=%u", fileId);
		addToQueue(CS_STATE_QUEUE_OP_REM_FILE, fileId, CS_TYPE::CONFIG_DO_NOT_USE, STATE_RETRY_STORE_DELAY_MS);
		break;
	case CS_STORAGE_OP_GC:
		LOGw("error collecting garbage");
		break;
	}
}

void State::handleEvent(event_t & event) {
	switch (event.type) {
	case CS_TYPE::EVT_TICK:
		delayedStoreTick();
		break;
	case CS_TYPE::EVT_STORAGE_REMOVE_FILE_DONE: {
		TYPIFY(EVT_STORAGE_REMOVE_FILE_DONE) fileId = *(TYPIFY(EVT_STORAGE_REMOVE_FILE_DONE)*)event.data;
		if (fileId == FILE_CONFIGURATION) {
			_performingFactoryReset = true;
			cs_ret_code_t retCode = _storage->garbageCollect();
			switch (retCode) {
			case ERR_BUSY:
				// Retry again later.
				addToQueue(CS_STATE_QUEUE_OP_GC, FILE_DO_NOT_USE, CS_TYPE::CONFIG_DO_NOT_USE, STATE_RETRY_STORE_DELAY_MS);
				break;
			default:
				break;
			}
		}
		break;
	}
	case CS_TYPE::EVT_STORAGE_GC_DONE: {
		if (_performingFactoryReset) {
			event_t resetEvent(CS_TYPE::EVT_STORAGE_FACTORY_RESET);
			EventDispatcher::getInstance().dispatch(resetEvent);
		}
		break;
	}
	case CS_TYPE::CMD_FACTORY_RESET: {
		factoryReset();
		break;
	}
	default:
		break;
	}
}
