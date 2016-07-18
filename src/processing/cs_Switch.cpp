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

//#define PRINT_SWITCH_VERBOSE

//#define PWM_DISABLE
#define HAS_RELAY true

Switch::Switch()
{
	LOGd(FMT_CREATE, "Switch");
}

void Switch::init() {

#ifndef PWM_DISABLE
	PWM& pwm = PWM::getInstance();
	pwm.init(PWM::config1Ch(1600L, PIN_GPIO_SWITCH));
#else
	nrf_gpio_cfg_output(PIN_GPIO_SWITCH);
#ifdef SWITCH_INVERSED
	nrf_gpio_pin_set(PIN_GPIO_SWITCH);
#else
	nrf_gpio_pin_clear(PIN_GPIO_SWITCH);
#endif
#endif

	State::getInstance().get(STATE_SWITCH_STATE, &_switchValue, 1);

	// [23.06.16] overwrites stored value, so we can't restore old switch state
	//	setValue(0);

#if HAS_RELAY
	nrf_gpio_cfg_output(PIN_GPIO_RELAY_OFF);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_OFF);
	nrf_gpio_cfg_output(PIN_GPIO_RELAY_ON);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_ON);
#endif
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

void Switch::setValue(switch_state_t value) {
	_switchValue = value;
}

switch_state_t Switch::getValue() {
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
	return _switchValue.relay_state != 0;
#else
	return _switchValue != 0;
#endif
}

void Switch::relayOn() {
	uint16_t relayHighDuration;
	Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &relayHighDuration);

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("trigger relay on pin for %d ms", relayHighDuration);
#endif

#if HAS_RELAY
	nrf_gpio_pin_set(PIN_GPIO_RELAY_ON);
	nrf_delay_ms(relayHighDuration);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_ON);

#ifdef EXTENDED_SWITCH_STATE
	_switchValue.relay_state = 1;
#else
	_switchValue = 255;
#endif
	updateSwitchState();
#endif
}

void Switch::relayOff() {
	uint16_t relayHighDuration;
	Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &relayHighDuration);

#ifdef PRINT_SWITCH_VERBOSE
	LOGi("trigger relay off pin for %d ms", relayHighDuration);
#endif

#if HAS_RELAY
	nrf_gpio_pin_set(PIN_GPIO_RELAY_OFF);
	nrf_delay_ms(relayHighDuration);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_OFF);

#ifdef EXTENDED_SWITCH_STATE
	_switchValue.relay_state = 0;
#else
	_switchValue = 0;
#endif
	updateSwitchState();
#endif
}

void Switch::turnOn() {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("Turn ON");
#endif

#if HAS_RELAY
	relayOn();
#else
	pwmOn();
#endif
}

void Switch::turnOff() {

#ifdef PRINT_SWITCH_VERBOSE
	LOGd("Turn OFF");
#endif

#if HAS_RELAY
	relayOff();
#else
	pwmOff();
#endif
}
