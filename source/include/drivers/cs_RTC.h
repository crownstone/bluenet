/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <stdint.h>


/**
 * implementation of some inline functions is deferred to mock .cpp file on host.
 */
#if !defined HOST_TARGET
#define INLINE inline
#else
#define INLINE
#endif

/**
 * Clock frequency of the RTC timer.
 *
 * TODO: Collect all macros in general file.
 */
#define RTC_CLOCK_FREQ 32768

/** Maximum value of the RTC counter. */
#define MAX_RTC_COUNTER_VAL 0x00FFFFFF

/**
 * Wrapper class for RTC functions.
 *
 * if NRF51_USE_SOFTDEVICE==1 uses the RTC0 clock managed by the softdevice,
 * otherwise uses the RTC1 clock used by the app timer
 * With the current clock frequency and max counter val, the RTC overflows every 512s.
 *
 */
class RTC {

private:
	RTC()           = default;
	RTC(RTC const&) = delete;
	void operator=(RTC const&) = delete;

public:
	/**
	 * return number of ticks
	 */
	INLINE static uint32_t getCount();

	/**
	 * Return time in ms, given time in ticks
	 */
	INLINE static uint32_t ticksToMs(uint32_t ticks);

	/**
	 * Return time in ticks, given time in ms
	 * Make sure time in ms is not too large! (limit is 512,000 ms with current frequency)
	 */
	INLINE static uint32_t msToTicks(uint32_t ms);

	/**
	 * return difference between two tick counter values
	 */
	inline static uint32_t difference(uint32_t ticksTo, uint32_t ticksFrom);
	/**
	 * return difference between two tick counter values in ms.
	 */
	inline static uint32_t differenceMs(uint32_t ticksTo, uint32_t ticksFrom);

	/**
	 * Returns the number of milliseconds passed since the given tick counter value.
	 */
	inline static uint32_t msPassedSince(uint32_t ticksFrom);

	/**
	 * return current clock in ms
	 */
	inline static uint32_t now();
};

// ----------------- inline implementation details -----------------

// -----------------------------
// platform independent  methods
// -----------------------------

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

#if !defined HOST_TARGET

// -----------------------------
// platform == nordic
// -----------------------------

#include <app_util.h>
#include <cs_Nordic.h>

uint32_t RTC::getCount() {
	return NRF_RTC0->COUNTER;
}

uint32_t RTC::ticksToMs(uint32_t ticks) {
	// To increase precision: multiply both sides of the division by a large number (multiple of 2 for speed).
	// Cast them to uint64_t to prevent overflow.
	// TODO: We have hardware floating points now. Get rid of ROUNDED_DIV macro, etc.
	return (uint32_t)ROUNDED_DIV(
			(uint64_t)65536 * ticks, (uint64_t)65536 * RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000);
}

uint32_t RTC::msToTicks(uint32_t ms) {
	// Order of multiplication and division is important:
	// because it shouldn't lose too much precision, but also not overflow
	return (uint64_t)ms * RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000;
}

#else

// -----------------------------
// platform == host
// -----------------------------

// implementation deferred to mock .cpp file.

#endif  // !defined HOST_TARGET
