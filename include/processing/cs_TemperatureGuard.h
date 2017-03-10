/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 14, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "drivers/cs_Timer.h"
#include "drivers/cs_Temperature.h"
#include <cstdint>
//#include "cfg/cs_Config.h"
#include "storage/cs_Settings.h"
#include "events/cs_EventListener.h"
#include "events/cs_EventDispatcher.h"
#include "drivers/cs_COMP.h"
#include "cfg/cs_Boards.h"

#define TEMPERATURE_UPDATE_FREQUENCY 10

/** Check if the temperature exceeds a certain threshold
 */
class TemperatureGuard : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static TemperatureGuard& getInstance() {
		static TemperatureGuard instance;
		return instance;
	}

	void init(boards_config_t* boardConfig);

	void tick();

	void scheduleNextTick();

	void start();

	void stop();

	static void staticTick(TemperatureGuard* ptr) {
		ptr->tick();
	}

	void handleCompEvent(CompEvent_t event);

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

private:
	/** Constructor
	 */
	TemperatureGuard();

	//! This class is singleton, deny implementation
	TemperatureGuard(TemperatureGuard const&);
	//! This class is singleton, deny implementation
	void operator=(TemperatureGuard const &);


#if (NORDIC_SDK_VERSION >= 11)
	app_timer_t              _appTimerData;
	app_timer_id_t           _appTimerId;
#else
	uint32_t                 _appTimerId;
#endif
	int8_t _maxChipTemp;
	COMP* _comp;
	GeneralEventType _lastChipTempEvent;
	GeneralEventType _lastPwmTempEvent;

//	void scheduleNextTick() {
//		Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(TEMPERATURE_UPDATE_FREQUENCY), this);
//	}
};


