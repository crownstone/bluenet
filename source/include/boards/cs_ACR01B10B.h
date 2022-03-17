/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 25, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <cfg/cs_DeviceTypes.h>
#include <boards/cs_ACR01B10D.h>

void asACR01B10B(boards_config_t* config) {
	// The ACR01B10B is similar to ACR01B10D, but lacks the dimmer enable pin.
	asACR01B10D(config);
	config->flags.usesNfcPins                  = false;
	config->pinEnableDimmer                    = PIN_NONE;
	config->flags.canDimOnWarmBoot             = false;
	config->flags.dimmerOnWhenPinsFloat        = true;

//	// With patch 2:
//	config->pinEnableDimmer                    = 15;
//	config->flags.canDimOnWarmBoot             = true; // TODO: can it?
//	config->flags.dimmerOnWhenPinsFloat        = false;
}
