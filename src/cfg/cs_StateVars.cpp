/** Store StateVars in RAM or persistent memory
 *
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 28, 2016
 * License: LGPLv3+
 */

#include "cfg/cs_StateVars.h"

#define PRINT_DEBUG

using namespace BLEpp;

#ifdef PRINT_DEBUG
app_timer_id_t _debugTimer;

void debugprint(void * p_context) {
	StateVars::getInstance().print();
}
#endif

StateVars::StateVars() {
	LOGi("load state vars");
	Storage::getInstance().getHandle(PS_ID_STATE, _stateVarsHandle);

	_switchState = new CyclicStorage<switch_state_t, SWITCH_STATE_REDUNDANCY>(
			_stateVarsHandle, getOffset(_stateVars.switchState), SWITCH_STATE_DEFAULT);
	_resetCounter = new CyclicStorage<reset_counter_t, RESET_COUNTER_REDUNDANCY>(
			_stateVarsHandle, getOffset(_stateVars.resetCounter), RESET_COUNTER_DEFAULT);
//	_accumulatedPower = new CyclicStorage<accumulated_power_t, ACCUMULATED_POWER_REDUNDANCY>(
//			_stateVarsHandle, getOffset(_stateVars.accumulatedPower), ACCUMULATED_POWER_DEFAULT);

#ifdef PRINT_DEBUG
	Timer::getInstance().createSingleShot(_debugTimer, debugprint);
#endif
};

void StateVars::print() {
	LOGi("switch state:");
	_switchState->print();
	LOGi("reset counter:");
	_resetCounter->print();
//	LOGi("accumulated power:");
//	_accumulatedPower->print();
}

pstorage_size_t StateVars::getOffset(uint8_t var[]) {
	return Storage::getOffset(&_stateVars, var);
}

//	void writeToStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
void StateVars::writeToStorage(uint8_t type, uint8_t* payload, uint8_t length, bool persistent) {
	switch(type) {
	// uint32_t variables
	case SV_RESET_COUNTER: {
		uint32_t value = ((uint32_t*)payload)[0];
		if (value == 0) {
			setStateVar(type, value);
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

//bool StateVars::returnUint32(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t& value) {
//	streamBuffer->setPayload((uint8_t*)&value, sizeof(uint32_t));
//	streamBuffer->setType(type);
//	return true;
//}

//bool StateVars::getStateVar(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t value, uint32_t defaultValue) {
//	readStateVars();
//	uint8_t plen = 1;
//	uint32_t payload[plen];
//	Storage::getUint32(value, payload[0], defaultValue);
//	streamBuffer->setPayload((uint8_t*)payload, plen*sizeof(uint32_t));
//	streamBuffer->setType(type);
//	LOGd("Value set in payload: %u with len %u", payload[0], streamBuffer->length());
//	return true;
//}

bool StateVars::getStateVar(uint8_t type, uint32_t& target) {
	switch(type) {
	case SV_RESET_COUNTER: {
		target = _resetCounter->read();
		LOGd("Read reset counter: %d", target);
		break;
	}
	case SV_SWITCH_STATE: {
		LOGd("Read switch state: %d", target);
		target = _switchState->read();
		break;
	}
	default:
		return false;
	}
//	LOGd("read state var: %u", target);
	return true;
}

//bool StateVars::setStateVar(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, uint32_t& target) {
//	if (length != 4) {
//		LOGw("Expected uint32");
//		return false;
//	}
//	uint16_t val = ((uint32_t*)payload)[0];
//	LOGi("Set %u to %u", type, val);
//	Storage::setUint32(val, target);
//	if (persistent) {
//		LOGi("target: %p", &target);
//
//		pstorage_size_t offset = Storage::getOffset(&_stateVars, &target);
//		Storage::getInstance().writeItem(_stateVarsHandle, offset, &target, 4);
//	}
//	EventDispatcher::getInstance().dispatch(type, &target, 4);
//	return true;
//}

bool StateVars::setStateVar(uint8_t type, uint32_t value) {
//	LOGi("setStateVar %d", type);
	switch(type) {
	case SV_RESET_COUNTER: {
		_resetCounter->store(value);
		break;
	}
	case SV_SWITCH_STATE: {
		_switchState->store(value);
		break;
	}
	}

#ifdef PRINT_DEBUG
	Timer::getInstance().start(_debugTimer, MS_TO_TICKS(100), NULL);
#endif

	return true;
}

ps_state_vars_t& StateVars::getStateVars() {
	return _stateVars;
}

//void StateVars::loadFromStorage() {
//	Storage::getInstance().readStorage(_stateVarsHandle, &_stateVars, sizeof(_stateVars));
//}

//void StateVars::writeStateVars() {
//	ps_state_vars_t _oldVars;
//
//	Storage::getInstance().readStorage(_stateVarsHandle, &_oldVars, sizeof(_oldVars));
//
//	// todo: compare flash with working copy and only store changed variables
//}
