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

//#define PRINT_SWITCH_VERBOSE

// [18.07.16] added for first version of plugins to disable the use of the igbt
//#define PWM_DISABLE

Switch::Switch():
//	_nextRelayVal(SWITCH_NEXT_RELAY_VAL_NONE),
	_hasRelay(false), _pinRelayOn(0), _pinRelayOff(0), _relayHighDuration(0), _delayedSwitchPending(false), _delayedSwitchState(0)
{
	LOGd(FMT_CREATE, "Switch");

	_switchTimerData = { {0} };
	_switchTimerId = &_switchTimerData;
}

void Switch::init(boards_config_t* board) {
#ifndef PWM_DISABLE
	PWM& pwm = PWM::getInstance();
	uint32_t pwmPeriod;
	Settings::getInstance().get(CONFIG_PWM_PERIOD, &pwmPeriod);
	pwm.init(pwm.config1Ch(pwmPeriod, board->pinGpioPwm, board->flags.pwmInverted), board->flags.pwmInverted); //! 50 Hz
#else
	nrf_gpio_cfg_output(board->pinGpioPwm);
	if (board->flags.pwmInverted) {
		nrf_gpio_pin_set(board->pinGpioPwm);
	} else {
		nrf_gpio_pin_clear(board->pinGpioPwm);
	}
#endif

	_hasRelay = board->flags.hasRelay;
	if (_hasRelay) {
		_pinRelayOff = board->pinGpioRelayOff;
		_pinRelayOn = board->pinGpioRelayOn;

		Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &_relayHighDuration);

		nrf_gpio_cfg_output(_pinRelayOff);
		nrf_gpio_pin_clear(_pinRelayOff);
		nrf_gpio_cfg_output(_pinRelayOn);
		nrf_gpio_pin_clear(_pinRelayOn);
	}

	//! Retrieve last switch state from persistent storage
	State::getInstance().get(STATE_SWITCH_STATE, &_switchValue, 1);
//	LOGd("switch state: pwm=%u relay=%u", _switchValue.pwm_state, _switchValue.relay_state);

	// For now: just turn pwm off on init, for safety.
	pwmOff();

	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_switchTimerId, (app_timer_timeout_handler_t)Switch::staticTimedSwitch);
}


void Switch::start() {

}

void Switch::updateSwitchState(switch_state_t oldVal) {
//	if (oldVal.pwm_state != _switchValue.pwm_state || oldVal.relay_state != _switchValue.relay_state) {
	if (memcmp(&oldVal, &_switchValue, sizeof(switch_state_t)) != 0) {
		LOGd("updateSwitchState: %d", _switchValue);
		State::getInstance().set(STATE_SWITCH_STATE, &_switchValue, sizeof(switch_state_t));
	}
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
	updateSwitchState(oldVal);
}


uint8_t Switch::getPwm() {
	return _switchValue.pwm_state;
}


void Switch::relayOn() {
	switch_state_t oldVal = _switchValue;
	_relayOn();
	updateSwitchState(oldVal);
}


void Switch::relayOff() {
	switch_state_t oldVal = _switchValue;
	_relayOff();
	updateSwitchState(oldVal);
}


bool Switch::getRelayState() {
	return _switchValue.relay_state;
}


void Switch::setSwitch(uint8_t switchState) {
#ifdef PRINT_SWITCH_VERBOSE
	LOGi("Set switch state: %d", switchState);
#endif
	switch_state_t oldVal = _switchValue;

	//! Relay on when value=100, relay off when value=0
	if (switchState > 99) {
		_relayOn();
	} else {
		_relayOff();
	}
	//! Pwm when value is 1-99, else pwm off
	if (switchState > 0 && switchState < 100) {
		_setPwm(switchState);
	}
	else {
		_setPwm(0);
	}

	if (_delayedSwitchPending) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGi("clear delayed switch state");
#endif

		Timer::getInstance().stop(_switchTimerId);
		_delayedSwitchPending = false;
	}

	updateSwitchState(oldVal);
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
			handleDelayed(p_item->switchState, p_item->timeout);
		}
	}

}
#endif


void Switch::handleDelayed(uint8_t switchState, uint16_t delay) {

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("trigger delayed switch state: %d, delay: %d", switchState, delay);
#endif

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


void Switch::timedSwitch() {
#ifdef PRINT_SWITCH_VERBOSE
	LOGi("timed switch, set");
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
	_switchValue.pwm_state = value;
#ifndef PWM_DISABLE
	PWM::getInstance().setValue(0, value);
#endif
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
