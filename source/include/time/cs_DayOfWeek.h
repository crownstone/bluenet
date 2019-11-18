/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cstddef>

typedef uint8_t DayOfWeekBitMask;

const constexpr DayOfWeekBitMask Monday     = 1 << 0;
const constexpr DayOfWeekBitMask Tuesday    = 1 << 1;
const constexpr DayOfWeekBitMask Wednesday  = 1 << 2;
const constexpr DayOfWeekBitMask Thursday   = 1 << 3;
const constexpr DayOfWeekBitMask Friday     = 1 << 4;
const constexpr DayOfWeekBitMask Saturday   = 1 << 5;
const constexpr DayOfWeekBitMask Sunday     = 1 << 6;