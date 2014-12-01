/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 1, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <characteristics/Dimmer.h>

Dimming::Dimming() {
	// set callback
	ZeroCrossingListener::listener = &zeroCrossing;
}

virtual Dimming::~Dimming() {
}

void Dimming::configure() {
	// configure timer for threshold
	
}

void Dimming::zeroCrossing() {
	// start timer again
	
	// turn on device as quick as possible
	TurnOn();

	// on timer event
	TurnOff();
}

void Dimmer::turnOn() {
	
}

void Dimmer::turnOff() {
	
}

ZeroCrossing::ZeroCrossing(ADC &adc): _adc(adc) { 
	positive_current = PIN_ADC0;
	inverted_current = PIN_ADC1;
}

virtual ZeroCrossing::~ZeroCrossing() {
}

void ZeroCrossing::configure() {
	// TODO: check if ADC is already in use..., add "owner" type of info to adc
	
	adc->nrf_adc_init(positive_current);
}

void ZeroCrossing::calcCrossing() {
	adc->nrf_adc_start();
	buffer<uint16_t> *buf = adc->getBuffer();

	bool zero_flag = false;
	while (!zero_flag) {
		if (!buf->empty) {
			uint16_t value = buf->pop();
			if (!value) zero_flag = true;
		} else {
			// TODO: now we have to iterate through a while-loop in the buffer, it is much better if we 
			// can set a callback in the ADC class, then we do not need to do waiting here e.g.
			// TODO: make sure this loop doesn't burn through all processor cycles
		}
	}
}

