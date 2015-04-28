/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

//#include <stdint.h>
//
//#include <ble/cs_Nordic.h>
//
#include <drivers/cs_Timer.h>
//#include <drivers/cs_Serial.h>

/* Clock frequency of the RTC timer */
#define RTC_CLOCK_FREQ          32768
/* Maximum value of the RTC counter. */
#define MAX_RTC_COUNTER_VAL     0x0007FFFF

/*
 * Wrapper class for RTC functions.
 *
 * if NRF51_USE_SOFTDEVICE==1 uses the RTC0 clock
 * managed by the softdevice
 * otherwise uses the RTC1 clock used by the app timer
 */
class RTC {

private:

	RTC() {};
	RTC(RTC const&); // singleton, deny implementation
	void operator=(RTC const &); // singleton, deny implementation

public:

	// return number of ticks
	inline static uint32_t getCount() {
#if NRF51_USE_SOFTDEVICE==1
		return NRF_RTC0->COUNTER;
#else
		uint32_t count;
		app_timer_cnt_get(&count);
		return count;
#endif
	}


	// return difference between two tick counter values
	inline static uint32_t difference(uint32_t ticksTo, uint32_t ticksFrom) {
#if NRF51_USE_SOFTDEVICE==1
		return ((ticksTo - ticksFrom) & MAX_RTC_COUNTER_VAL);
#else
		uint32_t difference;
		app_timer_cnt_diff_compute(ticksTo, ticksFrom, &difference);
		return difference;
#endif
	}

	// return current clock in ms
	inline static uint32_t now() {
#if NRF51_USE_SOFTDEVICE==1
		return getCount() / (RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000);
//		return (uint32_t)ROUNDED_DIV(getCount(), (uint64_t)RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000);
#else
		return getCount() / (APP_TIMER_CLOCK_FREQ / (NRF_RTC1->PRESCALER + 1) / 1000);
//		return (uint32_t)ROUNDED_DIV(getCount(), (uint64_t)APP_TIMER_CLOCK_FREQ / (NRF_RTC1->PRESCALER + 1) / 1000);
#endif
	}
};
