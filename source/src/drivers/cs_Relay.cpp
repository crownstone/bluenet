/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <drivers/cs_Relay.h>
#include <drivers/cs_Serial.h>

Relay::Relay(const boards_config_t& board, uint8_t relayHighDurationMs) :
	_hasRelay(board.flags.hasRelay),
	_pinRelayOn(board.pinGpioRelayOn),
	_pinRelayOff(board.pinGpioRelayOff),
	_relayHighDurationMs(relayHighDurationMs)
{
}

bool Relay::set(bool on) {
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
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOn);
	}
	return _hasRelay;
}

bool Relay::turnOff() {
	LOGd("turnOff");
	if (_hasRelay) {
		nrf_gpio_pin_set(_pinRelayOff);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOff);
	}
	return _hasRelay;
}
