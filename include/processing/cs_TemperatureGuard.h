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
#include "cfg/cs_Settings.h"
#include "events/cs_EventListener.h"

#define TEMPERATURE_UPDATE_FREQUENCY 0.2

/** Protection against temperature exceeding certain threshold
 */
class TemperatureGuard : EventListener {
public:
	TemperatureGuard() :
		_appTimerId(0),
		EventListener(CONFIG_MAX_CHIP_TEMP),
		_maxTemp(MAX_CHIP_TEMP)
	{

		ps_configuration_t cfg = Settings::getInstance().getConfig();
		Storage::getInt8(cfg.maxChipTemp, _maxTemp, MAX_CHIP_TEMP);

		EventDispatcher::getInstance().addListener(this);

//		Timer::getInstance().createRepeated(_appTimerId, (app_timer_timeout_handler_t)TemperatureGuard::staticTick);
		Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)TemperatureGuard::staticTick);

		LOGe("temp guard timer id: %d", _appTimerId);
	}
	void tick() {
		if (getTemperature() > _maxTemp) {
			//! Make sure pwm can't be set anymore
			PWM::getInstance().deinit();
			//! Switch off all channels
			PWM::getInstance().switchOff();
		}
		// TODO: make next time to next tick depend on current temperature
		scheduleNextTick();
	}

	void scheduleNextTick() {
//		LOGi("TemperatureGuard::scheduleNextTick");
		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
	}

	void startTicking() {
//		LOGi("TemperatureGuard::startTicking");
		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
	}

	void stopTicking() {
		Timer::getInstance().stop(_appTimerId);
	}

	static void staticTick(TemperatureGuard* ptr) {
		ptr->tick();
	}

	void handleEvent(uint16_t evt, void* p_data, uint16_t length) {
		switch (evt) {
		case CONFIG_MAX_CHIP_TEMP:
			_maxTemp = *(int32_t*)p_data;
			break;
		}
	}
private:
	uint32_t _appTimerId;
	int8_t _maxTemp;

//	void scheduleNextTick() {
//		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
//	}
};


