/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 15, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "processing/cs_Fridge.h"
//#include "drivers/cs_RTC.h"
#include "drivers/cs_Timer.h"
#include <storage/cs_Settings.h>
#include "drivers/cs_Temperature.h"
#include <events/cs_EventDispatcher.h>

//#define PRINT_FRIDGE_VERBOSE

Fridge::Fridge() : _appTimerId(0)
{
//	Timer::getInstance().createRepeated(_appTimerId, (app_timer_timeout_handler_t)Fridge::staticTick);
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)Fridge::staticTick);

	Settings::getInstance().get(CONFIG_MIN_ENV_TEMP, &_minTemp);
	Settings::getInstance().get(CONFIG_MAX_ENV_TEMP, &_maxTemp);

	EventDispatcher::getInstance().addListener(this);
}

void Fridge::startTicking() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(FRIDGE_UPDATE_FREQUENCY), this);
}

void Fridge::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(FRIDGE_UPDATE_FREQUENCY), this);
}

void Fridge::stopTicking() {
	Timer::getInstance().stop(_appTimerId);
}

void Fridge::tick() {
	int32_t temp;
	temp = getTemperature();
#ifdef PRINT_FRIDGE_VERBOSE
	LOGd("temp = %d", temp);
#endif
	if (temp < _minTemp) {
		EventDispatcher::getInstance().dispatch(EVT_ENV_TEMP_LOW);
	}
	if (temp > _maxTemp) {
		EventDispatcher::getInstance().dispatch(EVT_ENV_TEMP_HIGH);
	}

	scheduleNextTick();
}


void Fridge::handleEvent(uint16_t evt, void* p_data, uint16_t length) {

	switch(evt) {
	case CONFIG_MIN_ENV_TEMP: {
		_minTemp = *(int32_t*)p_data;
#ifdef PRINT_FRIDGE_VERBOSE
		LOGd(FMT_SET_INT_VAL, "min temp", _minTemp);
#endif
		break;
	}
	case CONFIG_MAX_ENV_TEMP: {
		_maxTemp = *(int32_t*)p_data;
#ifdef PRINT_FRIDGE_VERBOSE
		LOGd(FMT_SET_INT_VAL, "max temp", _maxTemp);
#endif
		break;
	}
	}
}
