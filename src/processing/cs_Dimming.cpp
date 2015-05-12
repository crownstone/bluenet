/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 1, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "processing/cs_Dimming.h"

Dimming::Dimming() {
}

void Dimming::configure() {
//	_adc->addListener(this);
	// configure timer for threshold
	
}

void Dimming::zeroCrossing() {
	// start timer again
	
	// turn on device as quick as possible
	turnOn();

	// on timer event
	turnOff();
}

void Dimming::turnOn() {
	
}

void Dimming::turnOff() {
	
}
