/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 25, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <cfg/cs_DeviceTypes.h>

/**
 * The Guidestone has pads for pin 9, 10, 25, 26, 27, SWDIO, SWDCLK, GND, VDD.
 */
void asGuidestone(boards_config_t* config) {
	config->pinRx                              = 25;
	config->pinTx                              = 26;
	config->deviceType                         = DEVICE_GUIDESTONE;
	config->minTxPower                         = -20;
	config->scanWindowUs                       = config->scanIntervalUs;
}
