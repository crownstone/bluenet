/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 19, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

//#include <ble/cs_Nordic.h>
#include <drivers/cs_Relay.h>
#include <logging/cs_Logger.h>
//#include <storage/cs_State.h>
//#include <test/cs_Test.h>
//#include <util/cs_Error.h>
//#include <nrf_gpio.h>
//#include <nrf_delay.h>

#define LogRelayDebug LOGvv

static bool isRelayPinOn = false;
static bool isDebugPinOn = false;

void Relay::init(const boards_config_t& board) {
	_initialized = true;
}

bool Relay::hasRelay() {
	return _hasRelay;
}

bool Relay::set(bool value) {
	assert(_initialized == true, "Not initialized");


	if (_pinRelayDebug != PIN_NONE) {
		bool debugPinValue = value;
		if (_ledInverted) {
			debugPinValue = !debugPinValue;
		}
		if (debugPinValue) {
			isDebugPinOn = true;
		}
		else {
			isDebugPinOn = false;
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
		isRelayPinOn = true;
	}
	return _hasRelay;
}

bool Relay::turnOff() {
	LOGd("turnOff");
	if (_hasRelay) {
		isRelayPinOn = false;
	}
	return _hasRelay;
}
