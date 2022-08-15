/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 23, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <stdint.h>

namespace Msws {

/**
 * Wrapper class for internal state of the Msws generator.
 */
class State {
public:
	uint64_t x;
	uint64_t w;
	uint64_t s;
};

}  // namespace Msws
