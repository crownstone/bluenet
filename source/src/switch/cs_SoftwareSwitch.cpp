/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 28, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_SoftwareSwitch.h>
#include <storage/cs_State.h>

SoftwareSwitch::SoftwareSwitch(SafeSwitch* safeSwitch) :
	safeSwitch(safeSwitch),
	intendedState(0)
{
	State::getInstance().get(CS_TYPE::CONFIG_PWM_ALLOWED, &allowDimming, sizeof(allowDimming));

	TYPIFY(CONFIG_SWITCH_LOCKED) switchLocked;
	State::getInstance().get(CS_TYPE::CONFIG_SWITCH_LOCKED, &switchLocked, sizeof(switchLocked));
	allowSwitching = !switchLocked;

//	currentState.state.relay = 0;
//	currentState.state.dimmer = 0;
}

cs_ret_code_t SoftwareSwitch::set(uint8_t intensity) {
	if (intensity > 100) {
		return ERR_WRONG_PARAMETER;
	}
	intendedState = intensity;
	return resolveIntendedState();
}

cs_ret_code_t SoftwareSwitch::resolveIntendedState() {
	switch_state_t currentState = getCurrentState();

	if (intendedState == 0) {
		cs_ret_code_t retCode = ERR_SUCCESS;
		cs_ret_code_t retCodeDimmer = setDimmer(0, currentState);
		if (retCodeDimmer != ERR_SUCCESS) {
			retCode = retCodeDimmer;
		}
		cs_ret_code_t retCodeRelay = setRelay(false, currentState);
		if (retCodeRelay != ERR_SUCCESS) {
			retCode = retCodeRelay;
		}
		return retCode;
	}
	else if (intendedState == 100) {
		if (currentState.state.relay || currentState.state.dimmer == 100) {
			return ERR_SUCCESS;
		}
		cs_ret_code_t retCode = ERR_UNSPECIFIED;
		if (allowDimming) {
			retCode = setDimmer(100, currentState);
		}
		else {
			retCode = setRelay(true, currentState);
		}
		return retCode;
	}
	else {
		cs_ret_code_t retCode = setDimmer(intendedState, currentState);
		currentState = getCurrentState();
		if (retCode == ERR_SUCCESS) {
			retCode = setRelay(false, currentState);
		}
		else {
			setRelay(true, currentState);
		}
		return retCode;
	}
}

switch_state_t SoftwareSwitch::getCurrentState() {
	return safeSwitch->getState();
}


cs_ret_code_t SoftwareSwitch::setRelay(bool on, switch_state_t currentState) {
	if (currentState.state.relay == on) {
		return ERR_SUCCESS;
	}
	if (!allowSwitching) {
		return ERR_NO_ACCESS;
	}
	return setRelayUnchecked(on);
}

cs_ret_code_t SoftwareSwitch::setRelayUnchecked(bool on) {
	return safeSwitch->setRelay(on);
}

cs_ret_code_t SoftwareSwitch::setDimmer(uint8_t intensity, switch_state_t currentState) {
	if (currentState.state.dimmer == intensity) {
		return ERR_SUCCESS;
	}
	if (!allowSwitching) {
		return ERR_NO_ACCESS;
	}
	if (!allowDimming && intensity > 0) {
		return ERR_NO_ACCESS;
	}
	return setDimmerUnchecked(intensity);
}

cs_ret_code_t SoftwareSwitch::setDimmerUnchecked(uint8_t intensity) {
	return safeSwitch->setDimmer(intensity);
}



cs_ret_code_t SoftwareSwitch::setAllowSwitching(bool allowed) {
	allowSwitching = allowed;
	TYPIFY(CONFIG_SWITCH_LOCKED) switchLocked = !allowSwitching;
	State::getInstance().set(CS_TYPE::CONFIG_SWITCH_LOCKED, &switchLocked, sizeof(switchLocked));
	return ERR_SUCCESS;
}

cs_ret_code_t SoftwareSwitch::setAllowDimming(bool allowed) {
	allowDimming = allowed;
	State::getInstance().set(CS_TYPE::CONFIG_SWITCH_LOCKED, &allowDimming, sizeof(allowDimming));
	// Update the switch state, as it may need to be changed.
	resolveIntendedState();
	return ERR_SUCCESS;
}



void SoftwareSwitch::handleEvent(event_t& evt) {
	switch (evt.type) {
		case CS_TYPE::CMD_SWITCHING_ALLOWED: {
			auto allowed = reinterpret_cast<TYPIFY(CMD_SWITCHING_ALLOWED)*>(evt.data);
			evt.result.returnCode = setAllowSwitching(*allowed);
			break;
		}
		case CS_TYPE::CMD_DIMMING_ALLOWED: {
			auto allowed = reinterpret_cast<TYPIFY(CMD_DIMMING_ALLOWED)*>(evt.data);
			evt.result.returnCode = setAllowDimming(*allowed);
			break;
		}
		case CS_TYPE::CMD_SET_RELAY: {
			auto turnOn = reinterpret_cast<TYPIFY(CMD_SET_RELAY)*>(evt.data);
			evt.result.returnCode = setRelay(*turnOn, getCurrentState());
			break;
		}
		case CS_TYPE::CMD_SET_DIMMER: {
			auto intensity = reinterpret_cast<TYPIFY(CMD_SET_DIMMER)*>(evt.data);
			evt.result.returnCode = setDimmer(*intensity, getCurrentState());
			break;
		}
		default:
			break;
	}
}
