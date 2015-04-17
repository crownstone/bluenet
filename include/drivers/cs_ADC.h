/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <stdint.h>

#include "events/cs_Dispatcher.h"
#include "cs_RTC.h"
#include "characteristics/cs_CurrentCurve.h"

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

	void setCurrentCurve(CurrentCurve<uint16_t>* curve) { _currentCurve = curve; }


//	/* Set threshold to start writing samples to buffer.
//	 *
//	 * Default threshold is DEFAULT_RECORDING_THRESHOLD
//	 */
//	inline void setThreshold(uint8_t threshold) { _threshold = threshold; }

	// Function to be called from interrupt, do not do much there!
	void update(uint16_t value);

private:
	/* Constructor
	 */
	ADC(): _currentCurve(NULL), _clock(NULL) {}

	// This class is singleton, deny implementation
	ADC(ADC const&);
	// This class is singleton, deny implementation
	void operator=(ADC const &);

	uint16_t _sampleNum;
	uint16_t _lastResult;
//	uint8_t _threshold;
	CurrentCurve<uint16_t>* _currentCurve;

	RealTimeClock *_clock;

	uint32_t config(uint8_t pin);
};
