/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <stdint.h>

#include "events/cs_Dispatcher.h"

class RealTimeClock: public Dispatcher {
private:
	RealTimeClock() {};
	RealTimeClock(RealTimeClock const&); // singleton, deny implementation
	void operator=(RealTimeClock const &); // singleton, deny implementation
public:
	// use static variant of singleton, no dynamic memory allocation
	static RealTimeClock& getInstance() {
		static RealTimeClock instance;
		return instance;
	}

	// initialize clock
	uint32_t init(uint32_t ms=0);

	// the tick is used to dispatch events if they have accumulated
	void tick();

	// start clock
	void start();

	// stop clock
	void stop();

	// return number of ticks
	uint32_t getCount();

	// return reference to internal flag
	int getFlag();

	// return current clock in ms
	static uint32_t now();
private:
};
