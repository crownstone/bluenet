/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 1, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

//#include "third/std/function.h"
#include "drivers/cs_ADC.h"

/**
 * Dimming of LEDs is by a simple duty cycle of the 230V/110V sine signal.
 */
class Dimming {
public:
	Dimming();

	~Dimming();

	void configure();

	// We get a dispatch from the ZeroCrossing utility
	void handleEvent() {
		zeroCrossing();
	}

	// Function to be executed on a zero-crossing
	void zeroCrossing();

	void turnOn();

	void turnOff();
private:
	// Reference to AD converter
	ADC *_adc;
};
