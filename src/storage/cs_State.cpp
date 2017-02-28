/** Store StateVars in RAM or persistent memory
 *
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 28, 2016
 * License: LGPLv3+
 */

#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <storage/cs_Settings.h>
#include "drivers/cs_Timer.h"
#include <storage/cs_StorageHelper.h>

#include <algorithm>

//! enable to print additional debug
//#define PRINT_DEBUG

#ifdef PRINT_DEBUG
#if (NORDIC_SDK_VERSION >= 11)
app_timer_t _debugTimerData = { {0} };
app_timer_id_t _debugTimer = &_debugTimerData;
#else
app_timer_id_t _debugTimer;
#endif
void debugprint(void * p_context) {
	State::getInstance().print();
}
#endif

State::State() :
		_initialized(false), _storage(NULL), _resetCounter(NULL), _switchState(NULL), _accumulatedEnergy(NULL),
		_temperature(0), _powerUsage(0), _time(0), _factoryResetState(FACTORY_RESET_STATE_NORMAL) {
	_errorState.asInt = 0;
}

void State::init() {
	_storage = &Storage::getInstance();

	if (!_storage->isInitialized()) {
		LOGe(FMT_NOT_INITIALIZED, "Storage");
		return;
	}

//	LOGd("loading state variables");
	_storage->getHandle(PS_ID_STATE, _stateHandle);

	// this is just a "place holder" to calculate the offsets. state variables do not reside in a struct,
	// but only in the cyclic storage.
	// the cyclic storage keeps the current value and syncs it with the storage. it does not access the
	// stateVars struct!!
	ps_state_t state;

	bool defaultOn = Settings::getInstance().isSet(CONFIG_DEFAULT_ON);
	// TODO: why use pwm for this?
	switch_state_storage_t defaultSwitchState = defaultOn ? 100 : 0; // 100 for pwm

#ifdef SWITCH_STATE_PERSISTENT
	_switchState = new CyclicStorage<switch_state_storage_t, SWITCH_STATE_REDUNDANCY, small_seq_number_t>(_stateHandle,
	        StorageHelper::getOffset(&state, state.switchState), defaultSwitchState);
#else
	_switchState = defaultSwitchState;
#endif
	_resetCounter = new CyclicStorage<reset_counter_t, RESET_COUNTER_REDUNDANCY, small_seq_number_t>(_stateHandle,
	        StorageHelper::getOffset(&state, state.resetCounter), RESET_COUNTER_DEFAULT);
	_accumulatedEnergy = new CyclicStorage<accumulated_energy_t, ACCUMULATED_ENERGY_REDUNDANCY, large_seq_number_t>(_stateHandle,
	        StorageHelper::getOffset(&state, state.accumulatedEnergy), ACCUMULATED_ENERGY_DEFAULT);

#ifdef PRINT_DEBUG
	Timer::getInstance().createSingleShot(_debugTimer, debugprint);
#endif

//	LOGd("loading general struct")
	_storage->getHandle(PS_ID_GENERAL, _structHandle);
	loadPersistentStorage();
}

void State::print() {
#ifdef PRINT_DEBUG
	LOGd("switch state:");
#ifdef SWITCH_STATE_PERSISTENT
	_switchState->print();
#endif
	LOGd("reset counter:");
	_resetCounter->print();
	LOGd("accumulated power:");
	_accumulatedEnergy->print();
#endif
}

