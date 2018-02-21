/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */

#include <processing/cs_Switch.h>

#include <cfg/cs_Boards.h>
#include <cfg/cs_Config.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_PWM.h>
#include <storage/cs_State.h>
#include <storage/cs_Settings.h>
#include <cfg/cs_Strings.h>
#include <ble/cs_Stack.h>
#include <processing/cs_Scanner.h>
#include <events/cs_EventDispatcher.h>

#define PRINT_SWITCH_VERBOSE

Switch::Switch():
//	_nextRelayVal(SWITCH_NEXT_RELAY_VAL_NONE),
	_pwmPowered(false),
	_relayPowered(false),
	_hasRelay(false),
	_pinRelayOn(0),
	_pinRelayOff(0),
	_relayHighDuration(0),
	_delayedSwitchPending(false),
	_delayedSwitchState(0),
	_delayedStoreStatePending(false)
{
	LOGd(FMT_CREATE, "Switch");

	_switchTimerData = { {0} };
	_switchTimerId = &_switchTimerData;

	_switchStoreStateTimerData = { {0} };
	_switchStoreStateTimerId = &_switchStoreStateTimerData;
}

void Switch::init(const boards_config_t& board) {
	PWM& pwm = PWM::getInstance();
	uint32_t pwmPeriod;
	Settings::getInstance().get(CONFIG_PWM_PERIOD, &pwmPeriod);
	LOGd("pwm pin %d", board.pinGpioPwm);

	pwm_config_t pwmConfig;
	pwmConfig.channelCount = 1;
	pwmConfig.period_us = pwmPeriod;
	pwmConfig.channels[0].pin = board.pinGpioPwm;
	pwmConfig.channels[0].inverted = board.flags.pwmInverted;
	pwm.init(pwmConfig);

	_hardwareBoard = board.hardwareBoard;

	_hasRelay = board.flags.hasRelay;
	if (_hasRelay) {
		_pinRelayOff = board.pinGpioRelayOff;
		_pinRelayOn = board.pinGpioRelayOn;

		Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &_relayHighDuration);

		nrf_gpio_cfg_output(_pinRelayOff);
		nrf_gpio_pin_clear(_pinRelayOff);
		nrf_gpio_cfg_output(_pinRelayOn);
		nrf_gpio_pin_clear(_pinRelayOn);
	}

	// Retrieve last switch state from persistent storage
	State::getInstance().get(STATE_SWITCH_STATE, &_switchValue, sizeof(switch_state_t));
	LOGd("Stored switch state: pwm=%u relay=%u", _switchValue.pwm_state, _switchValue.relay_state);

	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_switchTimerId, (app_timer_timeout_handler_t)Switch::staticTimedSwitch);
	Timer::getInstance().createSingleShot(_switchStoreStateTimerId, (app_timer_timeout_handler_t)Switch::staticTimedStoreSwitch);
}


void Switch::start() {
	_relayPowered = true;
//	// Restore the pwm state. Use _setPwm(), so that we don't write to persistent storage again.
//	_setPwm(_switchValue.pwm_state);

	// Already start PWM, so it can sync with the zero crossings. But don't set the value yet.
	PWM::getInstance().start(true);

	// Use relay to restore pwm state instead of pwm, because the pwm can only be used after some time.
	if (_switchValue.pwm_state != 0) {
		switch_state_t oldVal = _switchValue;
//		// This shouldn't happen, but let's check it to be sure.
//		LOGd("pwm allowed: %u", Settings::getInstance().isSet(CONFIG_PWM_ALLOWED));
//		if (!Settings::getInstance().isSet(CONFIG_PWM_ALLOWED)) {
//			_switchValue.pwm_state = 0;
//		}
		// Always set pwm state to 0, just use relay.
		// This is for the case of a wall switch: you don't want to hear the relay turn on and off every time you power the crownstone.
		_switchValue.pwm_state = 0;
		_relayOn();
		storeState(oldVal);
	}
	else {
		// Make sure the relay is in the stored position (no need to store)
		if (_switchValue.relay_state == 1) {
			_relayOn();
		}
		else {
			_relayOff();
		}
	}
}

