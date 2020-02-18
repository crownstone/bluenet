/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <drivers/cs_Relay.h>
#include <drivers/cs_Serial.h>
#include <storage/cs_State.h>
#include <util/cs_Error.h>

#include <test/cs_Test.h>

void Relay::init(const boards_config_t& board) {
	_initialized = true;

	State::getInstance().get(CS_TYPE::CONFIG_RELAY_HIGH_DURATION, &_relayHighDurationMs, sizeof(_relayHighDurationMs));

	_hasRelay = board.flags.hasRelay;
	_pinRelayOn = board.pinGpioRelayOn;
	_pinRelayOff =board.pinGpioRelayOff;

	nrf_gpio_cfg_output(_pinRelayOff);
	nrf_gpio_pin_clear(_pinRelayOff);
	nrf_gpio_cfg_output(_pinRelayOn);
	nrf_gpio_pin_clear(_pinRelayOn);

	LOGd("init duration=%u ms", _relayHighDurationMs);
}

bool Relay::set(bool on) {
	assert(_initialized == true, "Not initialized");

	TEST_PUSH_EXPR_B(this,"on",on);

	if (on) {
		return turnOn();
	}
	else {
		return turnOff();
	}
}

bool Relay::turnOn() {
	LOGd("turnOn");
	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOn);
		nrf_delay_ms(_relayHighDurationMs);
		nrf_gpio_pin_clear(_pinRelayOn);
	}
	return _hasRelay;
}

bool Relay::turnOff() {
	LOGd("turnOff");
	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOff);
		nrf_delay_ms(_relayHighDurationMs);
		nrf_gpio_pin_clear(_pinRelayOff);
	}
	return _hasRelay;
}
