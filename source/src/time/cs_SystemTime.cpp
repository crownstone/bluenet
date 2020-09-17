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
#include <drivers/cs_Serial.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <time/cs_Time.h>
#include <time/cs_TimeOfDay.h>

#include <test/cs_Test.h>

#define LOGSystemTimeDebug   LOGnone
#define LOGSystemTimeVerbose LOGnone

// ============== Static members ==============

// runtime variables
uint32_t SystemTime::rtcTimeStamp = 0;
uint32_t SystemTime::posixTimeStamp = 0;
uint32_t SystemTime::uptime_sec = 0;

uint16_t SystemTime::throttleSetTimeCountdownTicks = 0;
uint16_t SystemTime::throttleSetSunTimesCountdownTicks = 0;

// driver details
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
			MS_TO_TICKS(TICK_TIME_MS),
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

		TEST_PUSH_STATIC_D("SystemTime", "posixTime", posixTimeStamp);
		TEST_PUSH_STATIC_D("SystemTime", "timeOfday_h", now().timeOfDay().h());
		TEST_PUSH_STATIC_D("SystemTime", "timeOfday_m", now().timeOfDay().m());
		TEST_PUSH_STATIC_D("SystemTime", "timeOfday_s", now().timeOfDay().s());

		// update rtc timestamp subtract 1s from tickDiff by
		// increasing the rtc timestamp 1s.
		rtcTimeStamp += RTC::msToTicks(1000);

		// increment uptime.
		++uptime_sec; 
	}

	if (throttleSetTimeCountdownTicks) {
		--throttleSetTimeCountdownTicks;
	}
	if (throttleSetSunTimesCountdownTicks) {
		--throttleSetSunTimesCountdownTicks;
	}

	scheduleNextTick();
}

// ======================== Setters ========================

void SystemTime::setTime(uint32_t time, bool throttled) {
	if (time == 0) {
		return;
	}

	if (throttled && throttleSetTimeCountdownTicks) {
		LOGSystemTimeDebug("setTime throttled");
		return;
	}
	throttleSetTimeCountdownTicks = THROTTLE_SET_TIME_TICKS;

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

cs_ret_code_t SystemTime::setSunTimes(const sun_time_t& sunTimes, bool throttled) {
	if (sunTimes.sunrise > 24*60*60 || sunTimes.sunset > 24*60*60) {
		LOGw("Invalid suntimes: rise=%u set=%u", sunTimes.sunrise, sunTimes.sunset);
		return ERR_WRONG_PARAMETER;
	}

	if (throttled && throttleSetSunTimesCountdownTicks) {
		LOGSystemTimeDebug("setSunTimes throttled");
		return ERR_BUSY;
	}
	throttleSetSunTimesCountdownTicks = THROTTLE_SET_SUN_TIMES_TICKS;

	LOGSystemTimeDebug("setSunTimes rise=%u set=%u", sunTimes.sunrise, sunTimes.sunset);

	TYPIFY(STATE_SUN_TIME) stateSunTimes = sunTimes;
	cs_state_data_t stateData = cs_state_data_t(CS_TYPE::STATE_SUN_TIME, reinterpret_cast<uint8_t*>(&stateSunTimes), sizeof(stateSunTimes));
	cs_ret_code_t retCode = State::getInstance().setThrottled(stateData, SUN_TIME_THROTTLE_PERIOD_SECONDS);
	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_WAIT_FOR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE:
			return ERR_SUCCESS;
		default:
			return retCode;
	}
}

// ======================== Events ========================

void SystemTime::handleEvent(event_t & event) {
	switch(event.type) {
		case CS_TYPE::STATE_TIME: {
			// Time was set via State.set().
			// This may have been set by us! So only use it when no time is set yet?
			if (posixTimeStamp == 0) {
				LOGd("set time from state");
				setTime(*((TYPIFY(STATE_TIME)*)event.data));
				TEST_PUSH_D(this, posixTimeStamp);
			}
			break;
		}
		case CS_TYPE::EVT_MESH_TIME: {
			// Only set the time if there is currently no time set, as these timestamps may be old
			if (posixTimeStamp == 0) {
				LOGd("set time from mesh");
				setTime(*((TYPIFY(EVT_MESH_TIME)*)event.data));
				TEST_PUSH_D(this, posixTimeStamp);
			}
			break;
		}
		case CS_TYPE::CMD_SET_TIME: {
			LOGd("set time from command");
			setTime(*((TYPIFY(CMD_SET_TIME)*)event.data));
			TEST_PUSH_D(this, posixTimeStamp);
			break;
		}
		case CS_TYPE::CMD_TEST_SET_TIME: {
			LOGd("set test time");
			setTime(*((TYPIFY(CMD_SET_TIME)*)event.data), false);
			TEST_PUSH_D(this, posixTimeStamp);
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
				if (RNG::getInstance().getRandom8() < (255 / 10 + 1)) {
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

// ======================== Synchronization ========================

uint32_t SystemTime::syncTimeCoroutine(){
	if(no_data_known_yet()){
		// wait until sync time is over
		return sync_timeout_ms;
	}

	if(havent_heard_anything_for(sync_timeout_ms)){
		// the master clock just disappeared and we're in the process
		// of determining a new one.. lets consider ourselves a candidate
		currentMasterClockId = myId;
	}

	if(myId <= currentMasterClockId){
		// we're the master clock!
		sendTimeSyncMessage();
		return master_clock_update_period_ms;
	}

	return master_clock_reelection_period_ms;
}

void SystemTime::onTimeSyncMessageReceive(auto syncmessage){
	if (syncmessage->id <= currentMasterClockId){
		// lower id wins, sync message wins authority on the clock values.
		posixTimeStamp = syncmessage->timestamp;
		currentMasterClockId = syncmessage->id;
	}
}

void SystemTime::sendTimeSyncMessage(){

}

// ======================== Utility functions ========================

uint32_t SystemTime::posix(){
	return posixTimeStamp;
}

DayOfWeek SystemTime::day(){
	return now().dayOfWeek();
}

Time SystemTime::now(){
	return posix();
}


uint32_t SystemTime::up(){
	return uptime_sec;
}
