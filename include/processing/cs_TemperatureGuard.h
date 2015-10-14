/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 14, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "drivers/cs_Timer.h"
#include "drivers/cs_PWM.h"
#include "drivers/cs_Temperature.h"
#include <cstdint>

#define TEMPERATURE_UPDATE_FREQUENCY 0.2 // Hz
#define TEMPERATURE_MAX 80 // Celsius

class TemperatureGuard {
public:
	TemperatureGuard() : _appTimerId(0) {
		Timer::getInstance().createRepeated(_appTimerId, (app_timer_timeout_handler_t)TemperatureGuard::staticTick);
//		Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)TemperatureGuard::staticTick);
	}
	void tick() {
		if (getTemperature() > TEMPERATURE_MAX) {
			// Make sure pwm can't be set anymore
			PWM::getInstance().deinit();
			// Switch off all channels
			PWM::getInstance().switchOff();
		}
		// TODO: make next time to next tick depend on current temperature
//		scheduleNextTick();
	}

	void startTicking() {
		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
	}

	void stopTicking() {
		Timer::getInstance().stop(_appTimerId);
	}

	static void staticTick(TemperatureGuard* ptr) {
		ptr->tick();
	}
private:
	uint32_t _appTimerId;

//	void scheduleNextTick() {
//		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
//	}
};


