/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <drivers/cs_Timer.h>
#include <events/cs_EventListener.h>

#include <time/cs_Time.h>
#include <time/cs_TimeOfDay.h>

#include <stdint.h>

/**
 * This class keeps track of the real time in the current time zone.
 * It may obtain its data through the mesh, or some other way and try
 * to keep up to date using on board timing functionality.
 */
class SystemTime : public EventListener {
public:
	/**
	 * Get the current time as posix timestamp.
	 */
    static uint32_t posix();

    /**
     * Get the current time as Time object.
     */
    static Time now();
    
    /**
     * Get the current weekday.
     */
    static DayOfWeek day();

    /**
     * Uptime in seconds.
     */
    static uint32_t up();

    virtual void handleEvent(event_t& event);

    /**
     * Set the system wide time to given posix timestamp.
     * Dispatches EVT_TIME_SET.
     */
	static void setTime(uint32_t time);
    
    /**
     * Creates and starts the first tick timer.
     */
    static void init();

private:
    // state data
	static uint32_t rtcTimeStamp;
	static uint32_t posixTimeStamp;
    static uint32_t uptime_sec;

    // timing features
    static app_timer_t              appTimerData;
	static app_timer_id_t           appTimerId;

    static void scheduleNextTick();
    static void tick(void* unused);

    // settings
    static constexpr auto SCHEDULER_UPDATE_FREQUENCY = 2;
};

