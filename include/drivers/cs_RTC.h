/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <stdint.h>

#include <cs_Nordic.h>

#include <common/cs_Timer.h>
#include <drivers/cs_Serial.h>

/*
 * Wrapper class for RTC functions of app_timer.
 */
class RTC {

private:

	RTC() {};
	RTC(RTC const&); // singleton, deny implementation
	void operator=(RTC const &); // singleton, deny implementation

public:

	// start RTC clock
	static void start() {
		// only thing we need to do is make sure the timer was created
		Timer::getInstance();
	}

	// return number of ticks
	static uint32_t getCount() {
		uint32_t count;
		app_timer_cnt_get(&count);
		return count;
	}

	// return difference between two tick counter values
	static uint32_t difference(uint32_t ticksTo, uint32_t ticksFrom) {
		uint32_t difference;
		app_timer_cnt_diff_compute(ticksTo, ticksFrom, &difference);
		return difference;
	}

	// return current clock in ms
	static uint32_t now() {
		return getCount() / (APP_TIMER_CLOCK_FREQ / (NRF_RTC1->PRESCALER + 1) / 1000);
//		return (uint32_t)ROUNDED_DIV(getCount(), (uint64_t)APP_TIMER_CLOCK_FREQ / (NRF_RTC1->PRESCALER + 1) / 1000);
	}
};
