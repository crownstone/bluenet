/** Store StateVars in RAM or persistent memory
 *
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 28, 2016
 * License: LGPLv3+
 */

#include <storage/cs_StateVars.h>
#include <events/cs_EventDispatcher.h>

#include <algorithm>

#ifdef PRINT_DEBUG
app_timer_id_t _debugTimer;

void debugprint(void * p_context) {
	StateVars::getInstance().print();
}
#endif

StateVars::StateVars() :
		_initialized(false), _storage(NULL), _resetCounter(NULL), _switchState(NULL), _accumulatedPower(NULL),
		_temperature(0) {

}

void StateVars::init() {
	_storage = &Storage::getInstance();

	if (!_storage->isInitialized()) {
		LOGe("forgot to initialize Storage!");
		return;
	}

	LOGd("loading state variables");
	_storage->getHandle(PS_ID_STATE, _stateVarsHandle);

	// this is just a "place holder" to calculate the offsets. state variables do not reside in a struct,
	// but only in the cyclic storage.
	// the cyclic storage keeps the current value and syncs it with the storage. it does not access the
	// stateVars struct!!
	ps_state_vars_t stateVars;

	_switchState = new CyclicStorage<switch_state_t, SWITCH_STATE_REDUNDANCY>(_stateVarsHandle,
	        Storage::getOffset(&stateVars, stateVars.switchState), SWITCH_STATE_DEFAULT);
	_resetCounter = new CyclicStorage<reset_counter_t, RESET_COUNTER_REDUNDANCY>(_stateVarsHandle,
	        Storage::getOffset(&stateVars, stateVars.resetCounter), RESET_COUNTER_DEFAULT);
	_accumulatedPower = new CyclicStorage<accumulated_power_t, ACCUMULATED_POWER_REDUNDANCY>(_stateVarsHandle,
	        Storage::getOffset(&stateVars, stateVars.accumulatedPower), ACCUMULATED_POWER_DEFAULT);

#ifdef PRINT_DEBUG
	Timer::getInstance().createSingleShot(_debugTimer, debugprint);
#endif

	LOGd("loading general struct")
	_storage->getHandle(PS_ID_GENERAL, _structHandle);
	loadPersistentStorage();
}

void StateVars::print() {
	LOGd("switch state:");
	_switchState->print();
	LOGd("reset counter:");
	_resetCounter->print();
	LOGd("accumulated power:");
	_accumulatedPower->print();
}

void StateVars::writeToStorage(uint8_t type, uint8_t* payload, uint8_t length, bool persistent) {
	switch(type) {
	// uint32_t variables
	case SV_RESET_COUNTER: {
		if (length == 4) {
			LOGd("payload: %p", payload);
			uint32_t value = ((uint32_t*) payload)[0];
			if (value == 0) {
				setStateVar(type, value);
			}
		} else {
			LOGe("wrong length");
		}
		break;
	}
	// variables with write disabled
	case SV_SWITCH_STATE:
	case SV_TEMPERATURE:
	default:
		LOGw("There is no such state variable (%u), or writing is disabled", type);
	}
}

