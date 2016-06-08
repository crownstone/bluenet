/** Store StateVars in RAM or persistent memory
 *
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 28, 2016
 * License: LGPLv3+
 */

#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>

#include <algorithm>

#ifdef PRINT_DEBUG
app_timer_id_t _debugTimer;

void debugprint(void * p_context) {
	State::getInstance().print();
}
#endif

State::State() :
		_initialized(false), _storage(NULL), _resetCounter(NULL), _switchState(NULL), _accumulatedEnergy(NULL),
		_temperature(0) {

}

void State::init() {
	_storage = &Storage::getInstance();

	if (!_storage->isInitialized()) {
		LOGe("forgot to initialize Storage!");
		return;
	}

	LOGd("loading state variables");
	_storage->getHandle(PS_ID_STATE, _stateHandle);

	// this is just a "place holder" to calculate the offsets. state variables do not reside in a struct,
	// but only in the cyclic storage.
	// the cyclic storage keeps the current value and syncs it with the storage. it does not access the
	// stateVars struct!!
	ps_state_t state;

	_switchState = new CyclicStorage<switch_state_t, SWITCH_STATE_REDUNDANCY>(_stateHandle,
	        Storage::getOffset(&state, state.switchState), SWITCH_STATE_DEFAULT);
	_resetCounter = new CyclicStorage<reset_counter_t, RESET_COUNTER_REDUNDANCY>(_stateHandle,
	        Storage::getOffset(&state, state.resetCounter), RESET_COUNTER_DEFAULT);
	_accumulatedEnergy = new CyclicStorage<accumulated_energy_t, ACCUMULATED_ENERGY_REDUNDANCY>(_stateHandle,
	        Storage::getOffset(&state, state.accumulatedEnergy), ACCUMULATED_ENERGY_DEFAULT);

#ifdef PRINT_DEBUG
	Timer::getInstance().createSingleShot(_debugTimer, debugprint);
#endif

	LOGd("loading general struct")
	_storage->getHandle(PS_ID_GENERAL, _structHandle);
	loadPersistentStorage();
}

void State::print() {
	LOGd("switch state:");
	_switchState->print();
	LOGd("reset counter:");
	_resetCounter->print();
	LOGd("accumulated power:");
	_accumulatedEnergy->print();
}

void State::writeToStorage(uint8_t type, uint8_t* payload, uint8_t length, bool persistent) {
	switch(type) {
	// uint32_t variables
	case STATE_RESET_COUNTER: {
		if (length == 4) {
			LOGd("payload: %p", payload);
			uint32_t value = ((uint32_t*) payload)[0];
			if (value == 0) {
				set(type, value);
			}
		} else {
			LOGe("wrong length");
		}
		break;
	}
	// variables with write disabled
	case STATE_SWITCH_STATE:
	case STATE_TEMPERATURE:
	default:
		LOGw("There is no such state variable (%u), or writing is disabled", type);
	}
}

bool State::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {

	uint8_t* p_value;
	uint16_t size;
	switch(type) {
	case STATE_RESET_COUNTER: {
		uint32_t value;
		get(type, value);
		p_value = (uint8_t*)&value;
		size = sizeof(uint32_t);
		break;
	}
	case STATE_SWITCH_STATE: {
		uint8_t value;
		get(type, value);
		p_value = (uint8_t*)&value;
		size = sizeof(uint32_t);
		break;
	}
	case STATE_TEMPERATURE: {
		int32_t value;
		get(type, value);
		p_value = (uint8_t*)&value;
		size = sizeof(int32_t);
		break;
	}
	case STATE_TRACKED_DEVICES: {
		size = sizeof(tracked_device_list_t);
		uint8_t listBuffer[size];
		//! wrap in an object to get correct size
		TrackedDeviceList list;
		list.assign(listBuffer, size);
		//! clear the list to make sure it is correctly
		//! initialized
		list.clear();
		//! read the list from storage
		get(type, listBuffer, size);
		//! get the size from the list
		size = list.getDataLength();
		p_value = listBuffer;
		break;
	}
	case STATE_SCHEDULE: {
		size = sizeof(schedule_list_t);
		uint8_t listBuffer[size];
		//! wrap in an object to get correct size
		ScheduleList list;
		list.assign(listBuffer, size);
		//! clear the list to make sure it is correctly
		//! initialized
		list.clear();
		//! read the list from storage
		get(type, listBuffer, size);
		//! get the size from the list
		size = list.getDataLength();
		p_value = listBuffer;
		break;
	}
	default: {
		LOGw("There is no such state variable (%u).", type);
		return false;
	}
	}

	streamBuffer->setPayload(p_value, size);
	streamBuffer->setType(type);
	return true;
}