void Switch::startPwm() {
	if (_pwmPowered) {
		return;
	}
	_pwmPowered = true;

	// Restore the pwm state.
	bool success = _setPwm(_switchValue.pwm_state);
	// PWM was already started in start(), so it could sync with zero crossings.
//	PWM::getInstance().start(true); // Start after setting value, else there's a race condition.
	if (success && _switchValue.pwm_state != 0 && _switchValue.relay_state == 1) {
		// Don't use relayOff(), as that checks for switchLocked.
		switch_state_t oldVal = _switchValue;
		_relayOff();
		storeState(oldVal);
	}
	EventDispatcher::getInstance().dispatch(EVT_PWM_POWERED);
}

//void Switch::onZeroCrossing() {
//	PWM::getInstance().onZeroCrossing();
//}

void Switch::storeState(switch_state_t oldVal) {
	bool persistent = false;
//	if (oldVal.pwm_state != _switchValue.pwm_state || oldVal.relay_state != _switchValue.relay_state) {
	if (memcmp(&oldVal, &_switchValue, sizeof(switch_state_t)) != 0) {
		LOGd("storeState: %u", _switchValue);
		persistent = (oldVal.relay_state != _switchValue.relay_state);
		State::getInstance().set(STATE_SWITCH_STATE, &_switchValue, sizeof(switch_state_t), persistent);
	}

	// If not written to persistent storage immediately, do it after a delay.
	if (!persistent) {
		delayedStoreState(SWITCH_DELAYED_STORE_MS);
	}
}

void Switch::delayedStoreState(uint32_t delayMs) {
	if (_delayedStoreStatePending) {
		Timer::getInstance().stop(_switchStoreStateTimerId);
	}
	_delayedStoreStatePending = true;
	Timer::getInstance().start(_switchStoreStateTimerId, MS_TO_TICKS(delayMs), this);
}

void Switch::delayedStoreStateExecute() {
	if (!_delayedStoreStatePending) {
		return;
	}
	// Can't check if it's different from the old value, as the state is already updated, but maybe not to persistent storage.
//	switch_state_t oldVal;
//	State::getInstance().get(STATE_SWITCH_STATE, &oldVal, sizeof(switch_state_t));
//	if (memcmp(&oldVal, &_switchValue, sizeof(switch_state_t)) != 0) {
//		State::getInstance().set(STATE_SWITCH_STATE, &_switchValue, sizeof(switch_state_t), true);
//	}

	// Just write to persistent storage
	LOGd("write to storage: %u", _switchValue);
	State::getInstance().set(STATE_SWITCH_STATE, &_switchValue, sizeof(switch_state_t), true);
}


switch_state_t Switch::getSwitchState() {
#ifdef PRINT_SWITCH_VERBOSE
	LOGd(FMT_GET_INT_VAL, "Switch state", _switchValue);
#endif
	return _switchValue;
}


void Switch::turnOn() {
#ifdef PRINT_SWITCH_VERBOSE
	LOGd("Turn ON");
#endif
	setSwitch(SWITCH_ON);
}


void Switch::turnOff() {
#ifdef PRINT_SWITCH_VERBOSE
	LOGd("Turn OFF");
#endif
	setSwitch(0);
}


void Switch::toggle() {
	// TODO: maybe check if pwm is larger than some value?
	if (_switchValue.relay_state == 1 || _switchValue.pwm_state > 0) {
		setSwitch(0);
	}
	else {
		setSwitch(SWITCH_ON);
	}
}


void Switch::pwmOff() {
	setPwm(0);
}


void Switch::pwmOn() {
	setPwm(SWITCH_ON);
}


void Switch::setPwm(uint8_t value) {
	switch_state_t oldVal = _switchValue;
#ifdef PRINT_SWITCH_VERBOSE
	LOGd("set PWM %d", value);
#endif
	if (Settings::getInstance().isSet(CONFIG_SWITCH_LOCKED)) {
		LOGw("Switch locked!");
		return;
	}
//	LOGd("pwm allowed: %u", Settings::getInstance().isSet(CONFIG_PWM_ALLOWED));
//	if (!Settings::getInstance().isSet(CONFIG_PWM_ALLOWED)) {
//		LOGd("pwm not allowed");
//		return;
//	}
	bool success = _setPwm(value);
	// Turn on relay instead, when trying to dim too soon after boot.
	// Dimmer state will be restored when startPwm() is called.
//	if (_switchValue.relay_state == 0 && value != 0 && !_pwmPowered) {
	if (!success && _switchValue.relay_state == 0) {
		_relayOn();
	}
	storeState(oldVal);
}


uint8_t Switch::getPwm() {
	return _switchValue.pwm_state;
}


