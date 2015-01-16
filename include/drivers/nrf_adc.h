/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#ifndef __ADC__H__
#define __ADC__H__

#include <stdint.h>
#include <drivers/nrf_rtc.h>
#include <common/buffer.h>
#include <events/Dispatcher.h>

/**
 * Analog digital conversion class. 
 */

#define ADC_BUFFER_SIZE 130

class ADC: public Dispatcher {
private:
	ADC(): _buffer_size(ADC_BUFFER_SIZE), _clock(NULL) {}
	ADC(ADC const&); // singleton, deny implementation
	void operator=(ADC const &); // singleton, deny implementation
public:
	// use static variant of singelton, no dynamic memory allocation
	static ADC& getInstance() {
		static ADC instance;
		return instance;
	}

	// if decorated with a real time clock, we can "timestamp" the adc values
	void setClock(RealTimeClock &clock);

	uint32_t init(uint8_t pin);
	uint32_t config(uint8_t pin);
	void start();
	void stop();

	// function to be called from interrupt, do not do much there!
	void update(uint32_t value);

	// each tick we have time to dispatch events e.g.
	void tick();

	// return reference to internal buffer
	buffer_t<uint16_t>* getBuffer();

protected:
private:
	int _buffer_size;
	uint16_t _last_result;
	uint16_t _num_samples;
	bool _store;

	RealTimeClock *_clock;
};


#endif
