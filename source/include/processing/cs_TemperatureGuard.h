/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 14, 2015
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */

#pragma once

#include <cstdint>

#include "cfg/cs_Boards.h"
#include "common/cs_Types.h"
#include "drivers/cs_COMP.h"
#include "drivers/cs_Temperature.h"
#include "drivers/cs_Timer.h"
#include "events/cs_EventDispatcher.h"
#include "events/cs_EventListener.h"
#include "storage/cs_State.h"

#define TEMPERATURE_UPDATE_FREQUENCY 10

/** Check if the temperature exceeds a certain threshold
 */
class TemperatureGuard {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static TemperatureGuard& getInstance() {
		static TemperatureGuard instance;
		return instance;
	}

	void init(const boards_config_t& boardConfig);

	void tick();

	void scheduleNextTick();

	void start();

	void stop();

	static void staticTick(TemperatureGuard* ptr) { ptr->tick(); }

	void handleCompEvent(CompEvent_t event);

private:
	/** Constructor
	 */
	TemperatureGuard();

	//! This class is singleton, deny implementation
	TemperatureGuard(TemperatureGuard const&);
	//! This class is singleton, deny implementation
	void operator=(TemperatureGuard const&);

	app_timer_t _appTimerData;
	app_timer_id_t _appTimerId;
	TYPIFY(CONFIG_MAX_CHIP_TEMP) _maxChipTemp;
	COMP* _comp;
	CS_TYPE _lastChipTempEvent;
	CS_TYPE _lastPwmTempEvent;
	bool _dimmerTempInverted;
};
