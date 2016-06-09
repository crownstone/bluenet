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

Switch::Switch() :
		_switchValue(0)
{
	LOGd("creating switch");
}

void Switch::init() {

	PWM& pwm = PWM::getInstance();
	pwm.init(PWM::config1Ch(1600L, PIN_GPIO_SWITCH));

#if HAS_RELAY
	nrf_gpio_cfg_output(PIN_GPIO_RELAY_OFF);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_OFF);
	nrf_gpio_cfg_output(PIN_GPIO_RELAY_ON);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_ON);
#endif
}

void Switch::turnOff() {
	setValue(0);
}

void Switch::turnOn() {
	setValue(255);
}

void Switch::dim(uint8_t value) {
	setValue(value);
}

void Switch::setValue(uint8_t value) {
	_switchValue = value;
	PWM::getInstance().setValue(0, value);

	if (value) {
		EventDispatcher::getInstance().dispatch(EVT_POWER_ON, &_switchValue, 1);
	} else {
		EventDispatcher::getInstance().dispatch(EVT_POWER_OFF, &_switchValue, 1);
	}

	State::getInstance().set(STATE_SWITCH_STATE, value);
}

uint8_t Switch::getValue() {
	return _switchValue;
}

void Switch::relayOn() {
	LOGi("trigger relay on pin for %d ms", RELAY_HIGH_DURATION);
#if HAS_RELAY
	nrf_gpio_pin_set(PIN_GPIO_RELAY_ON);
	nrf_delay_ms(RELAY_HIGH_DURATION);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_ON);

	// todo: update switch state?
#endif
}

void Switch::relayOff() {
	LOGi("trigger relay off pin for %d ms", RELAY_HIGH_DURATION);
#if HAS_RELAY
	nrf_gpio_pin_set(PIN_GPIO_RELAY_OFF);
	nrf_delay_ms(RELAY_HIGH_DURATION);
	nrf_gpio_pin_clear(PIN_GPIO_RELAY_OFF);

	// todo: update switch state?
#endif
}
