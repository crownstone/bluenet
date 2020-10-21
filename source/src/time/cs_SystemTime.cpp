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
#include <time/cs_TimeSyncMessage.h>

#include <test/cs_Test.h>

#include <util/cs_Lollipop.h>

#define LOGSystemTimeDebug   LOGnone
#define LOGSystemTimeVerbose LOGnone

#if defined(DEBUGSYSTEM_TIME)
#ifndef DEBUG
#error "SystemTime Debug must be turned off in release"
#endif
#else
#pragma message("SystemTime Debug is turned on.")
#endif

// ============== Static members ==============

// runtime variables
uint32_t SystemTime::last_statetimeevent_stamp_rtc = 0;
uint32_t SystemTime::upTimeSec = 0;

uint16_t SystemTime::throttleSetTimeCountdownTicks = 0;
uint16_t SystemTime::throttleSetSunTimesCountdownTicks = 0;

// sync
high_resolution_time_stamp_t SystemTime::last_received_root_stamp = {0};
uint32_t SystemTime::local_time_of_last_received_root_stamp_rtc_ticks = 0;
stone_id_t SystemTime::currentMasterClockId = stone_id_unknown_value;
stone_id_t SystemTime::myId = stone_id_unknown_value;
Coroutine SystemTime::syncTimeCoroutine;

#ifdef DEBUG_SYSTEM_TIME
Coroutine SystemTime::debugSyncTimeCoroutine;
#endif


// driver details
app_timer_t SystemTime::appTimerData = {{0}};
app_timer_id_t SystemTime::appTimerId = &appTimerData;

// ============ SystemTime implementation ===========

