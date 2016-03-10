/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include "structs/cs_AlertAccessor.h"

#define SCHEDULE_SERVICE_UPDATE_FREQUENCY 2

/** ScheduleService organizes ticks for all components for non-urgent timing.
 */
class ScheduleService : public BLEpp::Service {
public:
//	typedef function<int8_t()> func_t;

	/** Constructor for alert notification service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store current limit.
	 * It also initializes all characteristics.
	 */
	ScheduleService();

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void init();

	/** Returns the current time as posix time
	 * returns 0 when no time was set yet
	 */
	uint32_t getTime();

	/** Set current posix time */
	void setTime(uint32_t time);

	/** Perform non urgent functionality every main loop.
	 *
	 * Every component has a "tick" function which is for non-urgent things.
	 * Urgent matters have to be resolved immediately in interrupt service handlers.
	 */
	void tick();
	void scheduleNextTick();

protected:
	void addCurrentTimeCharacteristic();



private:
	//! References to characteristics that need to be written from other functions
	BLEpp::Characteristic<uint32_t> *_currentTimeCharacteristic;
	uint32_t _rtcTimeStamp;
};
