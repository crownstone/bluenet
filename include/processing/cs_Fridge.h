/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 15, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <cstdint>
#include <drivers/cs_Timer.h>
#include <events/cs_EventListener.h>

//! Fridge update frequence for events (Hz)
#define FRIDGE_UPDATE_FREQUENCY 0.2

/**
 * Fridge generates an event when its temperature is out of range.
 */
class Fridge : EventListener {
public:
	Fridge();
	void tick();
	static void staticTick(Fridge* ptr) {
		ptr->tick();
	}
	void startTicking();
	void stopTicking();
	void scheduleNextTick();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

private:
#if (NORDIC_SDK_VERSION >= 11)
	app_timer_t              _appTimerData;
	app_timer_id_t           _appTimerId;
#else
	uint32_t _appTimerId;
#endif
	int8_t _minTemp;
	int8_t _maxTemp;

};