void Switch::relayOn() {
	if (Settings::getInstance().isSet(CONFIG_SWITCH_LOCKED)) {
		LOGw("Switch locked!");
		return;
	}
	switch_state_t oldVal = _switchValue;
	_relayOn();
	storeState(oldVal);
}


void Switch::relayOff() {
	if (Settings::getInstance().isSet(CONFIG_SWITCH_LOCKED)) {
		LOGw("Switch locked!");
		return;
	}
	switch_state_t oldVal = _switchValue;
	_relayOff();
	storeState(oldVal);
}


bool Switch::getRelayState() {
	return _switchValue.relay_state;
}


void Switch::setSwitch(uint8_t switchState) {
#ifdef PRINT_SWITCH_VERBOSE
	LOGi("Set switch state: %d", switchState);
#endif
	switch_state_t oldVal = _switchValue;
	if (Settings::getInstance().isSet(CONFIG_SWITCH_LOCKED)) {
		LOGw("Switch locked!");
		return;
	}

	switch (_hardwareBoard) {
		case ACR01B1A: {
			// Always use the relay
			if (switchState) {
				_relayOn();
			}
			else {
				_relayOff();
			}
			_setPwm(0);
			break;
		}
		default: {
			// TODO: the pwm gets set at the start of a period, which lets the light flicker in case the relay is turned off..
			// First pwm on, then relay off!
			// Otherwise, if you go from 100 to 90, the power first turns off, then to 90.
			// TODO: why not first relay on, then pwm off, when going from 90 to 100?

			// Pwm when value is 1-99, else pwm off
			bool pwmOnSuccess = true;
			if (switchState > 0 && switchState < SWITCH_ON) {
				pwmOnSuccess = _setPwm(switchState);
			}
			else {
				_setPwm(0);
			}

			// Relay on when value >= 100, or when trying to dim, but that was unsuccessful.
			// Else off (as the dimmer is parallel)
			if (switchState >= SWITCH_ON || !pwmOnSuccess) {
				if (_switchValue.relay_state == 0) {
					_relayOn();
				}
			}
			else {
				if (_switchValue.relay_state == 1) {
					_relayOff();
				}
			}
			break;
		}
	}

	// The new value overrules a timed switch.
	if (_delayedSwitchPending) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGi("clear delayed switch state");
#endif
		Timer::getInstance().stop(_switchTimerId);
		_delayedSwitchPending = false;
	}

	storeState(oldVal);
}


#if BUILD_MESHING == 1
void Switch::handleMultiSwitch(multi_switch_cmd_t* cmd) {

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("handle multi switch");
	LOGi("  intent: %d, switchState: %d, timeout: %d", cmd->intent, cmd->switchState, cmd->timeout);
#endif

	// todo: handle different intents
	switch (cmd->intent) {
	case SPHERE_ENTER:
	case SPHERE_EXIT:
	case ENTER:
	case EXIT:
	case MANUAL:
		if (cmd->timeout == 0) {
			setSwitch(cmd->switchState);
		}
		else {
			delayedSwitch(cmd->switchState, cmd->timeout);
		}
	}

}
#endif


void Switch::delayedSwitch(uint8_t switchState, uint16_t delay) {

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("trigger delayed switch state: %d, delay: %d", switchState, delay);
#endif

	if (delay == 0) {
		LOGw("delay can't be 0");
		delay = 1;
	}

	if (_delayedSwitchPending) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGi("clear existing delayed switch state");
#endif
		Timer::getInstance().stop(_switchTimerId);
	}

	_delayedSwitchPending = true;
	_delayedSwitchState = switchState;
	Timer::getInstance().start(_switchTimerId, MS_TO_TICKS(delay * 1000), this);
}


void Switch::delayedSwitchExecute() {
#ifdef PRINT_SWITCH_VERBOSE
	LOGi("execute delayed switch");
#endif

	if (_delayedSwitchPending) {
		setSwitch(_delayedSwitchState);
	}
}

void Switch::pwmNotAllowed() {
	switch_state_t oldVal = _switchValue;
	if (_switchValue.pwm_state != 0) {
		_setPwm(0);
		_relayOn();
	}
	storeState(oldVal);
}

