/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 15, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "processing/cs_Fridge.h"
//#include "drivers/cs_RTC.h"
#include "drivers/cs_Timer.h"
#include <cfg/cs_Settings.h>
#include "drivers/cs_Temperature.h"
#include <events/cs_EventDispatcher.h>

Fridge::Fridge() : _appTimerId(0)
{
	Timer::getInstance().createRepeated(_appTimerId, (app_timer_timeout_handler_t)Fridge::staticTick);

	ps_configuration_t cfg = Settings::getInstance().getConfig();
	Storage::getInt8(cfg.minEnvTemp, _minTemp, MIN_ENV_TEMP);
	Storage::getInt8(cfg.maxEnvTemp, _maxTemp, MAX_ENV_TEMP);

}

void Fridge::startTicking() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(FRIDGE_UPDATE_FREQUENCY), this);
}

void Fridge::stopTicking() {
	Timer::getInstance().stop(_appTimerId);
}

void Fridge::tick() {
	int32_t temp;
	temp = getTemperature();
//	LOGd("temp = %d", temp);
	if (temp < _minTemp) {
		EventDispatcher::getInstance().dispatch(EVT_ENV_TEMP_LOW);
	}
	if (temp > _maxTemp) {
		EventDispatcher::getInstance().dispatch(EVT_ENV_TEMP_HIGH);
	}
}
