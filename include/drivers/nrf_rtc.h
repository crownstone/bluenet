/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef INCLUDE_DRIVERS_NRF_RTC_H_
#define INCLUDE_DRIVERS_NRF_RTC_H_

#include <stdint.h>
#include <events/Dispatcher.h>

class RealTimeClock: public Dispatcher {
private:
	RealTimeClock() {};
	RealTimeClock(RealTimeClock const&); // singleton, deny implementation
	void operator=(RealTimeClock const &); // singleton, deny implementation
public:
	// use static variant of singelton, no dynamic memory allocation
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
private:
};

#endif /* INCLUDE_DRIVERS_NRF_RTC_H_ */
