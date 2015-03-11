/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <stdint.h>

#include "common/cs_CircularBuffer.h"
#include "events/cs_Dispatcher.h"
#include "cs_RTC.h"

/**
 * Analog digital conversion class. 
 */

//#define ADC_BUFFER_SIZE 144
#define ADC_BUFFER_SIZE 131
#define DEFAULT_RECORDING_THRESHOLD 1

struct AdcSamples {
	uint32_t timeStart;
	uint32_t timeEnd;
	CircularBuffer<uint16_t>* buf;
};

class ADC: public Dispatcher {

public:
	// Use static variant of singleton, no dynamic memory allocation
	static ADC& getInstance() {
		static ADC instance;
		return instance;
	}

	/* If decorated with a real time clock, we can "timestamp" the adc values.
	 *
	 * @clock Clock that is running.
	 */
	void setClock(RealTimeClock &clock);

	/* Initialize ADC, this will allocate memory for the samples.
	 *
	 * @pin The pin to use as input (AIN<pin>).
	 */
	uint32_t init(uint8_t pin);

	// Start the ADC sampling
	void start();

	// Stop the ADC sampling
	void stop();

	// Each tick we have time to dispatch events e.g.
	void tick();

	/* Return reference to the sampled data
	 *
	 * @return a reference to the sampled data
	 */
	AdcSamples* getSamples();

	// return reference to internal buffer
	CircularBuffer<uint16_t>* getBuffer();

	/* Set threshold to start writing samples to buffer.
	 *
	 * Default threshold is DEFAULT_RECORDING_THRESHOLD
	 */
	inline void setThreshold(uint8_t threshold) { _threshold = threshold; }

	// Function to be called from interrupt, do not do much there!
	void update(uint32_t value);

private:
	/* Constructor
	 * Will not allocate memory yet.
	 */
	ADC(): _bufferSize(ADC_BUFFER_SIZE),
		_threshold(DEFAULT_RECORDING_THRESHOLD),
       		_clock(NULL) {}

	// This class is singleton, deny implementation
	ADC(ADC const&);
	// This class is singleton, deny implementation
	void operator=(ADC const &);

	int _bufferSize;
	uint16_t _lastResult;
	uint16_t _numSamples;
	bool _store;
	uint8_t _threshold;

	RealTimeClock *_clock;

	uint32_t config(uint8_t pin);
};
