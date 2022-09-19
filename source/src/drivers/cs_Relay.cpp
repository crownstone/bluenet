/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <drivers/cs_Relay.h>
#include <logging/cs_Logger.h>
#include <nrf_delay.h>
#include <nrf_gpio.h>
#include <storage/cs_State.h>
#include <test/cs_Test.h>
#include <util/cs_Error.h>

void Relay::init(const boards_config_t& board) {
	_initialized = true;

	LOGd("relay init 1");
	State::getInstance().get(CS_TYPE::CONFIG_RELAY_HIGH_DURATION, &_relayHighDurationMs, sizeof(_relayHighDurationMs));
	_hasRelay    = ((board.pinRelayOn != PIN_NONE) && (board.pinRelayOff != PIN_NONE));
	_pinRelayOn  = board.pinRelayOn;
	_pinRelayOff = board.pinRelayOff;
	LOGd("relay init 2");
	nrf_gpio_cfg_output(_pinRelayOff);
	LOGd("relay init 3");
	nrf_gpio_pin_clear(_pinRelayOff);
	LOGd("relay init 4");
	nrf_gpio_cfg_output(_pinRelayOn);
	nrf_gpio_pin_clear(_pinRelayOn);
	LOGd("relay init 5");
	_pinRelayDebug = board.pinRelayDebug;
	_ledInverted   = board.flags.ledInverted;
	if (_pinRelayDebug != PIN_NONE) {
		nrf_gpio_cfg_output(_pinRelayDebug);
		nrf_gpio_pin_clear(_pinRelayDebug);
	}
	LOGd("init duration=%u ms", _relayHighDurationMs);
}

bool Relay::hasRelay() {
	return _hasRelay;
}

bool Relay::set(bool value) {
	assert(_initialized == true, "Not initialized");

	TEST_PUSH_EXPR_B(this, "on", value);

	if (_pinRelayDebug != PIN_NONE) {
		bool debugPinValue = value;
		if (_ledInverted) {
			debugPinValue = !debugPinValue;
		}
		if (debugPinValue) {
			nrf_gpio_pin_set(_pinRelayDebug);
		}
		else {
			nrf_gpio_pin_clear(_pinRelayDebug);
		}
	}

	if (value) {
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
