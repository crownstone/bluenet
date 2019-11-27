/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <time/cs_SystemTime.h>

#include <common/cs_Types.h>
#include <drivers/cs_RTC.h>
#include <storage/cs_State.h>
#include <events/cs_EventDispatcher.h>

#include <time/cs_Time.h>
#include <time/cs_TimeOfDay.h>

// ============== Static members ==============

uint32_t SystemTime::rtcTimeStamp;
uint32_t SystemTime::posixTimeStamp;
uint32_t SystemTime::uptime_sec = 0;

app_timer_t SystemTime::appTimerData = {{0}};
app_timer_id_t SystemTime::appTimerId = &appTimerData;

// ============ SystemTime implementation ===========

// Init and start the timer.
void SystemTime::init(){
	Timer::getInstance().createSingleShot(
        appTimerId, 
        static_cast<app_timer_timeout_handler_t>(&SystemTime::tick));

	scheduleNextTick();
}

void SystemTime::scheduleNextTick() {
		Timer::getInstance().start(
            appTimerId, 
            HZ_TO_TICKS(SCHEDULER_UPDATE_FREQUENCY), 
            nullptr);
}

void SystemTime::tick(void*) {
	// RTC can overflow every 512s
	uint32_t tickDiff = RTC::difference(RTC::getCount(), rtcTimeStamp);

	// If more than 1s elapsed since last set rtc timestamp:
	if (tickDiff > RTC::msToTicks(1000)) {
		if(posixTimeStamp != 0){
			// add 1s to posix time
			posixTimeStamp++;
			
			// and store the new time
			State::getInstance().set(CS_TYPE::STATE_TIME, &posixTimeStamp, sizeof(posixTimeStamp));
		}

		// update rtc timestamp subtract 1s from tickDiff by
		// increasing the rtc timestamp 1s.
		rtcTimeStamp += RTC::msToTicks(1000);

		// increment uptime.
		++uptime_sec; 
	}

	scheduleNextTick();
}

void SystemTime::setTime(uint32_t time) {
	if (time == 0) {
		return;
	}
	TimeOfDay t(time);

    LOGi("Set time to %02d:%02d:%02d", t.h(), t.m(), t.s());
    
    uint32_t prevtime = posixTimeStamp;
	posixTimeStamp = time;
	rtcTimeStamp = RTC::getCount();

	event_t event(
        CS_TYPE::EVT_TIME_SET,
        &prevtime,
        sizeof(prevtime));

	event.dispatch();
}

void SystemTime::handleEvent(event_t & event) {
	switch(event.type) {
		case CS_TYPE::STATE_TIME: {
			// Time was set via State.set().
			// This may have been set by us! So only use it when no time is set yet?
			if (posixTimeStamp == 0 && event.size == sizeof(TYPIFY(STATE_TIME))) {
				setTime(*((TYPIFY(STATE_TIME)*)event.data));
			}
			break;
		}
		case CS_TYPE::EVT_MESH_TIME: {
			// Only set the time if there is currently no time set, as these timestamps may be old
			if (posixTimeStamp == 0 && event.size == sizeof(TYPIFY(EVT_MESH_TIME))) {
				setTime(*((TYPIFY(EVT_MESH_TIME)*)event.data));
			}
			break;
		}
		case CS_TYPE::CMD_SET_TIME: {
			if (event.size == sizeof(TYPIFY(CMD_SET_TIME))) {
				setTime(*((TYPIFY(CMD_SET_TIME)*)event.data));
			}
			break;
		}
		case CS_TYPE::STATE_SUN_TIME: {
			// Sunrise/sunset adjusted. No need to do anything as it is already persisted.
		}
		default: {}
	}
}

TimeOfDay SystemTime::now(){
     return TimeOfDay(posixTimeStamp);
}

Time SystemTime::posix(){
    return posixTimeStamp;
}

uint32_t SystemTime::up(){
	return uptime_sec;
}