bool State::set(uint8_t type, int32_t value) {
	switch(type) {
	case STATE_TEMPERATURE: {
//		LOGd("Set temperature: %d", value);
		_temperature = value;
		break;
	}
	default:
		return false;
	}

	publishUpdate(type, (uint8_t*)&value, sizeof(int32_t));

	return true;
}

bool State::get(uint8_t type, int32_t& target) {
	switch(type) {
	case STATE_TEMPERATURE: {
		target = _temperature;
		LOGd("Read temperature: %d", target);
		break;
	}
	default:
		return false;
	}
	return true;
}

bool State::set(uint8_t type, uint8_t value) {

	EventDispatcher& dispatcher = EventDispatcher::getInstance();
	switch(type) {
	case STATE_SWITCH_STATE: {
		if (_switchState->read() != value) {
			_switchState->store(value);
		}
		break;
	}
	default:
		return false;
	}

	publishUpdate(type, &value, sizeof(uint8_t));

#ifdef PRINT_DEBUG
	Timer::getInstance().start(_debugTimer, MS_TO_TICKS(100), NULL);
#endif

	return true;
}

bool State::get(uint8_t type, uint8_t& target) {
	switch(type) {
	case STATE_SWITCH_STATE: {
		target = _switchState->read();
		LOGd("Read switch state: %d", target);
		break;
	}
	default:
		return false;
	}
	return true;
}

bool State::set(uint8_t type, uint32_t value) {

	switch(type) {
	case STATE_RESET_COUNTER: {
		if (_resetCounter->read() != value) {
			_resetCounter->store(value);
		}
		break;
	}
	case STATE_OPERATION_MODE: {
		uint32_t opMode;
		Storage::getUint32(_storageStruct.operationMode, opMode, OPERATION_MODE_SETUP);
		if (opMode != value) {
			Storage::setUint32(value, _storageStruct.operationMode);
			savePersistentStorageItem((uint8_t*) &_storageStruct.operationMode, sizeof(_storageStruct.operationMode));
		}
		break;
	}
	case STATE_POWER_USAGE: {
		_powerUsage = value;
		break;
	}
	default:
		return false;
	}

	publishUpdate(type, (uint8_t*)&value, sizeof(uint32_t));

#ifdef PRINT_DEBUG
	Timer::getInstance().start(_debugTimer, MS_TO_TICKS(100), NULL);
#endif

	return true;
}

bool State::get(uint8_t type, uint32_t& target) {
	switch(type) {
	case STATE_RESET_COUNTER: {
		target = _resetCounter->read();
		LOGd("Read reset counter: %d", target);
		break;
	}
	case STATE_SWITCH_STATE: {
		target = _switchState->read();
		LOGd("Read switch state: %d", target);
		break;
	}
	case STATE_OPERATION_MODE: {
		Storage::getUint32(_storageStruct.operationMode, target, OPERATION_MODE_SETUP);
		LOGd("Read operation mode: %d", target);
		break;
	}
	case STATE_POWER_USAGE: {
		target = _powerUsage;
		LOGd("Read power usage: %d", target);
		break;
	}
	default:
		return false;
	}
	return true;
}

