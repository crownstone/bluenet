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
    static Time posix();
    static TimeOfDay now();

    /**
     * Uptime in seconds.
     */
    static uint32_t up();

    // static Day day();
    // static Month month();
    // static Date date();

    virtual void handleEvent(event_t& event);

    /**
     * Changes the system wide time (i.e. SystemTime::posixTimeStamp) 
     * to the given value. Dispatches a EVT_TIME_SET event to signal 
     * any interested Listeners.
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
    static app_timer_t              appTimerData;// = {{0}};
	static app_timer_id_t           appTimerId;// = &_appTimerData;

    static void scheduleNextTick();
    static void tick(void* unused);

    // settings
    static constexpr auto SCHEDULER_UPDATE_FREQUENCY = 2;
};

