/*
 * CurrentLimit.cpp
 *
 *  Created on: Dec 2, 2014
 *      Author: Bart van Vliet
 */


#include <characteristics/CurrentLimit.h>
#include <drivers/serial.h>
#include <common/cs_boards.h>

CurrentLimit::CurrentLimit() {

}

CurrentLimit::~CurrentLimit() {

}

void CurrentLimit::init() {
	LPComp::getInstance().addListener(this);
}

void CurrentLimit::start(uint8_t* limit_value) {
	// There are only 6 levels...
	if (*limit_value > 6)
		*limit_value = 6;
	LPComp::getInstance().stop();
	LPComp::getInstance().config(PIN_AIN_LPCOMP, *limit_value, LPComp::LPC_UP);
	LPComp::getInstance().start();
}

void CurrentLimit::handleEvent() {
	LOGd("current limit exceeded!");
	// Turn off the "led"
	// TODO: do this in a neat way
	PWM::getInstance().setValue(0, 0);
}

void CurrentLimit::handleEvent(uint8_t type) {

}
