/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 25, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <cfg/cs_DeviceTypes.h>

void asUsbDongle(boards_config_t* config) {
	config->pinRx            = 8;
	config->pinTx            = 6;
	config->flags.enableUart = true;
	config->deviceType       = DEVICE_CROWNSTONE_USB;
	config->minTxPower       = -40;
	config->scanWindowUs     = config->scanIntervalUs;
}
