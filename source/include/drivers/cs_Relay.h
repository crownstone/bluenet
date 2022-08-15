/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Boards.h>
#include <common/cs_Types.h>

/**
 * Class that provides a bi-stable relay.
 * - Controls GPIO.
 * - Powers the relay long enough for it to toggle.
 * - TODO: toggles relay in a non blocking / asynchronous way.
 */
class Relay {
public:
	void init(const boards_config_t& board);

	/**
	 * Returns true when this board has a relay.
	 */
	bool hasRelay();

	/**
	 * Set relay on or off.
	 *
	 * @param[in] value      True to set the relay on.
	 * @return               True on success.
	 */
	bool set(bool value);

private:
	bool _initialized = false;
	bool _hasRelay;
	uint8_t _pinRelayOn;
	uint8_t _pinRelayOff;
	TYPIFY(CONFIG_RELAY_HIGH_DURATION) _relayHighDurationMs;

	uint8_t _pinRelayDebug;
	bool _ledInverted;

	bool turnOn();
	bool turnOff();
};
