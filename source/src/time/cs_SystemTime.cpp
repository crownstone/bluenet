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


/**
 *  Define this symbol for more mesh traffic to facilitate debugging.
 *  Please make sure to keep it out of release builds.
 */
#undef DEBUG_SYSTEM_TIME

// Check for debug symbol consistency
#ifdef DEBUG_SYSTEM_TIME
	#ifndef DEBUG
		#error "SystemTime Debug must be turned off in release"
	#else
		#pragma message("SystemTime Debug is turned on.")
	#endif  // DEBUG
#endif  // DEBUG_SYSTEM_TIME

// ============== Static member declarations ==============

// runtime variables
uint32_t SystemTime::rtcCountOfLastSecondIncrement = 0;
uint32_t SystemTime::upTimeSec = 0;

uint16_t SystemTime::throttleSetTimeCountdownTicks = 0;
uint16_t SystemTime::throttleSetSunTimesCountdownTicks = 0;

// sync
high_resolution_time_stamp_t SystemTime::rootTime;
uint32_t SystemTime::rtcCountOfLastRootTimeUpdate = 0;
uint32_t SystemTime::uptimeOfLastTimeSyncMessage = 0;
stone_id_t SystemTime::rootClockId = stone_id_init();
stone_id_t SystemTime::myId = stone_id_init();
Coroutine SystemTime::syncTimeCoroutine;
Coroutine SystemTime::debugSyncTimeCoroutine;

// driver details
app_timer_t SystemTime::appTimerData = {{0}};
app_timer_id_t SystemTime::appTimerId = &appTimerData;


// ====================== Constants ======================

constexpr uint32_t SystemTime::reboot_sync_timeout_ms() {
	// Should be larger than re-election timeout.
	return 10 * root_clock_update_period_ms();
}

constexpr uint32_t SystemTime::root_clock_update_period_ms() {
#ifdef DEBUG_SYSTEM_TIME
	return 5 * 1000;
#else
	return 60 * 60 * 1000; // 1 hour
#endif  // DEBUG_SYSTEM_TIME
}

constexpr uint32_t SystemTime::root_clock_reelection_timeout_ms() {
	return 5 * root_clock_update_period_ms();
}

constexpr stone_id_t SystemTime::stone_id_init() {
	// The largest value an id can be, which makes it accept any other stone id as root id.
	return 0xFF;
}

constexpr uint8_t SystemTime::timestamp_version_lollipop_max() {
	// six bit roll over
	return (1 << 6) - 1;
}

constexpr uint32_t SystemTime::debugSyncTimeMessagePeriodMs() {
	return 5*1000;
}


// ============ SystemTime implementation ===========

// ======================================================
// =========== Public function definitions ==============
// ======================================================

void SystemTime::init() {
	initDebug();

	syncTimeCoroutine.action = [](){ return syncTimeCoroutineAction(); };

	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &myId, sizeof(myId));

	Timer::getInstance().createSingleShot(
			appTimerId,
			static_cast<app_timer_timeout_handler_t>(&SystemTime::tick));

	scheduleNextTick();
}

// ======================== Utility functions ========================

uint32_t SystemTime::posix() {
	// Since rootTime is kept up to date, we can just return rootTime.
	return rootTime.version != 0 ? rootTime.posix_s : 0;
}

Time SystemTime::now() {
	return posix();
}

DayOfWeek SystemTime::day() {
	return now().dayOfWeek();
}

uint32_t SystemTime::up(){
	return upTimeSec;
}

high_resolution_time_stamp_t SystemTime::getSynchronizedStamp() {
	uint32_t ms_passed = RTC::msPassedSince(rtcCountOfLastRootTimeUpdate);

	high_resolution_time_stamp_t stamp;
	// be aware of the bracket placement ;)
	stamp.posix_s = rootTime.posix_s + ms_passed / 1000;
	stamp.posix_ms = (rootTime.posix_ms + ms_passed) % 1000;
	stamp.version = rootTime.version;
	return stamp;
}

