/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 28, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_SafeSwitch.h>
#include <storage/cs_State.h>
#include <events/cs_EventDispatcher.h>

SafeSwitch::SafeSwitch(const boards_config_t& board, Dimmer& dimmer, Relay& relay) :
	dimmer(&dimmer),
	relay(&relay),
	hardwareBoard(board.hardwareBoard)
{
	start();
}

void SafeSwitch::start() {
	dimmer->start();
}

switch_state_t SafeSwitch::getState() {
	return currentState;
}

cs_ret_code_t SafeSwitch::setRelay(bool on) {
	auto stateErrors = getErrorState();
	if (on && !isSafeToTurnRelayOn(stateErrors)) {
		return ERR_UNSAFE;
	}
	if (!on && !isSafeToTurnRelayOff(stateErrors)) {
		return ERR_UNSAFE;
	}
	return setRelayUnchecked(on);
}

cs_ret_code_t SafeSwitch::setRelayUnchecked(bool on) {
	if (currentState.state.relay == on) {
		return ERR_SUCCESS;
	}
	// TODO: set relay
	currentState.state.relay = on;
	store(currentState);
	return ERR_SUCCESS;
}



cs_ret_code_t SafeSwitch::setDimmer(uint8_t intensity) {
	auto stateErrors = getErrorState();
	if (intensity > 0) {
		if (!isSafeToDim(stateErrors)) {
			return ERR_UNSAFE;
		}
		if (!dimmerPowered) {
			return startDimmerPowerCheck(intensity);
		}
	}
	return setDimmerUnchecked(intensity);
}

cs_ret_code_t SafeSwitch::setDimmerUnchecked(uint8_t intensity) {
	if (currentState.state.dimmer == intensity) {
		return ERR_SUCCESS;
	}
	dimmer->set(intensity);
	currentState.state.dimmer = intensity;
	store(currentState);
	return ERR_SUCCESS;
}

cs_ret_code_t SafeSwitch::startDimmerPowerCheck(uint8_t intensity) {
	if (checkedDimmerPowerUsage || dimmerPowered) {
		return ERR_NOT_AVAILABLE;
	}

	switch (hardwareBoard) {
		// Builtin zero don't have an accurate enough power measurement.
		case ACR01B1A:
		case ACR01B1B:
		case ACR01B1C:
		case ACR01B1D:
		case ACR01B1E:
		// Plugs don't have an accurate enough power measurement.
		case ACR01B2A:
		case ACR01B2B:
		case ACR01B2C:
		case ACR01B2E:
		case ACR01B2G:
			return ERR_NOT_AVAILABLE;
			// Newer ones have an accurate power measurement, and a lower startup time of the dimmer circuit.
		default:
			break;
	}

	setDimmerUnchecked(intensity);
	setRelayUnchecked(false);

	if (dimmerCheckCountDown == 0) {
		dimmerCheckCountDown = DIMMER_BOOT_CHECK_DELAY_MS / TICK_INTERVAL_MS;
	}
	return ERR_SUCCESS;
}

void SafeSwitch::cancelDimmerPowerCheck() {
	dimmerCheckCountDown = 0;
}

void SafeSwitch::checkDimmerPower() {
	checkedDimmerPowerUsage = true;
	if (currentState.state.dimmer == 0) {
		return;
	}

	// TODO: check board type

	TYPIFY(STATE_POWER_USAGE) powerUsage;
	State::getInstance().get(CS_TYPE::STATE_POWER_USAGE, &powerUsage, sizeof(powerUsage));
	TYPIFY(CONFIG_POWER_ZERO) powerZero;
	State::getInstance().get(CS_TYPE::CONFIG_POWER_ZERO, &powerZero, sizeof(powerZero));

	LOGd("powerUsage=%i powerZero=%i", powerUsage, powerZero);

	if (powerUsage < DIMMER_BOOT_CHECK_POWER_MW
			|| (powerUsage < DIMMER_BOOT_CHECK_POWER_MW_UNCALIBRATED && powerZero == CONFIG_POWER_ZERO_INVALID)) {
		// Dimmer didn't work: turn relay on instead.
		setRelayUnchecked(true);
		setDimmerUnchecked(0);
	}
	else {
		dimmerPowered = true;
	}
	sendDimmerPoweredEvent();
}

void SafeSwitch::dimmerPoweredUp() {
	cancelDimmerPowerCheck();
	dimmerPowered = true;
	sendDimmerPoweredEvent();
}

void SafeSwitch::sendDimmerPoweredEvent() {
	TYPIFY(EVT_DIMMER_POWERED) powered = dimmerPowered;
	event_t event(CS_TYPE::EVT_DIMMER_POWERED, &powered, sizeof(powered));
	event.dispatch();
}



