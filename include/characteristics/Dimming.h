/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 1, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef CS_DIMMING_H_
#define CS_DIMMING_H_

#include <util/function.h> 

#include <drivers/nrf_adc.h>

/**
 * Dimming of LEDs is by a simple duty cycle of the 230V/110V sine signal. 
 */
class Dimming: public Listener {
	public:
		Dimming();

		virtual ~Dimming();

		// We get a dispatch from the ZeroCrossing utility 
		void handleEvent() {
			zeroCrossing();
		}

		// Function to be executed on a zero-crossing
		void zeroCrossing();
	private:

		ADC *_adc;
};

/**
 * Calculate zero-crossing. This uses the LPCOMP circuitry. This disallows the use of ADC at the same time.
 */
class ZeroCrossing: public Dispatcher {
	public:
		ZeroCrossing(ADC &adc);

		virtual ~ZeroCrossing();

	protected:
		void crossEvent() {
			dispatch();
		}

		void calculateCrossing();
	private:
		ADC *_adc;

		uint8_t positive_current;

		uint8_t inverted_current; // made positive, so uint8_t!
}


#endif // CS_DIMMING_H_
