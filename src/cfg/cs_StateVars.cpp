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

StateVars::StateVars() {
	LOGd("loading state variables");
	Storage::getInstance().getHandle(PS_ID_STATE, _stateVarsHandle);

	// this is just a "place holder" to calculate the offsets. state variables do not reside in a struct,
	// but only in the cyclic storage.
	// the cyclic storage keeps the current value and syncs it with the storage. it does not access the
	// stateVars struct!!
	ps_state_vars_t* stateVars;

	_switchState = new CyclicStorage<switch_state_t, SWITCH_STATE_REDUNDANCY>(
			_stateVarsHandle, Storage::getOffset(stateVars, stateVars->switchState), SWITCH_STATE_DEFAULT);
	_resetCounter = new CyclicStorage<reset_counter_t, RESET_COUNTER_REDUNDANCY>(
			_stateVarsHandle, Storage::getOffset(stateVars, stateVars->resetCounter), RESET_COUNTER_DEFAULT);
	_accumulatedPower = new CyclicStorage<accumulated_power_t, ACCUMULATED_POWER_REDUNDANCY>(
			_stateVarsHandle, Storage::getOffset(stateVars, stateVars->accumulatedPower), ACCUMULATED_POWER_DEFAULT);

#ifdef PRINT_DEBUG
	Timer::getInstance().createSingleShot(_debugTimer, debugprint);
#endif
};

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
			uint32_t value = ((uint32_t*)payload)[0];
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

	streamBuffer->setPayload((uint8_t*)&value, sizeof(uint32_t));
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
	}

#ifdef PRINT_DEBUG
	Timer::getInstance().start(_debugTimer, MS_TO_TICKS(100), NULL);
#endif

	return true;
}