bool State::set(uint8_t type, buffer_ptr_t buffer, uint16_t size) {

	switch(type) {
	case STATE_TRACKED_DEVICES: {
		Storage::setArray(buffer, _storageStruct.trackedDevices, size);
		savePersistentStorageItem(_storageStruct.trackedDevices, size);
		break;
	}
	case STATE_SCHEDULE: {
		Storage::setArray(buffer, _storageStruct.scheduleList, size);
		savePersistentStorageItem(_storageStruct.scheduleList, size);
		break;
	}
	default:
		return false;
	}

	publishUpdate(type, buffer, size);

	return true;

}

bool State::get(uint8_t type, buffer_ptr_t buffer, uint16_t size) {

	switch(type) {
	case STATE_TRACKED_DEVICES: {
		Storage::getArray(_storageStruct.trackedDevices, buffer, (buffer_ptr_t) NULL, size);
		LOGd("Read tracked devices:");
		BLEutil::printArray(buffer, size);
		break;
	}
	case STATE_SCHEDULE: {
		Storage::getArray(_storageStruct.scheduleList, buffer, (buffer_ptr_t) NULL, size);
		LOGd("Read schedule list:");
		BLEutil::printArray(buffer, size);
		break;
	}
	default:
		return false;
	}

	return true;
}

void State::publishUpdate(uint8_t type, uint8_t* data, uint16_t size) {

	EventDispatcher& dispatcher = EventDispatcher::getInstance();

	dispatcher.dispatch(type, data, size);

	if (isNotifying(type)) {
		state_vars_notifaction notification = {type, (uint8_t*)data, 4};
		dispatcher.dispatch(EVT_STATE_VARS_NOTIFICATION, &notification, sizeof(notification));
	}

}

void State::loadPersistentStorage() {
	_storage->readStorage(_structHandle, &_storageStruct, sizeof(_storageStruct));
}

void State::savePersistentStorage() {
	_storage->writeStorage(_structHandle, &_storageStruct, sizeof(_storageStruct));
}

void State::savePersistentStorageItem(uint8_t* item, uint16_t size) {
	uint32_t offset = Storage::getOffset(&_storageStruct, item);
	_storage->writeItem(_structHandle, offset, item, size);
}

bool State::isNotifying(uint8_t type) {
	std::vector<uint8_t>::iterator it = find(_notifyingStates.begin(), _notifyingStates.end(), type);
	return it != _notifyingStates.end();
}

void State::setNotify(uint8_t type, bool enable) {
	if (enable) {
		if (!isNotifying(type)) {
			LOGd("enable notifications for %d", type);
			_notifyingStates.push_back(type);
			_notifyingStates.shrink_to_fit();
			LOGd("size: %d", _notifyingStates.size());
		}
	} else {
		std::vector<uint8_t>::iterator it = find(_notifyingStates.begin(), _notifyingStates.end(), type);
		if (it != _notifyingStates.end()) {
//		if (isNotifying(type)) {
			LOGd("disable notifications for %d", type);
			_notifyingStates.erase(it);
			_notifyingStates.shrink_to_fit();
			LOGd("size: %d", _notifyingStates.size());
		}
	}
}

void State::factoryReset(uint32_t resetCode) {
	if (resetCode != FACTORY_RESET_CODE) {
		LOGe("wrong reset code!");
		return;
	}

	LOGw("resetting state vars");

	// clear the storage in flash
	_storage->clearStorage(PS_ID_STATE);
	_storage->clearStorage(PS_ID_GENERAL);

	// reload struct
	memset(&_storageStruct, 0xFF, sizeof(_storageStruct));
//	loadPersistentStorage();

	// reset the cyclic storage objects
	_switchState->reset();
	_resetCounter->reset();
	_accumulatedEnergy->reset();
}
