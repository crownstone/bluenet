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
	_hasRelay(false), _pinRelayOn(0), _pinRelayOff(0), _delayedSwitchState(NULL)
{
	LOGd(FMT_CREATE, "Switch");

#if (NORDIC_SDK_VERSION >= 11)
	_switchTimerData = { {0} };
	_switchTimerId = &_switchTimerData;
#else
	_switchTimerId(UINT32_MAX)
#endif
}

void Switch::init(boards_config_t* board) {

#ifndef PWM_DISABLE
	PWM& pwm = PWM::getInstance();
//	pwm.init(PWM::config1Ch(1600L, board->pinGpioPwm)); //! 625 Hz
//	pwm.init(pwm.config1Ch(20000L, board->pinGpioPwm)); //! 50 Hz
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

	// [23.06.16] overwrites stored value, so we can't restore old switch state
	//	setValue(0);

	_hasRelay = board->flags.hasRelay;
	if (_hasRelay) {
		_pinRelayOff = board->pinGpioRelayOff;
		_pinRelayOn = board->pinGpioRelayOn;

		nrf_gpio_cfg_output(_pinRelayOff);
		nrf_gpio_pin_clear(_pinRelayOff);
		nrf_gpio_cfg_output(_pinRelayOn);
		nrf_gpio_pin_clear(_pinRelayOn);
	}

	// For now: just turn it off on init, for safety.
//	State::getInstance().get(STATE_SWITCH_STATE, &_switchValue, 1);
//	LOGd("switch state: pwm=%u relay=%u", _switchValue.pwm_state, _switchValue.relay_state);
	pwmOff();

	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_switchTimerId, (app_timer_timeout_handler_t)Switch::staticTimedSwitch);
}

void Switch::start() {

}

void Switch::pwmOff() {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("PWM OFF");
#endif

	setPwm(0);
}

void Switch::pwmOn() {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("PWM ON");
#endif

	setPwm(255);
}

void Switch::dim(uint8_t value) {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("PWM %d", value);
#endif

	setPwm(value);
}

void Switch::updateSwitchState() {
	State::getInstance().set(STATE_SWITCH_STATE, &_switchValue, sizeof(switch_state_t));
}

void Switch::setPwm(uint8_t value) {
#ifndef PWM_DISABLE
	if (value > 0 && !allowPwmOn()) {
		LOGd("Don't turn on pwm");
		return;
	}
#ifdef EXTENDED_SWITCH_STATE
	_switchValue.pwm_state = value;
#else
	_switchValue = value;
#endif
	PWM::getInstance().setValue(0, value);
	updateSwitchState();
#else
	LOGe("PWM disabled!!");
#endif
}

//void Switch::setValue(switch_state_t value) {
//	_switchValue = value;
//}

switch_state_t Switch::getSwitchState() {
#ifdef PRINT_SWITCH_VERBOSE
	LOGd(FMT_GET_INT_VAL, "Switch state", _switchValue);
#endif
	return _switchValue;
}

uint8_t Switch::getPwm() {
#ifdef EXTENDED_SWITCH_STATE
	return _switchValue.pwm_state;
#else
	return _switchValue;
#endif
}

bool Switch::getRelayState() {
#ifdef EXTENDED_SWITCH_STATE
	return _switchValue.relay_state;
#else
	return _switchValue != 0;
#endif
}

void Switch::relayOn() {
	LOGd("relayOn");
//	LOGd("relayOn %u", _nextRelayVal);
//	if (Nrf51822BluetoothStack::getInstance().isScanning()) {
//		if (_nextRelayVal != SWITCH_NEXT_RELAY_VAL_NONE) {
//			_nextRelayVal = SWITCH_NEXT_RELAY_VAL_ON;
//			return;
//		}
//		//! Try again later
//		LOGd("Currently scanning, try again later");
//		_nextRelayVal = SWITCH_NEXT_RELAY_VAL_ON;
//		Timer::getInstance().start(_relayTimerId, MS_TO_TICKS(RELAY_DELAY), this);
//		Scanner::getInstance().manualStopScan(); // TODO: stop scanning via stack, let stack send out an event?
//		return;
//	}
//	_nextRelayVal = SWITCH_NEXT_RELAY_VAL_NONE;

	// WHY READ IT EVERY TIME FROM SETTINGS AND NOT READ IT ONCE AT INIT AND USE THAT VALUE?
	uint16_t relayHighDuration;
	Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &relayHighDuration);

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("trigger relay on pin for %d ms", relayHighDuration);
#endif

	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOn);
		nrf_delay_ms(relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOn);
	}