ERR_CODE State::writeToStorage(uint8_t type, uint8_t* payload, uint8_t length, bool persistent) {
	switch(type) {
	// uint32_t variables
	case STATE_RESET_COUNTER: {
		if (length == 2) {
			uint32_t value = ((uint32_t*) payload)[0];
			if (value == 0) {
				return set(type, payload, length);
			}
		} else {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		break;
	}
	// variables with write disabled
	case STATE_SWITCH_STATE:
	case STATE_TEMPERATURE:
	case STATE_FACTORY_RESET:
	default:
		LOGw(FMT_STATE_NOT_FOUND, type);
	}

	return STATE_WRITE_DISABLED;
}

ERR_CODE State::readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {

	uint16_t size;
	ERR_CODE error_code;

	switch(type) {
	case STATE_RESET_COUNTER: {
		size = sizeof(uint16_t);
		break;
	}
	case STATE_SWITCH_STATE: {
		size = sizeof(uint8_t);
		break;
	}
	case STATE_TEMPERATURE: {
		size = sizeof(int32_t);
		break;
	}
	case STATE_FACTORY_RESET: {
		size = sizeof(uint8_t);
		break;
	}
	case STATE_TIME: {
		size = sizeof(uint32_t);
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
		error_code = get(type, listBuffer, size);
		if (SUCCESS(error_code)) {
			//! get the size from the list
			size = list.getDataLength();

			streamBuffer->setPayload(listBuffer, size);
			streamBuffer->setType(type);
		}
		return error_code;
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
		error_code = get(type, listBuffer, size);
		if (SUCCESS(error_code)) {
			//! get the size from the list
			size = list.getDataLength();

			streamBuffer->setPayload(listBuffer, size);
			streamBuffer->setType(type);
		}
		return error_code;
	}
	case STATE_LEARNED_SWITCHES:
	default: {
		LOGw(FMT_STATE_NOT_FOUND, type);
		return ERR_STATE_NOT_FOUND;
	}
	}

	uint8_t payload[size];
	error_code = get(type, payload, size);
	if (SUCCESS(error_code)) {
		streamBuffer->setPayload(payload, size);
		streamBuffer->setType(type);
	}
	return error_code;
}

uint16_t State::getStateItemSize(uint8_t type) {

	switch(type) {
	case STATE_RESET_COUNTER: {
		return sizeof(reset_counter_t);
	}
	case STATE_SWITCH_STATE: {
		return sizeof(uint8_t);
	}
	case STATE_ACCUMULATED_ENERGY: {
		return sizeof(accumulated_energy_t);
	}
	case STATE_POWER_USAGE: {
		return sizeof(_powerUsage);
	}
	case STATE_TRACKED_DEVICES: {
		return sizeof(tracked_device_list_t);
	}
	case STATE_SCHEDULE: {
		return sizeof(schedule_list_t);
	}
	case STATE_OPERATION_MODE:
	case STATE_ERROR_OVER_CURRENT:
	case STATE_ERROR_OVER_CURRENT_PWM:
	case STATE_ERROR_CHIP_TEMP:
	case STATE_ERROR_PWM_TEMP: {
		return sizeof(uint8_t);
	}
	case STATE_TEMPERATURE: {
		return sizeof(_temperature);
	}
	case STATE_FACTORY_RESET: {
		return sizeof(_factoryResetState);
	}
	case STATE_TIME: {
		return sizeof(_time);
	}
	case STATE_LEARNED_SWITCHES: {
		return MAX_SWITCHES * sizeof(learned_enocean_t);
	}
	case STATE_ERRORS: {
		return sizeof(state_errors_t);
	}
	default: {
		LOGw(FMT_STATE_NOT_FOUND, type);
		return 0;
	}
	}
}

ERR_CODE State::verify(uint8_t type, uint16_t size) {

	bool success;
	switch(type) {
	case STATE_RESET_COUNTER:
	case STATE_SWITCH_STATE:
	case STATE_ACCUMULATED_ENERGY:
	case STATE_POWER_USAGE:
	case STATE_OPERATION_MODE:
	case STATE_TEMPERATURE:
	case STATE_FACTORY_RESET:
	case STATE_TIME:
	case STATE_ERRORS:
	case STATE_ERROR_OVER_CURRENT:
	case STATE_ERROR_OVER_CURRENT_PWM:
	case STATE_ERROR_CHIP_TEMP:
	case STATE_ERROR_PWM_TEMP: {
		success = size == getStateItemSize(type);
		break;
	}
	case STATE_TRACKED_DEVICES:
	case STATE_SCHEDULE:
	case STATE_LEARNED_SWITCHES: {
		success = size <= getStateItemSize(type);
		break;
	}
	default: {
		LOGw(FMT_STATE_NOT_FOUND, type);
		return ERR_STATE_NOT_FOUND;
	}
	}

	if (success) {
		return ERR_SUCCESS;
	} else {
		LOGw(FMT_VERIFICATION_FAILED, type);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
}

ERR_CODE State::set(uint8_t type, void* target, uint16_t size) {

	ERR_CODE error_code;
	error_code = verify(type, size);

	if (SUCCESS(error_code)) {
		switch(type) {
		case STATE_TEMPERATURE: {
			_temperature = *(int32_t*)target;
//#ifdef PRINT_DEBUG
//			LOGd("Set temperature: %d", _temperature);
//#endif
			break;
		}
		case STATE_FACTORY_RESET: {
			_factoryResetState = *(uint8_t*)target;
			break;
		}
		case STATE_SWITCH_STATE: {
			uint8_t value = *(uint8_t*)target;
#ifdef SWITCH_STATE_PERSISTENT
			if (_switchState->read() != value) {
				_switchState->store(value);
			}
#else
			_switchState = value;
#endif
			break;
		}
		case STATE_OPERATION_MODE: {
			uint8_t value = *(uint8_t*)target;
			uint32_t opMode;
			StorageHelper::getUint32(_storageStruct.operationMode, &opMode, OPERATION_MODE_SETUP);
			if (opMode != value) {
				StorageHelper::setUint32(value, _storageStruct.operationMode);
				savePersistentStorageItem((uint8_t*) &_storageStruct.operationMode, sizeof(_storageStruct.operationMode));
			}
			break;
		}
		case STATE_RESET_COUNTER: {
			uint16_t value = *(uint16_t*)target;
			if (_resetCounter->read() != value) {
				_resetCounter->store(value);
			}
			break;
		}
		case STATE_POWER_USAGE: {
			_powerUsage = *(int32_t*)target;
			break;
		}
		case STATE_TIME: {
			_time = *(uint32_t*)target;
//#ifdef PRINT_DEBUG
//			LOGd("set time: %d", _time);
//#endif
			break;
		}
		case STATE_TRACKED_DEVICES: {
			StorageHelper::setArray((buffer_ptr_t)target, _storageStruct.trackedDevices, size);
			savePersistentStorageItem(_storageStruct.trackedDevices, size);
			break;
		}
		case STATE_SCHEDULE: {
			StorageHelper::setArray((buffer_ptr_t)target, _storageStruct.scheduleList, size);
			savePersistentStorageItem(_storageStruct.scheduleList, size);
			break;
		}
		case STATE_LEARNED_SWITCHES: {
			StorageHelper::setArray((buffer_ptr_t)target, _storageStruct.learnedSwitches, size);
			savePersistentStorageItem(_storageStruct.learnedSwitches, size);
			break;
		}
		case STATE_ERRORS: {
			_errorState = *(state_errors_t*)target;
			break;
		}
		case STATE_ERROR_OVER_CURRENT: {
			_errorState.errors.overCurrent = *(uint8_t*)target;
			break;
		}
		case STATE_ERROR_OVER_CURRENT_PWM: {
			_errorState.errors.overCurrentPwm = *(uint8_t*)target;
			break;
		}
		case STATE_ERROR_CHIP_TEMP: {
			_errorState.errors.chipTemp = *(uint8_t*)target;
			break;
		}
		case STATE_ERROR_PWM_TEMP: {
			_errorState.errors.pwmTemp = *(uint8_t*)target;
			break;
		}
		case STATE_ACCUMULATED_ENERGY: {
//			break;
		}
		default:
			return ERR_STATE_NOT_FOUND;
		}

		publishUpdate(type, (uint8_t*)target, size);
		return ERR_SUCCESS;
	}

	return error_code;
}

ERR_CODE State::get(uint8_t type, void* target, uint16_t size) {

	ERR_CODE error_code;
	error_code = verify(type, size);

	if (SUCCESS(error_code)) {
		switch(type) {
		case STATE_TEMPERATURE: {
			*(int32_t*)target = _temperature;
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_INT_VAL, "temperature", *(int32_t*)target);
#endif
			break;
		}
		case STATE_FACTORY_RESET: {
			*(uint8_t*)target = _factoryResetState;
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_INT_VAL, "factory reset", *(uint8_t*)target);
#endif
			break;
		}
		case STATE_OPERATION_MODE: {
			StorageHelper::getUint8(_storageStruct.operationMode, (uint8_t*)target, DEFAULT_OPERATION_MODE);
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_INT_VAL, "operation mode", *(uint8_t*)target);
#endif
			break;
		}
		case STATE_RESET_COUNTER: {
			*(uint16_t*)target = _resetCounter->read();
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_INT_VAL, "reset counter", *(uint16_t*)target);
#endif
			break;
		}
		case STATE_SWITCH_STATE: {
#ifdef SWITCH_STATE_PERSISTENT
			*(uint8_t*)target = _switchState->read();
#else
			*(uint8_t*)target = _switchState;
#endif
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_INT_VAL, "switch state", *(uint8_t*)target);
#endif
			break;
		}
		case STATE_POWER_USAGE: {
			*(int32_t*)target = _powerUsage;
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_INT_VAL, "power usage", *(int32_t*)target);
#endif
			break;
		}
		case STATE_TIME: {
			*(uint32_t*)target = _time;
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_INT_VAL, "time", *(uint32_t*)target);
#endif
			break;
		}
		case STATE_TRACKED_DEVICES: {
			StorageHelper::getArray(_storageStruct.trackedDevices, (buffer_ptr_t)target, (buffer_ptr_t) NULL, size);
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_STR_VAL, "tracked devices", "");
			BLEutil::printArray((buffer_ptr_t)target, size);
