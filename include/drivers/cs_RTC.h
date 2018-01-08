/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
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
//#define MAX_RTC_COUNTER_VAL     0x0007FFFF //! Where did this come from?
#define MAX_RTC_COUNTER_VAL     0x00FFFFFF

/** Wrapper class for RTC functions.
 *
 * if NRF51_USE_SOFTDEVICE==1 uses the RTC0 clock
 * managed by the softdevice
 * otherwise uses the RTC1 clock used by the app timer
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
#if NRF51_USE_SOFTDEVICE==1
		return NRF_RTC0->COUNTER;
#else
		uint32_t count;
		app_timer_cnt_get(&count);
		return count;
#endif
	}


	//! return difference between two tick counter values
	inline static uint32_t difference(uint32_t ticksTo, uint32_t ticksFrom) {
#if NRF51_USE_SOFTDEVICE==1
		return ((ticksTo - ticksFrom) & MAX_RTC_COUNTER_VAL);
#else
		uint32_t difference;
		app_timer_cnt_diff_compute(ticksTo, ticksFrom, &difference);
		return difference;
#endif
	}

	//! return current clock in ms
	inline static uint32_t now() {
		return ticksToMs(getCount());
	}

	/** Return time in ms, given time in ticks */
	inline static uint32_t ticksToMs(uint32_t ticks) {
#if NRF51_USE_SOFTDEVICE==1
		//! Order of multiplication and division is important, because it shouldn't lose too much precision, but also not overflow
//		return ticks * (NRF_RTC0->PRESCALER + 1) / RTC_CLOCK_FREQ * 1000;
		return (uint32_t)ROUNDED_DIV(ticks, (uint64_t)RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000);
#else
//		return ticks * (NRF_RTC1->PRESCALER + 1) / APP_TIMER_CLOCK_FREQ * 1000;
		return (uint32_t)ROUNDED_DIV(ticks, (uint64_t)APP_TIMER_CLOCK_FREQ / (NRF_RTC1->PRESCALER + 1) / 1000);
#endif
	}

	/** Return time in ticks, given time in ms
	 * Make sure time in ms is not too large! (limit is 512,000 ms with current frequency)
	 */
	inline static uint32_t msToTicks(uint32_t ms) {
#if NRF51_USE_SOFTDEVICE==1
		//! Order of multiplication and division is important, because it shouldn't lose too much precision, but also not overflow
		return (uint64_t)ms * RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000;
#else
		return (uint64_t)ms * APP_TIMER_CLOCK_FREQ / (NRF_RTC1->PRESCALER + 1) / 1000;
#endif
	}
};