void SafeSwitch::forceSwitchOff() {
	dimmer->set(0);
	relay->set(false);

	currentState.state.relay = 0;
	currentState.state.dimmer = 0;
	store(currentState);

	event_t event(CS_TYPE::EVT_SWITCH_FORCED_OFF);
	EventDispatcher::getInstance().dispatch(event);
}

void SafeSwitch::forceRelayOnAndDimmerOff() {
	// First set relay on, so that the switch doesn't first turn off, and later on again.
	// The relay protects the dimmer, because it opens a parallel circuit for the current to flow through.
	relay->set(true);
	dimmer->set(0);

	currentState.state.relay = 1;
	currentState.state.dimmer = 0;
	store(currentState);

	event_t event(CS_TYPE::EVT_RELAY_FORCED_ON);
	EventDispatcher::getInstance().dispatch(event);
	event_t event(CS_TYPE::EVT_DIMMER_FORCED_OFF);
	EventDispatcher::getInstance().dispatch(event);
}



state_errors_t SafeSwitch::getErrorState() {
	TYPIFY(STATE_ERRORS) stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	return stateErrors;
}

bool SafeSwitch::isSwitchOverLoaded(state_errors_t stateErrors) {
	return stateErrors.errors.chipTemp
			|| stateErrors.errors.overCurrent;
}

bool SafeSwitch::hasDimmerError(state_errors_t stateErrors) {
	return stateErrors.errors.overCurrentDimmer
			|| stateErrors.errors.dimmerTemp
			|| stateErrors.errors.dimmerOn;
}

bool SafeSwitch::isSafeToTurnRelayOn(state_errors_t stateErrors) {
	return hasDimmerError(stateErrors) || !isSwitchOverLoaded(stateErrors);
}

bool SafeSwitch::isSafeToTurnRelayOff(state_errors_t stateErrors) {
	return !hasDimmerError(stateErrors);
}

bool SafeSwitch::isSafeToDim(state_errors_t stateErrors) {
	return !hasDimmerError(stateErrors) && !isSwitchOverLoaded(stateErrors);
}



void SafeSwitch::store(switch_state_t newState) {
	if (newState == storedState) {
		return;
	}
	storedState = newState;

	bool persistNow = false;
	cs_state_data_t stateData(CS_TYPE::STATE_SWITCH_STATE, reinterpret_cast<uint8_t*>(&newState), sizeof(newState));
	if (persistNow) {
		State::getInstance().set(stateData);
	} else {
		State::getInstance().setDelayed(stateData, SWITCH_DELAYED_STORE_MS / 1000);
	}
	LOGd("store(%u, %u%%)", currentState.state.relay, currentState.state.dimmer);
}


void SafeSwitch::goingToDfu() {
	switch (hardwareBoard) {
		// Dev boards
		case PCA10036:
		case PCA10040:
		// Builtin zero
		case ACR01B1A:
		case ACR01B1B:
		case ACR01B1C:
		case ACR01B1D:
		case ACR01B1E:
		// First builtin one
		case ACR01B10B:
		// Plugs
		case ACR01B2A:
		case ACR01B2B:
		case ACR01B2C:
		case ACR01B2E:
		case ACR01B2G: {
			// These boards turn on the dimmer when GPIO pins are floating.
			// Turn relay on, to prevent current going through the dimmer.
			setRelayUnchecked(true);
			break;
		}
		// These don't have a dimmer.
		case GUIDESTONE:
		case CS_USB_DONGLE:
		// Newer ones have dimmer enable pin.
		case ACR01B10C:
		default:
			break;
	}
}

void SafeSwitch::handleEvent(event_t & evt) {
	switch (evt.type) {
		case CS_TYPE::EVT_GOING_TO_DFU:
			goingToDfu();
			break;
		case CS_TYPE::CMD_FACTORY_RESET:
			// TODO: turn relay off (or on?) without storing
			break;
		case CS_TYPE::EVT_TICK:
			if (dimmerPowerUpCountDown && --dimmerPowerUpCountDown == 0) {
				dimmerPoweredUp();
			}
			if (dimmerCheckCountDown && --dimmerCheckCountDown == 0) {
				checkDimmerPower();
			}
			break;
		case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER:
		case CS_TYPE::EVT_DIMMER_TEMP_ABOVE_THRESHOLD:
		case CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED:
			forceRelayOnAndDimmerOff();
			break;
		case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
		case CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD:
			forceSwitchOff();
			break;
	}
}
