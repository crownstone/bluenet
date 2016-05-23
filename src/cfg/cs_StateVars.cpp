/** Store StateVars in RAM or persistent memory
 *
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 28, 2016
 * License: LGPLv3+
 */

#include <cfg/cs_StateVars.h>
#include <events/cs_EventDispatcher.h>

#ifdef PRINT_DEBUG
app_timer_id_t _debugTimer;

void debugprint(void * p_context) {
	StateVars::getInstance().print();
}
#endif

StateVars::StateVars() :
		_initialized(false), _storage(NULL), _resetCounter(NULL), _switchState(NULL), _accumulatedPower(NULL) {
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
	default:
		LOGw("There is no such state variable (%u), or writing is disabled", type);
	}
}

bool StateVars::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
	uint32_t value;
	switch(type) {
	case SV_RESET_COUNTER:
	case SV_SWITCH_STATE: {
		getStateVar(type, value);
		break;
	}
	default: {
		LOGw("There is no such state variable (%u).", type);
		return false;
	}
	}

	streamBuffer->setPayload((uint8_t*) &value, sizeof(uint32_t));
	streamBuffer->setType(type);
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

bool StateVars::setStateVar(uint8_t type, uint32_t value) {
//	LOGi("setStateVar %d", type);

	EventDispatcher& dispatcher = EventDispatcher::getInstance();
	switch(type) {
	case SV_RESET_COUNTER: {
		_resetCounter->store(value);
		break;
	}
	case SV_SWITCH_STATE: {
		_switchState->store(value);
		dispatcher.dispatch(type, &value, 4);
		break;
	}
	case SV_OPERATION_MODE: {
		Storage::setUint32(value, _storageStruct.operationMode);
		savePersistentStorageItem((uint8_t*) &_storageStruct.operationMode, sizeof(_storageStruct.operationMode));
		break;
	}
	}

#ifdef PRINT_DEBUG
	Timer::getInstance().start(_debugTimer, MS_TO_TICKS(100), NULL);
#endif

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

	return true;

}

bool StateVars::getStateVar(uint8_t type, buffer_ptr_t buffer, uint16_t size) {

	switch(type) {
	case SV_TRACKED_DEVICES: {
		Storage::getArray(_storageStruct.trackedDevices, buffer, (buffer_ptr_t) NULL, size);
		break;
	}
	case SV_SCHEDULE: {
		Storage::getArray(_storageStruct.scheduleList, buffer, (buffer_ptr_t) NULL, size);
		break;
	}
	default:
		return false;
	}

	return true;
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
