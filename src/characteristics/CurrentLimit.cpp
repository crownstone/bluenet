/*
 * CurrentLimit.cpp
 *
 *  Created on: Dec 2, 2014
 *      Author: Bart van Vliet
 */


#include <characteristics/CurrentLimit.h>
#include <drivers/serial.h>

CurrentLimit::CurrentLimit(LPComp& lpcomp): _lpcomp(lpcomp) {

}

CurrentLimit::~CurrentLimit() {

}

void CurrentLimit::init() {
	_lpcomp.addListener(this);
}

void CurrentLimit::handleEvent() {
	LOGd("current limit exceeded!");
	// Turn off the "led"
	// TODO: do this in a neat way
	nrf_pwm_set_value(0, 0);
}

void CurrentLimit::handleEvent(uint8_t type) {

}
