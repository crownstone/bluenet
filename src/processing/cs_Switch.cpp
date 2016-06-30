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

#define PRINT_VERBOSE

Switch::Switch() :
		_switchValue(0)
{
	LOGd(FMT_CREATE, "Switch");
}

void Switch::init() {

	PWM& pwm = PWM::getInstance();
	pwm.init(PWM::config1Ch(1600L, PIN_GPIO_SWITCH));

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

#ifdef PRINT_VERBOSE
	LOGd("PWM OFF");
#endif

	setValue(0);
}

void Switch::pwmOn() {

#ifdef PRINT_VERBOSE
	LOGd("PWM ON");
#endif

	setValue(255);
}

void Switch::dim(uint8_t value) {

#ifdef PRINT_VERBOSE
	LOGd("PWM %d", value);
#endif

	setValue(value);
}

void Switch::setValue(uint8_t value) {
	_switchValue = value;
	PWM::getInstance().setValue(0, value);

	State::getInstance().set(STATE_SWITCH_STATE, value);
}

uint8_t Switch::getValue() {

#ifdef PRINT_VERBOSE
	LOGd(FMT_GET_INT_VAL, "Switch state", _switchValue);
#endif

	return _switchValue;
}

void Switch::relayOn() {
	uint16_t relayHighDuration;
	Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &relayHighDuration);

#ifdef PRINT_VERBOSE
	LOGd("trigger relay on pin for %d ms", relayHighDuration);
#endif

#if HAS_RELAY
	nrf_gpio_pin_set(PIN_GPIO_RELAY_ON);
	nrf_delay_ms(relayHighDuration);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_ON);

	State::getInstance().set(STATE_SWITCH_STATE, (uint8_t)255);
#endif
}

void Switch::relayOff() {
	uint16_t relayHighDuration;
	Settings::getInstance().get(CONFIG_RELAY_HIGH_DURATION, &relayHighDuration);

#ifdef PRINT_VERBOSE
	LOGi("trigger relay off pin for %d ms", relayHighDuration);
#endif

#if HAS_RELAY
	nrf_gpio_pin_set(PIN_GPIO_RELAY_OFF);
	nrf_delay_ms(relayHighDuration);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_OFF);

	State::getInstance().set(STATE_SWITCH_STATE, (uint8_t)0);
#endif
}

void Switch::turnOn() {

#ifdef PRINT_VERBOSE
	LOGd("Turn ON");
#endif

#if HAS_RELAY
	relayOn();
#else
	pwmOn();
#endif
}

void Switch::turnOff() {

#ifdef PRINT_VERBOSE
	LOGd("Turn OFF");
#endif

#if HAS_RELAY
	relayOff();
#else
	pwmOff();
#endif
}
