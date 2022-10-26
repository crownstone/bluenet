/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <stdint.h>

/**
 * Interface class that defines what a switch can do.
 */
class ISwitch {
	virtual void setRelay(bool is_on)        = 0;
	virtual void setDimmerPower(bool is_on)  = 0;
	virtual void setIntensity(uint8_t value) = 0;
	virtual ~ISwitch() = default;
};
