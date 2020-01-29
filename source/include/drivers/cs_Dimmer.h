/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Boards.h>

class Dimmer {
public:
	Dimmer(const boards_config_t& board, uint32_t pwmPeriodUs, bool startDimmerOnZeroCrossing);
	void start();

	bool set(uint8_t intensity);

private:
	uint32_t hardwareBoard;
	bool started = false;
	bool enabled = false;

	void enable();
};


