/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <drivers/cs_Timer.h>

/**
 * Clock frequency of the RTC timer.
 *
 * TODO: Collect all macros in general file.
 */
#define RTC_CLOCK_FREQ          32768

/** Maximum value of the RTC counter. */
#define MAX_RTC_COUNTER_VAL     0x00FFFFFF


/** Wrapper class for RTC functions.
 *
 * if NRF51_USE_SOFTDEVICE==1 uses the RTC0 clock
 * managed by the softdevice
 * otherwise uses the RTC1 clock used by the app timer
 *
 * With the current clock frequency and max counter val, the RTC overflows every 512s.
 *
 * TODO: Implementation should be in a .cpp file
 * TODO: Get rid of all these static functions.
 * TODO: We have hardware floating points now. Get rid of ROUNDED_DIV macro, etc.
 */
class RTC {

private:

	RTC() {};
	RTC(RTC const&); //! singleton, deny implementation
	void operator=(RTC const &); //! singleton, deny implementation

public:

	//! return number of ticks
	inline static uint32_t getCount() {
		return NRF_RTC0->COUNTER;
	}


	//! return difference between two tick counter values
	inline static uint32_t difference(uint32_t ticksTo, uint32_t ticksFrom) {
		return ((ticksTo - ticksFrom) & MAX_RTC_COUNTER_VAL);
	}

	/**
	 * Returns the number of miliseconds passed since the given tick_count.
	 *
	 * E.g.
	 *
	 * uint32_t stamp = RTC::getCount();
	 * ... 3 seconds pass ...
	 * printf("%d miliseconds passed", RTC::msPassedSince(stamp);
	 */
	inline static uint32_t msPassedSince(uint32_t tick_count){
		return ticksToMs(difference(getCount(),tick_count));
	}

	//! return current clock in ms
	inline static uint32_t now() {
		return ticksToMs(getCount());
	}

	/** Return time in ms, given time in ticks */
	inline static uint32_t ticksToMs(uint32_t ticks) {
		// Order of multiplication and division is important, because it shouldn't lose too much precision, but also not overflow
		return (uint32_t)ROUNDED_DIV(ticks, (uint64_t)RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000);
	}

	/** Return time in ticks, given time in ms
	 * Make sure time in ms is not too large! (limit is 512,000 ms with current frequency)
	 */
	inline static uint32_t msToTicks(uint32_t ms) {
		// Order of multiplication and division is important, because it shouldn't lose too much precision, but also not overflow
		return (uint64_t)ms * RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000;
	}
};
