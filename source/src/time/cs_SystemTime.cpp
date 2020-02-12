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

#include <protocol/mesh/cs_MeshModelPackets.h>

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
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_OUTGOING:{
			auto req = reinterpret_cast<cs_mesh_model_msg_sync_request_t*>(event.data);
			// fill the request with a data type that is shared accross the sphere

			// (assume crownstone_id is set by the Mesh::requestSync() method. )
			// for now just use field 0. Should be the first Unspecified one.
			req->sphere_data_ids[0] = static_cast<uint8_t>(SphereDataId::Time); 
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_INCOMING:{
			auto request = reinterpret_cast<cs_mesh_model_msg_sync_request_t*>(event.data);

			auto begin = std::begin(request->sphere_data_ids);
			auto end = std::end(request->sphere_data_ids);
			auto result = std::find(begin,end, static_cast<uint8_t>(SphereDataId::Time));

			if(result != end){
				// SphereDataId::Time was requested, SystemTime is responsible for that :)
				
				// create response struct
				cs_mesh_model_msg_sync_response_t response_payload = {{}};
				response_payload.crownstone_id = request->crownstone_id;
				response_payload.sphere_data_id = static_cast<uint8_t>(SphereDataId::Time);
				response_payload.data[0] = dummy_time;

				dummy_time++; // just for test data refreshing

				// build response payload
				TYPIFY(CMD_SEND_MESH_MSG) response_message = {{}};
				response_message.type = CS_MESH_MODEL_TYPE_SYNC_RESPONSE;
				response_message.payload = reinterpret_cast<uint8_t*>(&response_payload);
				response_message.size = sizeof(response_payload);

				// wrap it in an event to send over internal bus in order to broadcast
				event_t send_response_event(
							CS_TYPE::CMD_SEND_MESH_MSG, 
							&response_message, 
							sizeof(response_message) );

				send_response_event.dispatch();
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_RESPONSE_INCOMING:{
			auto response = reinterpret_cast<cs_mesh_model_msg_sync_response_t*>(event.data);

			if(SphereDataId(response->sphere_data_id) == SphereDataId::Time){
				// SphereDataId::Time was requested, SystemTime is responsible for that :)
				LOGd("sync response received, %x", response->data[0]);
				
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
