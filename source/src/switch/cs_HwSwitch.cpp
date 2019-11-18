/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_HwSwitch.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_PWM.h>

#define HWSWITCH_LOG LOGd

void HwSwitch::relayOn() {
	HWSWITCH_LOG("%s", __func__);
	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOn);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOn);
	}
}

void HwSwitch::relayOff() {
	HWSWITCH_LOG("%s", __func__);
	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOff);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOff);
	}
}

void HwSwitch::enableDimmer(){
	HWSWITCH_LOG("%s", __func__);
	nrf_gpio_pin_set(_pinEnableDimmer);
}

void HwSwitch::disableDimmer(){
	HWSWITCH_LOG("%s", __func__);
	// Warning: untested.
	nrf_gpio_pin_clear(_pinEnableDimmer);
}

void HwSwitch::setRelay(bool is_on){
	if(is_on){
		relayOn();
	} else {
		relayOff();
	}
}

void HwSwitch::setDimmerPower(bool is_on){
	if(is_on){
		enableDimmer();
	} else {
		disableDimmer();
	}
}

void HwSwitch::setIntensity(uint8_t value) {
	HWSWITCH_LOG("%s (%d)", __func__, value);
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
HwSwitch::HwSwitch(const boards_config_t& board, uint32_t pwmPeriod, uint16_t relayHighDuration) {
	pwm_config_t pwmConfig;
	pwmConfig.channelCount = 1;
	pwmConfig.period_us = pwmPeriod;
	pwmConfig.channels[0].pin = board.pinGpioPwm;
	pwmConfig.channels[0].inverted = board.flags.pwmInverted;

	PWM::getInstance().init(pwmConfig);

	switch(board.hardwareBoard){
		case PCA10036:
		case PCA10040:
			PWM::getInstance().start(false);
			break;
		default:
			PWM::getInstance().start(true);
			break;
	}

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

