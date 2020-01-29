/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Boards.h>

class Relay {
public:
	Relay(const boards_config_t& board, uint8_t relayHighDurationMs);
	bool set(bool on);

private:
	bool _hasRelay;
	uint8_t _pinRelayOn;
	uint8_t _pinRelayOff;
    uint8_t _relayHighDurationMs;

    bool turnOn();
    bool turnOff();
};