bool StateVars::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {

	uint8_t* p_value;
	uint16_t size;
	switch(type) {
	case SV_RESET_COUNTER: {
		uint32_t value;
		getStateVar(type, value);
		p_value = (uint8_t*)&value;
		size = sizeof(uint32_t);
		break;
	}
	case SV_SWITCH_STATE: {
		uint8_t value;
		getStateVar(type, value);
		p_value = (uint8_t*)&value;
		size = sizeof(uint32_t);
		break;
	}
	case SV_TEMPERATURE: {
		int32_t value;
		getStateVar(type, value);
		p_value = (uint8_t*)&value;
		size = sizeof(int32_t);
		break;
	}
	case SV_TRACKED_DEVICES: {
		size = sizeof(tracked_device_list_t);
		uint8_t listBuffer[size];
		//! wrap in an object to get correct size
		TrackedDeviceList list;
		list.assign(listBuffer, size);
		//! clear the list to make sure it is correctly
		//! initialized
		list.clear();
		//! read the list from storage
		getStateVar(type, listBuffer, size);
		//! get the size from the list
		size = list.getDataLength();
		p_value = listBuffer;
		break;
	}
	case SV_SCHEDULE: {
		size = sizeof(schedule_list_t);
		uint8_t listBuffer[size];
		//! wrap in an object to get correct size
		ScheduleList list;
		list.assign(listBuffer, size);
		//! clear the list to make sure it is correctly
		//! initialized
		list.clear();
		//! read the list from storage
		getStateVar(type, listBuffer, size);
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

bool StateVars::setStateVar(uint8_t type, int32_t value) {
	switch(type) {
	case SV_TEMPERATURE: {
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

bool StateVars::getStateVar(uint8_t type, int32_t& target) {
	switch(type) {
	case SV_TEMPERATURE: {
		target = _temperature;
		LOGd("Read temperature: %d", target);
		break;
	}
	default:
		return false;
	}
	return true;
}

bool StateVars::setStateVar(uint8_t type, uint8_t value) {

	EventDispatcher& dispatcher = EventDispatcher::getInstance();
	switch(type) {
	case SV_SWITCH_STATE: {
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

bool StateVars::getStateVar(uint8_t type, uint8_t& target) {
	switch(type) {
	case SV_SWITCH_STATE: {
		target = _switchState->read();
		LOGd("Read switch state: %d", target);
		break;
	}
	default:
		return false;
	}
	return true;
}

bool StateVars::setStateVar(uint8_t type, uint32_t value) {

	switch(type) {
	case SV_RESET_COUNTER: {
		if (_resetCounter->read() != value) {
			_resetCounter->store(value);
		}
		break;
	}
	case SV_OPERATION_MODE: {
		uint32_t opMode;
		Storage::getUint32(_storageStruct.operationMode, opMode, OPERATION_MODE_SETUP);
		if (opMode != value) {
			Storage::setUint32(value, _storageStruct.operationMode);
			savePersistentStorageItem((uint8_t*) &_storageStruct.operationMode, sizeof(_storageStruct.operationMode));
		}
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

bool StateVars::getStateVar(uint8_t type, uint32_t& target) {
	switch(type) {
	case SV_RESET_COUNTER: {
		target = _resetCounter->read();
		LOGd("Read reset counter: %d", target);
		break;
	}
	case SV_SWITCH_STATE: {
		target = _switchState->read();
		LOGd("Read switch state: %d", target);
		break;
	}
	case SV_OPERATION_MODE: {
		Storage::getUint32(_storageStruct.operationMode, target, OPERATION_MODE_SETUP);
		LOGd("Read operation mode: %d", target);
		break;
	}
	default:
		return false;
	}
	return true;
}

bool StateVars::setStateVar(uint8_t type, buffer_ptr_t buffer, uint16_t size) {

	switch(type) {
	case SV_TRACKED_DEVICES: {
		Storage::setArray(buffer, _storageStruct.trackedDevices, size);
		savePersistentStorageItem(_storageStruct.trackedDevices, size);
		break;
	}
	case SV_SCHEDULE: {
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

bool StateVars::getStateVar(uint8_t type, buffer_ptr_t buffer, uint16_t size) {

	switch(type) {
	case SV_TRACKED_DEVICES: {
		Storage::getArray(_storageStruct.trackedDevices, buffer, (buffer_ptr_t) NULL, size);
		LOGd("Read tracked devices:");
		BLEutil::printArray(buffer, size);
		break;
	}
	case SV_SCHEDULE: {
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

void StateVars::publishUpdate(uint8_t type, uint8_t* data, uint16_t size) {

	EventDispatcher& dispatcher = EventDispatcher::getInstance();

	dispatcher.dispatch(type, data, size);

	if (isNotifying(type)) {
		state_vars_notifaction notification = {type, (uint8_t*)data, 4};
		dispatcher.dispatch(EVT_STATE_VARS_NOTIFICATION, &notification, sizeof(notification));
	}

}

void StateVars::loadPersistentStorage() {
	_storage->readStorage(_structHandle, &_storageStruct, sizeof(_storageStruct));
}

void StateVars::savePersistentStorage() {
	_storage->writeStorage(_structHandle, &_storageStruct, sizeof(_storageStruct));
}

void StateVars::savePersistentStorageItem(uint8_t* item, uint16_t size) {
	uint32_t offset = Storage::getOffset(&_storageStruct, item);
	_storage->writeItem(_structHandle, offset, item, size);
}

bool StateVars::isNotifying(uint8_t type) {
	std::vector<uint8_t>::iterator it = find(_notifyingStateVars.begin(), _notifyingStateVars.end(), type);
	return it != _notifyingStateVars.end();
}

void StateVars::setNotify(uint8_t type, bool enable) {
	if (enable) {
		if (!isNotifying(type)) {
			LOGd("enable notifications for %d", type);
			_notifyingStateVars.push_back(type);
			_notifyingStateVars.shrink_to_fit();
			LOGd("size: %d", _notifyingStateVars.size());
		}
	} else {
		std::vector<uint8_t>::iterator it = find(_notifyingStateVars.begin(), _notifyingStateVars.end(), type);
		if (it != _notifyingStateVars.end()) {
//		if (isNotifying(type)) {
			LOGd("disable notifications for %d", type);
			_notifyingStateVars.erase(it);
			_notifyingStateVars.shrink_to_fit();
			LOGd("size: %d", _notifyingStateVars.size());
		}
	}
}

void StateVars::factoryReset(uint32_t resetCode) {
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
	_accumulatedPower->reset();
}
