/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <time/cs_SystemTime.h>

#include <common/cs_Types.h>
#include <drivers/cs_RNG.h>
#include <drivers/cs_RTC.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <time/cs_Time.h>
#include <time/cs_TimeOfDay.h>

#define LOGSystemTimeVerbose LOGnone

// ============== Static members ==============

uint32_t SystemTime::rtcTimeStamp = 0;
uint32_t SystemTime::posixTimeStamp = 0;
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
		if (posixTimeStamp != 0) {
			// add 1s to posix time
			posixTimeStamp++;
			
			// and store the new time
			State::getInstance().set(CS_TYPE::STATE_TIME, &posixTimeStamp, sizeof(posixTimeStamp));

			LOGSystemTimeVerbose("posix=%u", posixTimeStamp);
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

    LOGi("Set time to %u %02d:%02d:%02d", time, t.h(), t.m(), t.s());
    
    uint32_t prevtime = posixTimeStamp;
	posixTimeStamp = time;
	rtcTimeStamp = RTC::getCount();

	event_t event(
        CS_TYPE::EVT_TIME_SET,
        &prevtime,
        sizeof(prevtime));

	event.dispatch();
}

uint8_t dummy_time = 0xae;

void SystemTime::handleEvent(event_t & event) {
	switch(event.type) {
		case CS_TYPE::STATE_TIME: {
			// Time was set via State.set().
			// This may have been set by us! So only use it when no time is set yet?
			if (posixTimeStamp == 0) {
				LOGd("set time from state");
				setTime(*((TYPIFY(STATE_TIME)*)event.data));
			}
			break;
		}
		case CS_TYPE::EVT_MESH_TIME: {
			// Only set the time if there is currently no time set, as these timestamps may be old
			if (posixTimeStamp == 0) {
				LOGd("set time from mesh");
				setTime(*((TYPIFY(EVT_MESH_TIME)*)event.data));
			}
			break;
		}
		case CS_TYPE::CMD_SET_TIME: {
			LOGd("set time from command");
			setTime(*((TYPIFY(CMD_SET_TIME)*)event.data));
			break;
		}
		case CS_TYPE::STATE_SUN_TIME: {
			// Sunrise/sunset adjusted. No need to do anything as it is already persisted.
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_OUTGOING: {
			if (posixTimeStamp == 0) {
				// If posix time is unknown, we request for it.
				auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_OUTGOING)*>(event.data);
				req->bits.time = true;
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_INCOMING: {
			auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING)*>(event.data);
			if (req->bits.time && posixTimeStamp != 0) {
				// Posix time is requested by a crownstone in the mesh.
				// If we know the time, send it.
				// But only with a 1/10 chance, to prevent flooding the mesh.
				uint8_t rand8;
				RNG::fillBuffer(&rand8, 1);
				if (rand8 < 26) {
					cs_mesh_model_msg_time_t packet;
					packet.timestamp = posixTimeStamp;

					TYPIFY(CMD_SEND_MESH_MSG) meshMsg;
					meshMsg.type = CS_MESH_MODEL_TYPE_STATE_TIME;
					meshMsg.urgency = CS_MESH_URGENCY_HIGH;
					meshMsg.reliability = CS_MESH_RELIABILITY_LOW;
					meshMsg.payload = (uint8_t*)&packet;
					meshMsg.size = sizeof(packet);
					event_t timeEvent(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
					timeEvent.dispatch();
				}
			}
			break;
		}
		default: {}
	}
}

Time SystemTime::posix(){
    return posixTimeStamp;
}

DayOfWeek SystemTime::day(){
    return posix().dayOfWeek();
}

Time SystemTime::now(){
     return posix();
}


uint32_t SystemTime::up(){
	return uptime_sec;
}
