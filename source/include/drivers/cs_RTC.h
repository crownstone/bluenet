/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <stdint.h>

#include <test/cs_TestAccess.h>

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
	static uint32_t getCount();

	/**
	 * Return time in ms, given time in ticks
	 */
	static uint32_t ticksToMs(uint32_t ticks);

	/**
	 * Return time in ticks, given time in ms
	 * Make sure time in ms is not too large! (limit is 512,000 ms with current frequency)
	 */
	static uint32_t msToTicks(uint32_t ms);

	/**
	 * return difference between two tick counter values
	 */
	static uint32_t difference(uint32_t ticksTo, uint32_t ticksFrom);
	/**
	 * return difference between two tick counter values in ms.
	 */
	static uint32_t differenceMs(uint32_t ticksTo, uint32_t ticksFrom);

	/**
	 * Returns the number of milliseconds passed since the given tick counter value.
	 */
	static uint32_t msPassedSince(uint32_t ticksFrom);

	/**
	 * return current clock in ms
	 */
	inline static uint32_t now();

#ifdef HOST_TARGET
	/**
	 * allows for fast-forward/rewinding rtc time during tests.
	 */
	static void offsetMs(int ms);
	static int _offsetMs;

	/**
	 * Acquires a start time to base the RTC tick values off of.
	 */
	static void start();

#endif
};
