/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_HwSwitch.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_PWM.h>

void HwSwitch::relayOn() {
	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOn);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOn);
	}
}

void HwSwitch::relayOff() {
	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOff);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOff);
	}
}

void HwSwitch::setPwm(uint8_t value) {
	PWM::getInstance().setValue(0, value);
}

/**
 * Initialize the "switch". The switch class encapsulates both the relay for switching high-power loads and the
 * IGBTs for very fast switching of low-power loads. The latter enables all types of dimming. The following types
 * of dimming exists:
 *   + pulse width modulation (PWM)
 *   + leading edge dimming
 *   + trailing edge dimming
 */
void HwSwitch::init(const boards_config_t& board, uint32_t pwmPeriod, uint16_t relayHighDuration) {
	pwm_config_t pwmConfig;
	pwmConfig.channelCount = 1;
	pwmConfig.period_us = pwmPeriod;
	pwmConfig.channels[0].pin = board.pinGpioPwm;
	pwmConfig.channels[0].inverted = board.flags.pwmInverted;
	PWM::getInstance().init(pwmConfig);

	_pinEnableDimmer = board.pinGpioEnablePwm;
	_hasRelay = board.flags.hasRelay;
    _relayHighDuration = relayHighDuration;
	if (_hasRelay) {
		_pinRelayOff = board.pinGpioRelayOff;
		_pinRelayOn = board.pinGpioRelayOn;

		nrf_gpio_cfg_output(_pinRelayOff);
		nrf_gpio_pin_clear(_pinRelayOff);
		nrf_gpio_cfg_output(_pinRelayOn);
		nrf_gpio_pin_clear(_pinRelayOn);
	}
}

void HwSwitch::enableDimmer(){
	nrf_gpio_pin_set(_pinEnableDimmer);
}