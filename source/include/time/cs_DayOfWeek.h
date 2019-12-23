/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstddef>

enum class DayOfWeek : uint8_t { 
    Sunday      = 1 << 0
    Monday      = 1 << 1,
    Tuesday     = 1 << 2,
    Wednesday   = 1 << 3,
    Thursday    = 1 << 4,
    Friday      = 1 << 5,
    Saturday    = 1 << 6,
};

typedef uint8_t DayOfWeekBitMask;