#endif
			break;
		}
		case STATE_SCHEDULE: {
			StorageHelper::getArray(_storageStruct.scheduleList, (buffer_ptr_t)target, (buffer_ptr_t) NULL, size);
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_STR_VAL, "schedule list", "");
			BLEutil::printArray((buffer_ptr_t)target, size);
#endif
			break;
		}
		case STATE_LEARNED_SWITCHES: {
			StorageHelper::getArray(_storageStruct.learnedSwitches, (buffer_ptr_t)target, (buffer_ptr_t) NULL, size);
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_STR_VAL, "learned switches", "");
			BLEutil::printArray((buffer_ptr_t)target, size);
#endif
			break;
		}
		case STATE_ERRORS: {
			*(state_errors_t*)target = _errorState;
#ifdef PRINT_DEBUG
			LOGd(FMT_GET_INT_VAL, "errorState", *(state_errors_t*)target);
#endif
			break;
		}
		case STATE_ACCUMULATED_ENERGY: {
//			break;
		}
		default:
			return ERR_STATE_NOT_FOUND;
		}

//		publishUpdate(type, (uint8_t*)target, size);
		return ERR_SUCCESS;
	}

	return error_code;
}

void State::publishUpdate(uint8_t type, uint8_t* data, uint16_t size) {

	EventDispatcher& dispatcher = EventDispatcher::getInstance();

	dispatcher.dispatch(type, data, size);

	if (isNotifying(type)) {
		state_vars_notifaction notification = {type, (uint8_t*)data, size};
		dispatcher.dispatch(EVT_STATE_NOTIFICATION, &notification, sizeof(notification));
	}

}