#ifdef EXTENDED_SWITCH_STATE
	_switchValue.relay_state = true;
#else
	_switchValue = 255;
#endif
	updateSwitchState();
}

void Switch::relayOff() {
	LOGd("relayOff");
//	LOGd("relayOff %u", _nextRelayVal);
//	if (Nrf51822BluetoothStack::getInstance().isScanning()) {
//		if (_nextRelayVal != SWITCH_NEXT_RELAY_VAL_NONE) {
//			_nextRelayVal = SWITCH_NEXT_RELAY_VAL_OFF;
//			return;
//		}
//		//! Try again later
//		LOGd("Currently scanning, try again later");
//		_nextRelayVal = SWITCH_NEXT_RELAY_VAL_OFF;
//		Timer::getInstance().start(_relayTimerId, MS_TO_TICKS(RELAY_DELAY), this);
////		Scanner::getInstance().manualStopScan(); // TODO: stop scanning via stack, let stack send out an event?
//		return;
//	}
//	_nextRelayVal = SWITCH_NEXT_RELAY_VAL_NONE;

	// WHY READ IT EVERY TIME FROM SETTINGS AND NOT READ IT ONCE AT INIT AND USE THAT VALUE?
	uint16_t relayHighDuration;
	Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &relayHighDuration);

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("trigger relay off pin for %d ms", relayHighDuration);
#endif

	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOff);
		nrf_delay_ms(relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOff);
	}

#ifdef EXTENDED_SWITCH_STATE
	_switchValue.relay_state = false;
#else
	_switchValue = 0;
#endif
	updateSwitchState();
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
		switch_state_t* pSwitchState = (switch_state_t*)&p_item->switchState;
		if (p_item->timeout == 0) {
			handle(pSwitchState);
		} else {
			handleDelayed(pSwitchState, p_item->timeout);
		}
	}

}
#endif

void Switch::handle(switch_state_t* switchState) {

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("trigger switch state: %d", *switchState);
#endif

	if (switchState->relay_state) {
		relayOn();
	} else {
		relayOff();
	}
	setPwm(switchState->pwm_state);

	if (_delayedSwitchState != NULL) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGi("clear delayed switch state");
#endif

		Timer::getInstance().stop(_switchTimerId);
		free(_delayedSwitchState);
		_delayedSwitchState = NULL;
	}
}

void Switch::handleDelayed(switch_state_t* switchState, uint16_t delay) {

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("trigger delayed switch state: %d, delay: %d", *switchState, delay);
#endif

	if (_delayedSwitchState != NULL) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGi("clear existing delayed switch state");
#endif
		Timer::getInstance().stop(_switchTimerId);
		free(_delayedSwitchState);
	}

	_delayedSwitchState = new switch_state_t;
	memcpy(_delayedSwitchState, switchState, sizeof(switch_state_t));
	Timer::getInstance().start(_switchTimerId, MS_TO_TICKS(delay * 1000), this);
}

void Switch::timedSwitch() {
#ifdef PRINT_SWITCH_VERBOSE
	LOGi("timed switch, set");
#endif

	if (_delayedSwitchState != NULL) {
		handle(_delayedSwitchState);
	}
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

void Switch::forcePwmOff() {
	LOGw("forcePwmOff");
	pwmOff();
	EventDispatcher::getInstance().dispatch(EVT_PWM_FORCED_OFF);
}

void Switch::forceSwitchOff() {
	LOGw("forceSwitchOff");
	pwmOff();
	relayOff();
	EventDispatcher::getInstance().dispatch(EVT_SWITCH_FORCED_OFF);
}

bool Switch::allowPwmOn() {
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, &stateErrors, sizeof(state_errors_t));
//	uint32_t* errorsint = (uint32_t*)&stateErrors;
//	LOGd("state errors: %u", *errorsint);
//	return !(stateErrors.chipTemp || stateErrors.overCurrent || stateErrors.overCurrentPwm || stateErrors.pwmTemp);
	return !(stateErrors.chipTemp || stateErrors.overCurrent || stateErrors.pwmTemp);
}

bool Switch::allowSwitchOn() {
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, &stateErrors, sizeof(state_errors_t));
	return (stateErrors.chipTemp || stateErrors.overCurrent);
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
		forcePwmOff();
		break;
	}
	case EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
	case EVT_CHIP_TEMP_ABOVE_THRESHOLD: {
		forceSwitchOff();
		break;
	}
	}
}
