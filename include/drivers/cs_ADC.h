/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <stdint.h>

#include "common/cs_Buffer.h"
#include "events/cs_Dispatcher.h"
#include "cs_RTC.h"

/**
 * Analog digital conversion class. 
 */

#define ADC_BUFFER_SIZE 130
#define DEFAULT_RECORDING_THRESHOLD 1

class ADC: public Dispatcher {
private:
	ADC(): _bufferSize(ADC_BUFFER_SIZE), 
		_threshold(DEFAULT_RECORDING_THRESHOLD),
       		_clock(NULL) {}
	ADC(ADC const&); // singleton, deny implementation
	void operator=(ADC const &); // singleton, deny implementation
public:
	// use static variant of singleton, no dynamic memory allocation
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

	// set threshold (default threshold is DEFAULT_RECORDING_THRESHOLD)
	inline void setThreshold(uint8_t threshold) { _threshold = threshold; }
protected:
private:
	int _bufferSize;
	uint16_t _lastResult;
	uint16_t _numSamples;
	bool _store;
	uint8_t _threshold;

	RealTimeClock *_clock;
};