// ======================== timing driver stuff ========================

void SystemTime::scheduleNextTick() {
	Timer::getInstance().start(
			appTimerId,
			MS_TO_TICKS(TICK_TIME_MS),
			nullptr);
}

void SystemTime::tick(void*) {
	// Work with the same RTC count in all this code.
	uint32_t rtcCount = RTC::getCount();

	static bool firstTickCall = true;
	if (firstTickCall) {
		firstTickCall = false;
		rtcCountOfLastSecondIncrement = rtcCount;

		setRootTimeStamp(high_resolution_time_stamp_t(), myId, rtcCount);
	}

	if (RTC::difference(rtcCount, rtcCountOfLastSecondIncrement) >= RTC::msToTicks(1000)) {
		// At least 1 second has passed!
		rtcCountOfLastSecondIncrement += RTC::msToTicks(1000);
		upTimeSec += 1;

		updateRootTimeStamp(rtcCount);

		TYPIFY(STATE_TIME) stateTime = rootTime.posix_s;
		LOGSystemTimeVerbose("set state time ts=%u", stateTime);
		State::getInstance().set(CS_TYPE::STATE_TIME, &stateTime, sizeof(stateTime));
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

void SystemTime::setTime(uint32_t time, bool throttled, bool sendToMesh) {
	LOGSystemTimeDebug("setTime posix=%u throttled=%d sendToMesh=%d", time, throttled, sendToMesh);
	if (time == 0) {
		return;
	}

	if (throttled && throttleSetTimeCountdownTicks) {
		LOGSystemTimeDebug("setTime throttled");
		return;
	}
	throttleSetTimeCountdownTicks = THROTTLE_SET_TIME_TICKS;

	uint32_t rtcCount = RTC::getCount();

	TimeOfDay t(time);
	LOGi("Set time to %u %02d:%02d:%02d", time, t.h(), t.m(), t.s());

	publishSyncMessageForTesting();

	uint32_t prevtime = posix();

	high_resolution_time_stamp_t stamp;
	stamp.posix_s = time;
	stamp.posix_ms = 0;
	stamp.version = Lollipop::next(rootTime.version, timestamp_version_lollipop_max());

	// Setting root id to 0 is a brutal claim to be root.
	// It results in no crownstone sending time sync messages
	// anymore until the root_clock_reelection_timeout_ms expires.
	// It strictly enforces the synchronization among crownstones
	// because all nodes, even the true root clock, will update
	// their local time.
	setRootTimeStamp(stamp, 0, rtcCount);

	if (sendToMesh) {
		sendTimeSyncMessage(getSynchronizedStamp(), 0);
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
		LOGSystemTimeVerbose("setSunTimes throttled");
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
	syncTimeCoroutine.handleEvent(event);
	debugSyncTimeCoroutine.handleEvent(event);

	switch (event.type) {
		case CS_TYPE::CMD_SET_TIME: {
			LOGSystemTimeDebug("set time from command source: type=%u id=%u", event.source.source.type, event.source.source.id);
			uint32_t time = *((TYPIFY(CMD_SET_TIME)*)event.data);
			bool sendToMesh = isOnlyReceiveByThisDevice(event.source);
			setTime(time, true, sendToMesh);
			break;
		}
		case CS_TYPE::CMD_TEST_SET_TIME: {
			LOGd("set test time");
			// Simply set root time, but don't let other crownstones know.
			high_resolution_time_stamp_t stamp;
			stamp.posix_s = *((TYPIFY(CMD_SET_TIME)*)event.data);
			stamp.posix_ms = 0;
			stamp.version = rootTime.version;
			setRootTimeStamp(stamp, myId, RTC::getCount());
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
					// don't use rootClockId here, we don't want to
					// be an impostor as we might have drifted a bit.
					sendTimeSyncMessage(getSynchronizedStamp(), myId);
				}
			}
			break;
		}
		default: {}
	}
}

// ======================== Synchronization ========================

uint8_t SystemTime::timeStampVersion() {
	return rootTime.version;
}

void SystemTime::setRootTimeStamp(high_resolution_time_stamp_t stamp, stone_id_t id, uint32_t rtcCount) {
	LOGSystemTimeDebug("setRootTimeStamp, posix=%u ms=%u version=%u id=%u", stamp.posix_s, stamp.posix_ms, stamp.version, id);

	rootClockId = id;
	rootTime = stamp;
	rtcCountOfLastRootTimeUpdate = rtcCount;
}

void SystemTime::updateRootTimeStamp(uint32_t rtcCount) {
	uint32_t msPassed = RTC::differenceMs(rtcCount, rtcCountOfLastRootTimeUpdate);

	rootTime.posix_s += msPassed / 1000;
	rootTime.posix_ms = (rootTime.posix_ms + msPassed) % 1000;

	rtcCountOfLastRootTimeUpdate = rtcCount;
}

uint32_t SystemTime::syncTimeCoroutineAction() {
	LOGSystemTimeDebug("syncTimeCoroutineAction");
	static bool firstCoroutineCall = true;
	if (firstCoroutineCall) {
		LOGSystemTimeDebug("first call");
		// reboot occured, wait until sync time is over
		firstCoroutineCall = false;
		return Coroutine::delayMs(reboot_sync_timeout_ms());
	}

	if (reelectionPeriodTimedOut()) {
		LOGSystemTimeDebug("reelectionPeriodTimedOut");
		rootClockId = myId;

		// optionally: return random delay to reduce chance
		// of sync message collision during reelection?
		// E.g.:
		// uint8_t max_delay_ms = 100;
		// return Coroutine::delay_ms(RNG::getInstance().getRandom8() % max_delay_ms);
	}

	if (meIsRootClock()) {
		LOGSystemTimeDebug("meIsRootClock");
		auto stamp = getSynchronizedStamp();
		sendTimeSyncMessage(stamp, myId);
		return Coroutine::delayMs(root_clock_update_period_ms());
	}

	LOGSystemTimeDebug("syncTimeCoroutineAction did nothing, waiting for reelection (myId=%u, rootId=%u, version=%u)",
			myId, rootClockId, rootTime.version);

	// we need to check at least once in a while so that if there are
	// no more sync messages sent, the coroutine will eventually trigger a self promotion.
	return Coroutine::delayMs(root_clock_reelection_timeout_ms());
}

void SystemTime::onTimeSyncMessageReceive(time_sync_message_t syncMessage) {
	uint32_t rtcCount = RTC::getCount();
	bool versionIsNewer = Lollipop::isNewer(rootTime.version, syncMessage.stamp.version, timestamp_version_lollipop_max());
	bool versionIsEqual = rootTime.version == syncMessage.stamp.version;

	LOGSystemTimeDebug("onTimeSyncMsg msg: {id=%u version=%u posix=%u} cur: {id=%u version=%u posix=%u}",
			syncMessage.rootId,
			syncMessage.stamp.version,
			syncMessage.stamp.posix_s,
			rootClockId,
			rootTime.version,
			rootTime.posix_s
	);
	LOGSystemTimeDebug("isNewer=%d isEqual=%d isRoot=%d isRebootTimedOut=%d",
			versionIsNewer,
			versionIsEqual,
			isRootClock(syncMessage.rootId),
			rebootTimedOut()
	);

	if (versionIsNewer || (versionIsEqual && isRootClock(syncMessage.rootId)) || (rootTime.version == 0 && !rebootTimedOut())) {
		// sync message wons authority on the clock values.
		setRootTimeStamp(syncMessage.stamp, syncMessage.rootId, rtcCount);
		uptimeOfLastTimeSyncMessage = upTimeSec;

		// TODO: could postpone reelection if coroutine interface would be improved
		// sync_routine.reschedule(root_clock_reelection_timeout_ms);

		// TODO: send SYNC_TIME_JUMP event in case of a big difference in time.
		// That way components can react appropriately.
	}
	else {
		LOGSystemTimeDebug("dropped");
	}

	pushSyncMessageToTestSuite(syncMessage);
}

void SystemTime::sendTimeSyncMessage(high_resolution_time_stamp_t stamp, stone_id_t id) {
	LOGSystemTimeDebug("send time sync message");

	cs_mesh_model_msg_time_sync_t timeSyncMsg;
	timeSyncMsg.posix_s  = stamp.posix_s;
	timeSyncMsg.posix_ms = stamp.posix_ms;
	timeSyncMsg.version  = stamp.version;
	timeSyncMsg.overrideRoot = (id == 0);

	LOGSystemTimeDebug("send sync message: s=%d ms=%d version=%d override=%d",
			timeSyncMsg.posix_s,
			timeSyncMsg.posix_ms,
			timeSyncMsg.version,
			timeSyncMsg.overrideRoot);

	cs_mesh_msg_t meshMsg;
	meshMsg.type = CS_MESH_MODEL_TYPE_TIME_SYNC;
	meshMsg.payload = reinterpret_cast<uint8_t*>(&timeSyncMsg);
	meshMsg.size  = sizeof(timeSyncMsg);
	meshMsg.reliability = CS_MESH_RELIABILITY_LOW;
	meshMsg.urgency = CS_MESH_URGENCY_HIGH;

	event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	event.dispatch();
}

// ======================= Predicates =======================

bool SystemTime::isRootClock(stone_id_t candidate) {
	return candidate <= rootClockId;
}

bool SystemTime::meIsRootClock() {
	return isRootClock(myId);
}



bool SystemTime::reelectionPeriodTimedOut() {
	return root_clock_reelection_timeout_ms() / 1000 <= upTimeSec - uptimeOfLastTimeSyncMessage;
}

bool SystemTime::rebootTimedOut() {
	return reboot_sync_timeout_ms() / 1000 <= upTimeSec;
}


bool SystemTime::isOnlyReceiveByThisDevice(cmd_source_with_counter_t counted_source) {
	auto src = counted_source.source;
	if (src.flagExternal) {
		return false;
	}

	return src.type == CS_CMD_SOURCE_TYPE_UART
			|| (src.type == CS_CMD_SOURCE_TYPE_ENUM
					&&  src.id == CS_CMD_SOURCE_CONNECTION);
}




// ==========================================
// =========== Debug functions ==============
// ==========================================

void SystemTime::initDebug() {
#ifdef DEBUG_SYSTEM_TIME
	debugSyncTimeCoroutine.action = [](){
			LOGd("debug sync time");
			publishSyncMessageForTesting();
			return Coroutine::delayMs(debugSyncTimeMessagePeriodMs());
		};
#endif  // DEBUG_SYSTEM_TIME
}

void SystemTime::publishSyncMessageForTesting(){
#ifdef DEBUG_SYSTEM_TIME
	// we can just send a normal sync message.
	// Implementation is robust against false root clock claims by nature.
//	sendTimeSyncMessage(getSynchronizedStamp(), myId);
#endif // DEBUG_SYSTEM_TIME
}

void SystemTime::pushSyncMessageToTestSuite(time_sync_message_t& syncmessage){
#ifdef DEBUG_SYSTEM_TIME
	char valuestring [50];

	LOGSystemTimeDebug("push sync message to host: %d %d %d %d",
				syncmessage.stamp.posix_s,
				syncmessage.stamp.posix_ms,
				syncmessage.rootId,
				syncmessage.stamp.version);

	sprintf(valuestring, "%lu,%u,%u,%u",
			syncmessage.stamp.posix_s,
			syncmessage.stamp.posix_ms,
			syncmessage.rootId,
			syncmessage.stamp.version);

	TEST_PUSH_STATIC_S("SystemTime", "timesyncmsg", valuestring);
#endif  // DEBUG_SYSTEM_TIME
}

