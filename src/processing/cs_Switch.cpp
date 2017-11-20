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

// [18.07.16] added for first version of plugins to disable the use of the igbt
//#define PWM_DISABLE

Switch::Switch():
//	_nextRelayVal(SWITCH_NEXT_RELAY_VAL_NONE),
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
#ifndef PWM_DISABLE
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
#else
	nrf_gpio_cfg_output(board.pinGpioPwm);
	if (board->flags.pwmInverted) {
		nrf_gpio_pin_set(board.pinGpioPwm);
	} else {
		nrf_gpio_pin_clear(board.pinGpioPwm);
	}
#endif

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

	//! Retrieve last switch state from persistent storage
	State::getInstance().get(STATE_SWITCH_STATE, &_switchValue, sizeof(switch_state_t));
//	LOGd("switch state: pwm=%u relay=%u", _switchValue.pwm_state, _switchValue.relay_state);

	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_switchTimerId, (app_timer_timeout_handler_t)Switch::staticTimedSwitch);
	Timer::getInstance().createSingleShot(_switchStoreStateTimerId, (app_timer_timeout_handler_t)Switch::staticTimedStoreSwitch);
}


void Switch::start() {
	PWM::getInstance().start(true);
	// Restore the pwm state. Use _setPwm(), so that we don't write to persistent storage again.
	_setPwm(_switchValue.pwm_state);
}

void Switch::onZeroCrossing() {
	PWM::getInstance().onZeroCrossing();
}

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
	if (_hasRelay) {
		relayOn();
	} else {
		pwmOn();
	}
}


void Switch::turnOff() {
#ifdef PRINT_SWITCH_VERBOSE
	LOGd("Turn OFF");
#endif
	if (_hasRelay) {
		relayOff();
	} else {
		pwmOff();
	}
}


void Switch::toggle() {
	if ((_hasRelay && getRelayState()) || (!_hasRelay && getPwm())) {
		turnOff();
	} else {
		turnOn();
	}
}


void Switch::pwmOff() {
	setPwm(0);
}


void Switch::pwmOn() {
	setPwm(255);
}


void Switch::setPwm(uint8_t value) {
	switch_state_t oldVal = _switchValue;
#ifdef PRINT_SWITCH_VERBOSE
	LOGd("set PWM %d", value);
#endif
	_setPwm(value);
	storeState(oldVal);
}


uint8_t Switch::getPwm() {
	return _switchValue.pwm_state;
}


void Switch::relayOn() {
	switch_state_t oldVal = _switchValue;
	_relayOn();
	storeState(oldVal);
}


void Switch::relayOff() {
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

	switch (_hardwareBoard) {
		case PCA10040:
		case ACR01B2C:
		case ACR01B1D: {
			// First pwm, then relay!
			// Pwm when value is 1-99, else pwm off
			if (switchState > 0 && switchState < 100) {
				_setPwm(switchState);
			}
			else {
				_setPwm(0);
			}
			// Relay on when value >= 100, else off (as the dimmer is parallel)
			if (switchState > 99) {
				_relayOn();
			}
			else {
				_relayOff();
			}
			break;
		}
		default: {
			// Always use the relay
			if (switchState) {
				_relayOn();
			}
			else {
				_relayOff();
			}
			_setPwm(0);
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
void Switch::handleMultiSwitch(multi_switch_item_t* p_item) {

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("handle multi switch");
	LOGi("  intent: %d, switchState: %d, timeout: %d", p_item->intent, p_item->switchState, p_item->timeout);
#endif

	// todo: handle different intents
	switch (p_item->intent) {
	case SPHERE_ENTER:
	case SPHERE_EXIT:
	case ENTER:
	case EXIT:
	case MANUAL:
		if (p_item->timeout == 0) {
			setSwitch(p_item->switchState);
		} else {
			delayedSwitch(p_item->switchState, p_item->timeout);
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


void Switch::_setPwm(uint8_t value) {
	if (value > 0 && !allowPwmOn()) {
		LOGd("Don't turn on pwm");
		return;
	}

#ifndef PWM_DISABLE
	// When the user wants to dim at 99%, assume the user actually wants full on, but doesn't want to use the relay.
	if (value >= 99) {
		value = 100;
	}
	PWM::getInstance().setValue(0, value);
#else
	LOGd("setPwm %u", value);
	if (value) {
		nrf_gpio_pin_set(PWM_PIN);
	} else {
		nrf_gpio_pin_clear(PWM_PIN);
	}
#endif
	_switchValue.pwm_state = value;
}


void Switch::_relayOn() {
	LOGd("relayOn");
	if (!allowSwitchOn()) {
		LOGd("Don't turn relay on");
		return;
	}

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("trigger relay on pin for %d ms", _relayHighDuration);
#endif

	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOn);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOn);
	}
	_switchValue.relay_state = 1;
}

void Switch::_relayOff() {
	LOGd("relayOff");

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("trigger relay off pin for %d ms", _relayHighDuration);
#endif

	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOff);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOff);
	}
	_switchValue.relay_state = 0;
}

void Switch::forcePwmOff() {
	LOGw("forcePwmOff");
	pwmOff();
	EventDispatcher::getInstance().dispatch(EVT_PWM_FORCED_OFF);
}

void Switch::forceSwitchOff() {
	LOGw("forceSwitchOff");
	setSwitch(0);
	EventDispatcher::getInstance().dispatch(EVT_SWITCH_FORCED_OFF);
	// Try again later, in case the first one didn't work..
	delayedSwitch(0, 5);
}

bool Switch::allowPwmOn() {
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, &stateErrors, sizeof(state_errors_t));
	LOGd("errors=%d", stateErrors.asInt);
	return !(stateErrors.errors.chipTemp || stateErrors.errors.overCurrent || stateErrors.errors.overCurrentPwm || stateErrors.errors.pwmTemp);
//	return !(stateErrors.errors.chipTemp || stateErrors.errors.pwmTemp);
}

bool Switch::allowSwitchOn() {
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, &stateErrors, sizeof(state_errors_t));
	return !(stateErrors.errors.chipTemp || stateErrors.errors.overCurrent || stateErrors.errors.pwmTemp);
//	return !(stateErrors.errors.chipTemp || stateErrors.errors.pwmTemp);
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
	case EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM: {
		forcePwmOff();
		break;
	}
	case EVT_PWM_TEMP_ABOVE_THRESHOLD: {
//		forcePwmOff();
		forceSwitchOff();
		break;
	}
	case EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
	case EVT_CHIP_TEMP_ABOVE_THRESHOLD: {
		forceSwitchOff();
		break;
	}
	}
}
