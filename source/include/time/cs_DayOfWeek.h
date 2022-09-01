/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <util/cs_Math.h>

#include <cstdint>

enum class DayOfWeek : uint8_t {
	Sunday    = 1 << 0,
	Monday    = 1 << 1,
	Tuesday   = 1 << 2,
	Wednesday = 1 << 3,
	Thursday  = 1 << 4,
	Friday    = 1 << 5,
	Saturday  = 1 << 6,
};

typedef uint8_t DayOfWeekBitMask;

inline uint8_t dayNumber(DayOfWeek day) {
	for(auto i{0}; i < 7; i++) {
		if (day == DayOfWeek(1<<i)) {
			return i;
		}
	}
	return 0xff;
}

inline DayOfWeek operator+(DayOfWeek day, int offset) {
	offset               = CsMath::mod(offset, 7);
	uint16_t shifted_day = static_cast<uint8_t>(day) << offset;

	if (shifted_day & 0b01111111) {
		// no overflow occured, simply return it.
		return DayOfWeek(static_cast<uint8_t>(shifted_day & 0b01111111));
	}
	else {
		// passed week boundary by shifting so shift it back by 7.
		return DayOfWeek(static_cast<uint8_t>((shifted_day >> 7) & 0b01111111));
	}
}

inline DayOfWeek operator-(DayOfWeek day, int offset) {
	return day + -offset;
}
