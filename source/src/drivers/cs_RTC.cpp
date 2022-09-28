/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 22, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <app_util.h>
#include <cs_Nordic.h>
#include <drivers/cs_RTC.h>

uint32_t RTC::getCount() {
	return NRF_RTC0->COUNTER;
}

uint32_t RTC::ticksToMs(uint32_t ticks) {
	// To increase precision: multiply both sides of the division by a large factor (multiple of 2 for speed).
	// Cast them to uint64_t to prevent overflow.
	// TODO: We have hardware floating points now. Get rid of ROUNDED_DIV macro, etc.
	uint64_t largeFactor = 1 << 16;
	return (uint32_t)ROUNDED_DIV(
			(uint64_t)largeFactor * ticks, (uint64_t)largeFactor * RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000);
}

uint32_t RTC::msToTicks(uint32_t ms) {
	// Order of multiplication and division is important:
	// because it shouldn't lose too much precision, but also not overflow
	return (uint64_t)ms * RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000;
}

// Platform independent methods

uint32_t RTC::difference(uint32_t ticksTo, uint32_t ticksFrom) {
	return ((ticksTo - ticksFrom) & MAX_RTC_COUNTER_VAL);
}

uint32_t RTC::differenceMs(uint32_t ticksTo, uint32_t ticksFrom) {
	return ticksToMs(difference(ticksTo, ticksFrom));
}

uint32_t RTC::msPassedSince(uint32_t ticksFrom) {
	return ticksToMs(difference(getCount(), ticksFrom));
}

uint32_t RTC::now() {
	return ticksToMs(getCount());
}