bool Switch::_setPwm(uint8_t value) {
	LOGd("setPwm %u", value);
	if (value > 0 && !allowPwmOn()) {
		LOGi("Don't turn on pwm");
		return false;
	}

	LOGd("pwm allowed: %u", Settings::getInstance().isSet(CONFIG_PWM_ALLOWED));
	if (value != 0 && !Settings::getInstance().isSet(CONFIG_PWM_ALLOWED)) {
		LOGd("pwm not allowed");
		_switchValue.pwm_state = 0;
		return false;
	}

	// When the user wants to dim at 99%, assume the user actually wants full on, but doesn't want to use the relay.
	if (value >= (SWITCH_ON - 1)) {
		value = SWITCH_ON;
	}
	PWM::getInstance().setValue(0, value);
	_switchValue.pwm_state = value;
	if (!_pwmPowered) {
		// State stored, but not executed yet, so return false.
		return false;
	}
	return true;
}


void Switch::_relayOn() {
	LOGd("relayOn");
	if (!allowRelayOn()) {
		LOGi("Don't turn relay on");
		return;
	}


	if (_hasRelay) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGd("trigger relay on pin for %d ms", _relayHighDuration);
#endif
		nrf_gpio_pin_set(_pinRelayOn);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOn);
	}
	_switchValue.relay_state = 1;
}

void Switch::_relayOff() {
	LOGd("relayOff");
	if (!allowRelayOff()) {
		LOGi("Don't turn relay off");
		return;
	}

	if (_hasRelay) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGi("trigger relay off pin for %d ms", _relayHighDuration);
#endif
		nrf_gpio_pin_set(_pinRelayOff);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOff);
	}
	_switchValue.relay_state = 0;
}

void Switch::forcePwmOff() {
	LOGw("forcePwmOff");
	switch_state_t oldVal = _switchValue;
	_setPwm(0);
	storeState(oldVal);
	EventDispatcher::getInstance().dispatch(EVT_PWM_FORCED_OFF);
}

void Switch::forceRelayOn() {
	LOGw("forceRelayOn");
	switch_state_t oldVal = _switchValue;
	_relayOn();
	storeState(oldVal);
	EventDispatcher::getInstance().dispatch(EVT_RELAY_FORCED_ON);
	// Try again later, in case the first one didn't work..
	delayedSwitch(SWITCH_ON, 5);
}

void Switch::forceSwitchOff() {
	LOGw("forceSwitchOff");
	switch_state_t oldVal = _switchValue;
	_setPwm(0);
	_relayOff();
	storeState(oldVal);
	EventDispatcher::getInstance().dispatch(EVT_SWITCH_FORCED_OFF);
	// Try again later, in case the first one didn't work..
	delayedSwitch(0, 5);
}

bool Switch::allowPwmOn() {
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, stateErrors.asInt);
	LOGd("errors=%d", stateErrors.asInt);

	return !(stateErrors.errors.chipTemp || stateErrors.errors.overCurrent || stateErrors.errors.overCurrentPwm || stateErrors.errors.pwmTemp || stateErrors.errors.dimmerOn);
//	return !(stateErrors.errors.chipTemp || stateErrors.errors.pwmTemp);
}

bool Switch::allowRelayOff() {
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, stateErrors.asInt);

	// When dimmer has (had) problems, protect the dimmer by keeping the relay on.
	return !(stateErrors.errors.overCurrentPwm || stateErrors.errors.pwmTemp || stateErrors.errors.dimmerOn);
}

bool Switch::allowRelayOn() {
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, stateErrors.asInt);
	LOGd("errors=%d", stateErrors.asInt);

	// When dimmer has (had) problems, protect the dimmer by keeping the relay on.
	if (!allowRelayOff()) {
		return true;
	}

	// Otherwise, relay should stay off when the overall temperature was too high, or there was over current.
	return !(stateErrors.errors.chipTemp || stateErrors.errors.overCurrent);
}

void Switch::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_POWER_ON:
	case EVT_TRACKED_DEVICE_IS_NEARBY: {
		turnOn();
		break;
	}
	case EVT_POWER_OFF:
	case EVT_TRACKED_DEVICE_NOT_NEARBY: {
		turnOff();
		break;
	}
	case EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM:
	case EVT_PWM_TEMP_ABOVE_THRESHOLD:
	case EVT_DIMMER_ON_FAILURE_DETECTED:
		// First set relay on, so that the switch doesn't first turn off, and later on again.
		// The relay protects the dimmer, because the current will flow through the relay.
		forceRelayOn();
		forcePwmOff();
		break;
	case EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
	case EVT_CHIP_TEMP_ABOVE_THRESHOLD:
		forceSwitchOff();
		break;
	case EVT_PWM_ALLOWED:
		if (*(bool*)p_data == false) {
			pwmNotAllowed();
		}
		break;
	}
}