// Init and start the timer.
void SystemTime::init(){
	assertTimeSyncParameters();
	syncTimeCoroutine.action = [](){ return syncTimeCoroutineAction(); };

#ifdef DEBUG_SYSTEM_TIME
	debugSyncTimeCoroutine.action = [](){
		LOGd("debug sync time");
		publishSyncMessageForTesting();
		return Coroutine::delay_ms(debugSyncTimeMessagePeriodMs);
	};
#endif

	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &myId, sizeof(myId));

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
	static bool first_call_to_tick = true;
	if(first_call_to_tick){
		first_call_to_tick = false;
		last_statetimeevent_stamp_rtc = RTC::getCount();

		high_resolution_time_stamp_t start_stamp;
		start_stamp.posix_s = 0;
		start_stamp.posix_ms = 0;
		start_stamp.version = 0;
		logRootTimeStamp(start_stamp, myId);
	}

	if (RTC::msPassedSince(last_statetimeevent_stamp_rtc) >= 1000){
		// a second has passed!
		last_statetimeevent_stamp_rtc += RTC::msToTicks(1000);
		upTimeSec += 1;

		auto stamp = getSynchronizedStamp();
		uint32_t posix_s = stamp.posix_s;
		State::getInstance().set(CS_TYPE::STATE_TIME, &posix_s, sizeof(posix_s));

		if(thisDeviceClaimsMasterClock()){
			// Master clokc has to update its stamp every
			// now and then to prevent roll over.
			updateRootTimeStamp();
		}
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

void SystemTime::setTime(uint32_t time, bool throttled, bool unsynchronize) {
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

#ifdef DEBUG_SYSTEM_TIME
	publishSyncMessageForTesting();
#endif

	uint32_t prevtime = posix();

	high_resolution_time_stamp_t stamp;
	stamp.posix_s = time;
	stamp.posix_ms = 0;
	if(unsynchronize){
		// incrementing the version would incur mesh syncrhonisation.
		stamp.version = last_received_root_stamp.version;
	} else {
		stamp.version = Lollipop::next(last_received_root_stamp.version, time_stamp_version_lollipop_max);
	}

	if (unsynchronize) {
		// just log the stamp locally, with myId as clock id.
		// Don't mention anything to the mesh.
		logRootTimeStamp(stamp, myId);
	} else {
		// log with root_id 0 and broadcast this over the mesh.
		logRootTimeStamp(stamp, 0);

		// Note:
		// setting root_id 0 is a brutal claim to be root.
		// It results in no crownstone sending time sync messages
		// anymore until the master_clock_reelection_timeout_ms expires.
		// It strictly enforces the synchronisation among crownstones
		// because all nodes, even the true root clock, will update
		// their local time.
	}

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
	bool handled_by_coroutine = false;
	handled_by_coroutine |= syncTimeCoroutine(event);
#ifdef DEBUG_SYSTEM_TIME
	handled_by_coroutine |= debugSyncTimeCoroutine(event);
#endif
	if (handled_by_coroutine) {
		return;
	}

	switch(event.type) {
		case CS_TYPE::CMD_SET_TIME: {
			LOGd("set time from command");
			// TODO: based on source, decide wether or not to broadcast the update
			setTime(*((TYPIFY(CMD_SET_TIME)*)event.data));
			if(isOnlyReceiveByThisDevice(event.source)){
				// only this device heard the message, so we make sure to propagate it.
				sendTimeSyncMessage(getSynchronizedStamp(), 0);
			}
			break;
		}
		case CS_TYPE::CMD_TEST_SET_TIME: {
			LOGd("set test time");
			// calls setTime, ignoring throttling and allowing desynchronization

			// TODO: Don't broadcast!
			setTime(*((TYPIFY(CMD_SET_TIME)*)event.data), false, true);
			break;
		}
		case CS_TYPE::STATE_SUN_TIME: {
			// Sunrise/sunset adjusted. No need to do anything as it is already persisted.
			break;
		}
		case CS_TYPE::EVT_MESH_TIME_SYNC: {
			time_sync_message_t* syncmsg = reinterpret_cast<TYPIFY(EVT_MESH_TIME_SYNC)*>(event.data);
			onTimeSyncMessageReceive(*syncmsg);
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_OUTGOING: {
			if (timeStampVersion() == 0) {
				auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_OUTGOING)*>(event.data);
				req->bits.time = true;
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_INCOMING: {
			auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING)*>(event.data);
			if (req->bits.time && timeStampVersion() != 0) {
				// Posix time is requested by a crownstone in the mesh.
				// If we know the time, send it.
				// But only with a 1/10 chance, to prevent flooding the mesh.
				if (RNG::getInstance().getRandom8() < (255 / 10 + 1)) {
					// we're sending what we currently think is the time.
					// don't use currentMasterClockId here, we don't want to
					// be an impostor as we might have drifted a bit.
					sendTimeSyncMessage(getSynchronizedStamp(), myId);
				}
			}
			break;
		}
		default: {}
	}
}

bool SystemTime::isOnlyReceiveByThisDevice(cmd_source_with_counter_t counted_source){
	auto src = counted_source.source;
	if (src.flagExternal) {
		return false;
	}

	return src.type == CS_CMD_SOURCE_TYPE_UART
			|| (src.type == CS_CMD_SOURCE_TYPE_ENUM
					&&  src.id == CS_CMD_SOURCE_CONNECTION);
}

// ======================== Synchronization ========================

void SystemTime::logRootTimeStamp(high_resolution_time_stamp_t stamp, stone_id_t id){
	LOGSystemTimeDebug("logRootTimeStamp, changing masterClockId of #%d to: #%d", myId, id);

	currentMasterClockId = id;
	last_received_root_stamp = stamp;
	local_time_of_last_received_root_stamp_rtc_ticks = RTC::getCount();
}

void SystemTime::updateRootTimeStamp(){
	uint32_t now_rtc = RTC::getCount();
	uint32_t ms_passed = RTC::differenceMs(now_rtc, local_time_of_last_received_root_stamp_rtc_ticks);

	high_resolution_time_stamp_t updated_stamp;
	updated_stamp.posix_s = last_received_root_stamp.posix_s + ms_passed / 1000;
	updated_stamp.posix_ms = (last_received_root_stamp.posix_ms + ms_passed) % 1000;

	last_received_root_stamp = updated_stamp;

	// this is where we are currently losing precision
	// because we are storing ms only, and not exact tick count.
	local_time_of_last_received_root_stamp_rtc_ticks += RTC::msToTicks(ms_passed);
}


high_resolution_time_stamp_t SystemTime::getSynchronizedStamp(){
	uint32_t ms_passed = RTC::msPassedSince(local_time_of_last_received_root_stamp_rtc_ticks);

	high_resolution_time_stamp_t stamp;
	// be aware of the bracket placement ;)
	stamp.posix_s = last_received_root_stamp.posix_s + ms_passed / 1000;
	stamp.posix_ms = (last_received_root_stamp.posix_ms + ms_passed) % 1000;
	stamp.version = last_received_root_stamp.version;

	return stamp;
}

uint32_t SystemTime::syncTimeCoroutineAction(){
	LOGSystemTimeDebug("synccoroutine action called");
	static bool is_first_call = true;
	if(is_first_call){
		LOGSystemTimeDebug("is_first_call");
		// reboot occured, wait until sync time is over
		is_first_call = false;
		return Coroutine::delay_ms(reboot_sync_timeout_ms);
	}

	if(reelectionPeriodTimedOut()) {
		LOGSystemTimeDebug("reelectionPeriodTimedOut");
		currentMasterClockId = myId;

		// optionally: return random delay to reduce chance
		// of sync message collision during reelection?
		// E.g.:
		// uint8_t max_delay_ms = 100;
		// return Coroutine::delay_ms(RNG::getInstance().getRandom8() % max_delay_ms);
	}

	if(thisDeviceClaimsMasterClock()){
		LOGSystemTimeDebug("thisDeviceClaimsMasterClock");
		auto stamp = getSynchronizedStamp();
		sendTimeSyncMessage(stamp, myId);
		return Coroutine::delay_ms(master_clock_update_period_ms);
	}

	LOGSystemTimeDebug("syncTimeCoroutineAction did nothing, waiting for reelection (myid=%d, masterid= %d, version=%d)",
			myId, currentMasterClockId, last_received_root_stamp.version);

	// we need to check at least once in a while so that if there are
	// no more sync messages sent, the coroutine will eventually trigger a self promotion.
	return Coroutine::delay_ms(master_clock_reelection_timeout_ms);
}

void SystemTime::onTimeSyncMessageReceive(time_sync_message_t syncmessage){
	LOGSystemTimeDebug("onTimeSyncMessageReceive");
	bool version_has_incremented = Lollipop::compare(
										last_received_root_stamp.version,
										syncmessage.stamp.version,
										time_stamp_version_lollipop_max);
	bool version_is_equal = last_received_root_stamp.version == syncmessage.stamp.version;

	if (version_has_incremented || (version_is_equal && isClockAuthority(syncmessage.root_id))) {
		// sync message wons authority on the clock values.
		LOGSystemTimeVerbose("logging sync message {id=%d, version=%d}, current data {id=%d, version=%d}.",
						syncmessage.root_id,
						syncmessage.stamp.version,
						currentMasterClockId,
						last_received_root_stamp.version
						);
		LOGSystemTimeVerbose("Version increment:%d, Version eq: %d, is authority:%d",
				version_has_incremented,
				version_is_equal,
				isClockAuthority(syncmessage.root_id));
		logRootTimeStamp(syncmessage.stamp, syncmessage.root_id);

		// TODO: could postpone reelection if coroutine interface would be improved
		// sync_routine.reschedule(master_clock_reelection_timeout_ms);

		// TODO: send SYNC_TIME_JUMP event in case of a big difference in time.
		// That way components can react appropriately.
	} else {
		LOGSystemTimeDebug("dropped sync message {id=%d, version=%d}, current data {id=%d, version=%d}",
				syncmessage.root_id,
				syncmessage.stamp.version,
				currentMasterClockId,
				last_received_root_stamp.version
				);
	}

#ifdef DEBUG_SYSTEM_TIME
	pushSyncMessageToTestSuite(syncmessage);
#endif
}

void SystemTime::sendTimeSyncMessage(high_resolution_time_stamp_t stamp, stone_id_t id) {
	LOGSystemTimeDebug("send time sync message");

	cs_mesh_model_msg_time_sync_t timeSyncMsg;
	timeSyncMsg.posix_s  = stamp.posix_s;
	timeSyncMsg.posix_ms = stamp.posix_ms;
	timeSyncMsg.version  = stamp.version;
	timeSyncMsg.overrideRoot = (id == 0);

	LOGSystemTimeDebug("send sync message: %d %d %d %d",
			timeSyncMsg.posix_s,
			timeSyncMsg.posix_ms,
			timeSyncMsg.overrideRoot,
			timeSyncMsg.version);

	cs_mesh_msg_t meshMsg;
	meshMsg.type = CS_MESH_MODEL_TYPE_TIME_SYNC;
	meshMsg.payload = reinterpret_cast<uint8_t*>(&timeSyncMsg);
	meshMsg.size  = sizeof(timeSyncMsg);
	meshMsg.reliability = CS_MESH_RELIABILITY_LOW;
	meshMsg.urgency = CS_MESH_URGENCY_HIGH;

	event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	event.dispatch();
}

bool SystemTime::isClockAuthority(stone_id_t candidate){
	return candidate <= currentMasterClockId;
}

bool SystemTime::thisDeviceClaimsMasterClock(){
	return isClockAuthority(myId);
}

void SystemTime::clearMasterClockId(){
	currentMasterClockId = stone_id_unknown_value;
}

void SystemTime::assertTimeSyncParameters(){
	if (reboot_sync_timeout_ms < 3 * master_clock_update_period_ms) {
		LOGw("reboot delay for sync times is very small compared to sync message update period");
	}
	if (master_clock_reelection_timeout_ms < 3 * master_clock_update_period_ms) {
		LOGw("master clock reelection period is very small compared to sync message update period");
	}
}

bool SystemTime::reelectionPeriodTimedOut(){
	return master_clock_reelection_timeout_ms <=
			RTC::msPassedSince(local_time_of_last_received_root_stamp_rtc_ticks);
}


#ifdef DEBUG_SYSTEM_TIME

void SystemTime::publishSyncMessageForTesting(){
	// we can just send a normal sync message.
	// Implementation is robust against false root clock claims by nature.
	sendTimeSyncMessage(getSynchronizedStamp(), myId);
}

void SystemTime::pushSyncMessageToTestSuite(time_sync_message_t& syncmessage){
	char valuestring [50];

	LOGSystemTimeDebug("push sync message to host: %d %d %d %d",
				syncmessage.stamp.posix_s,
				syncmessage.stamp.posix_ms,
				syncmessage.root_id,
				syncmessage.stamp.version);

	sprintf(valuestring, "%lu,%u,%u,%u",
			syncmessage.stamp.posix_s,
			syncmessage.stamp.posix_ms,
			syncmessage.root_id,
			syncmessage.stamp.version);

	TEST_PUSH_STATIC_S("SystemTime", "timesyncmsg", valuestring);
}
#endif



// ======================== Utility functions ========================

uint32_t SystemTime::posix(){
	auto stamp = getSynchronizedStamp();
	return stamp.version != 0 ? stamp.posix_s : 0;
}

DayOfWeek SystemTime::day(){
	return now().dayOfWeek();
}

Time SystemTime::now(){
	return posix();
}

uint32_t SystemTime::up(){
	return upTimeSec;
}