void State::loadPersistentStorage() {
	_storage->readStorage(_structHandle, &_storageStruct, sizeof(_storageStruct));
}

void State::savePersistentStorage() {
	_storage->writeStorage(_structHandle, &_storageStruct, sizeof(_storageStruct));
}

void State::savePersistentStorageItem(uint8_t* item, uint16_t size) {
	uint32_t offset = StorageHelper::getOffset(&_storageStruct, item);
	_storage->writeItem(_structHandle, offset, item, size);
}

bool State::isNotifying(uint8_t type) {
	std::vector<uint8_t>::iterator it = find(_notifyingStates.begin(), _notifyingStates.end(), type);
	return it != _notifyingStates.end();
}

void State::setNotify(uint8_t type, bool enable) {
	if (enable) {
		if (!isNotifying(type)) {
#ifdef PRINT_DEBUG
			LOGd("enable notifications for %d", type);
#endif
			_notifyingStates.push_back(type);
			_notifyingStates.shrink_to_fit();
//			LOGd("size: %d", _notifyingStates.size());
		}
	} else {
		std::vector<uint8_t>::iterator it = find(_notifyingStates.begin(), _notifyingStates.end(), type);
		if (it != _notifyingStates.end()) {
//		if (isNotifying(type)) {
#ifdef PRINT_DEBUG
			LOGd("disable notifications for %d", type);
#endif
			_notifyingStates.erase(it);
			_notifyingStates.shrink_to_fit();
//			LOGd("size: %d", _notifyingStates.size());
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

	LOGw("resetting state variables");

	// clear the storage in flash
	_storage->clearStorage(PS_ID_STATE);
	_storage->clearStorage(PS_ID_GENERAL);

	// reload struct
	memset(&_storageStruct, 0xFF, sizeof(_storageStruct));
//	loadPersistentStorage();

	// reset the cyclic storage objects
#ifdef SWITCH_STATE_PERSISTENT
	_switchState->reset();
#endif
	_resetCounter->reset();
	_accumulatedEnergy->reset();
}
