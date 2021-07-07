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
#include <logging/cs_Logger.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <time/cs_Time.h>
#include <time/cs_TimeOfDay.h>
#include <time/cs_TimeSyncMessage.h>
#include <test/cs_Test.h>
#include <util/cs_Lollipop.h>

#define LOGSystemTimeInfo   LOGd
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
	// After the mesh sync process, we should have received all the
	// time sync messages we need to have the correct time.
	return MESH_SYNC_GIVE_UP_MS;
}

constexpr uint32_t SystemTime::root_clock_update_period_ms() {
#ifdef DEBUG_SYSTEM_TIME
	return 5 * 1000;
#else
	return 20 * 60 * 1000; // 20 minutes
#endif  // DEBUG_SYSTEM_TIME
}

constexpr uint32_t SystemTime::root_clock_reelection_timeout_ms() {
	// Chances of missing 10 messages should be low.
	// From a test: 57% of msgs received, with a network of 2 nodes at 0.5m distance.
	// So chance of missing 10 msgs would be: 0.43^10 = 0.0002
	return 10 * root_clock_update_period_ms();
}

constexpr stone_id_t SystemTime::stone_id_init() {
	// The largest value an id can be, which makes it accept any other stone id as root id.
	return 0xFF;
}

constexpr uint8_t SystemTime::timestamp_version_lollipop_max() {
	// six bit roll over
	return (1 << 6) - 1;
}

constexpr uint8_t SystemTime::timestamp_version_min_valid() {
	return 1;
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

	// Start the root clock, but accept clock of other IDs.
	uint32_t rtcCount = RTC::getCount();
	rtcCountOfLastSecondIncrement = rtcCount;
	setRootTimeStamp(high_resolution_time_stamp_t(), stone_id_init(), rtcCount);

	scheduleNextTick();
}

// ======================== Utility functions ========================

uint32_t SystemTime::posix() {
	if (rootTime.version  < timestamp_version_min_valid()) {
		return 0;
	}

	// Base on up to date timestamp.
	return getSynchronizedStamp().posix_s;
}

Time SystemTime::now() {
	return Time(posix());
}

DayOfWeek SystemTime::day() {
	return now().dayOfWeek();
}

uint32_t SystemTime::up() {
	return upTimeSec;
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

	if (RTC::difference(rtcCount, rtcCountOfLastRootTimeUpdate) >= RTC::msToTicks(TIME_UPDATE_PERIOD_MS)) {
		updateRootTimeStamp(rtcCount);
	}

	if (RTC::difference(rtcCount, rtcCountOfLastSecondIncrement) >= RTC::msToTicks(1000)) {
		// At least 1 second has passed!
		rtcCountOfLastSecondIncrement += RTC::msToTicks(1000);
		upTimeSec += 1;
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
	LOGSystemTimeInfo("setTime posix=%u throttled=%d sendToMesh=%d", time, throttled, sendToMesh);
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
	// TODO: set at least to timestamp_version_min_valid()
	stamp.version = Lollipop::next(rootTime.version, timestamp_version_lollipop_max());

	// Setting root id to 0 is a brutal claim to be root.
	// It results in no crownstone claiming to be root, until re-election timeout.
	// It enforces the synchronization among crownstones because all nodes,
	// even the true root clock, will update their local time.
	setRootTimeStamp(stamp, 0, rtcCount);

	if (sendToMesh) {
		// Send more reliable message.
		// TODO: this sends the message multiple times without updating the timestamp in the message.
		// It would be better to let this node be the root clock for some time or so.
		sendTimeSyncMessage(getSynchronizedStamp(), 0, true);
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
			LOGSystemTimeInfo("set time from command source: type=%u id=%u", event.source.source.type, event.source.source.id);
			uint32_t time = *((TYPIFY(CMD_SET_TIME)*)event.data);
			bool sendToMesh = isOnlyReceiveByThisDevice(event.source);
			setTime(time, true, sendToMesh);
			break;
		}
		case CS_TYPE::CMD_TEST_SET_TIME: {
			// brainwash this crownstone and don't let other crownstones know,
			// this might be an intentional desync.
			uint32_t posixtime = *((TYPIFY(CMD_SET_TIME)*)event.data);
			LOGSystemTimeInfo("set test time %u", posixtime);
			setTime(posixtime, false, false);
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
			// Keep requesting the time until a valid posix time is received.
			if (timeStampVersion() < timestamp_version_min_valid()) {
				auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_OUTGOING)*>(event.data);
				req->bits.time = true;
			}
			break;
		}
		case CS_TYPE::EVT_MESH_SYNC_REQUEST_INCOMING: {
			auto req = reinterpret_cast<TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING)*>(event.data);
			if (req->bits.time) {
				// Time is requested by a crownstone in the mesh: send our time.
				// It doesn't matter whether we have the correct time or whether we are the root clock:
				// the receiving node will select which clock to use.

				// But only send a message when we should be in sync with the rest of the network: after boot timeout.
				if (rebootTimedOut()) {
					// Since we send the message with a low reliability, it doesn't flood the mesh that much.
					// Send with a 1/4 chance, to prevent flooding the mesh.
					if (RNG::getInstance().getRandom8() < (255 / 4 + 1)) {
						sendTimeSyncMessage(getSynchronizedStamp(), myId);
					}
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

	// Clock should go msPassed forward, this can be multiple seconds.
	uint32_t secondsIncrement = (rootTime.posix_ms + msPassed) / 1000;
	rootTime.posix_ms =         (rootTime.posix_ms + msPassed) - 1000 * secondsIncrement; // Same as (rootTime.posix_ms + msPassed) % 1000.
	rootTime.posix_s += secondsIncrement;

	LOGSystemTimeDebug("updateRootTimeStamp s=%u ms=%u", rootTime.posix_s, rootTime.posix_ms);
	rtcCountOfLastRootTimeUpdate = rtcCount;
}

high_resolution_time_stamp_t SystemTime::getSynchronizedStamp() {
	uint32_t msPassed = RTC::msPassedSince(rtcCountOfLastRootTimeUpdate);

	// Don't update the root clock, as this function can be called many times,
	// which would add up imprecision to the root clock.
	high_resolution_time_stamp_t stamp;

	uint32_t secondsIncrement = (rootTime.posix_ms + msPassed) / 1000;
	stamp.posix_ms =            (rootTime.posix_ms + msPassed) - 1000 * secondsIncrement; // Same as (rootTime.posix_ms + msPassed) % 1000.
	stamp.posix_s = rootTime.posix_s + secondsIncrement;
	stamp.version = rootTime.version;
	LOGSystemTimeVerbose("getSynchronizedStamp s=%u ms=%u version=%u", stamp.posix_s, stamp.posix_ms, stamp.version);
	return stamp;
}

uint32_t SystemTime::syncTimeCoroutineAction() {
	LOGSystemTimeDebug("syncTimeCoroutineAction");

	if (reelectionPeriodTimedOut()) {
		LOGSystemTimeDebug("reelectionPeriodTimedOut");
		rootClockId = myId;
	}

//	if (meIsRootClock()) {
//		LOGSystemTimeDebug("meIsRootClock");
		auto stamp = getSynchronizedStamp();
		sendTimeSyncMessage(stamp, myId);
		return Coroutine::delayMs(root_clock_update_period_ms());
//	}

//	LOGSystemTimeDebug("syncTimeCoroutineAction did nothing, waiting for reelection (myId=%u, rootId=%u, version=%u)",
//			myId, rootClockId, rootTime.version);
//
//	// we need to check at least once in a while so that if there are
//	// no more sync messages sent, the coroutine will eventually trigger a self promotion.
//	return Coroutine::delayMs(root_clock_reelection_timeout_ms());
}

void SystemTime::onTimeSyncMessageReceive(time_sync_message_t syncMessage) {
	uint32_t rtcCount = RTC::getCount();
	bool versionIsNewer = Lollipop::isNewer(rootTime.version, syncMessage.stamp.version, timestamp_version_lollipop_max());
	bool versionIsEqual = rootTime.version == syncMessage.stamp.version;

	if (versionIsNewer || (versionIsEqual && isRootClock(syncMessage.srcId))) {
		// sync message wins authority on the clock values.
		setRootTimeStamp(syncMessage.stamp, syncMessage.srcId, rtcCount);
		uptimeOfLastTimeSyncMessage = upTimeSec;

		// After accepting the first time sync message, we now have a clock that should be in sync with other nodes.
		// So this is a good time to consider ourselves to be the root clock.
		if (meIsRootClock()) {
			LOGSystemTimeDebug("Set me as root: myId=%u rootClockId=%u", myId, rootClockId);
			rootClockId = myId;
		}

		// TODO: could postpone reelection if coroutine interface would be improved
		// sync_routine.reschedule(root_clock_reelection_timeout_ms);

		// TODO: send SYNC_TIME_JUMP event in case of a big difference in time.
		// That way components can react appropriately.
	}
	else {
		LOGSystemTimeDebug("ignored");
	}
	// These prints should be done after setRootTimeStamp(), else they influence the synchronization.
	LOGSystemTimeDebug("onTimeSyncMsg msg: {id=%u version=%u s=%u ms=%u} cur: {id=%u version=%u s=%u ms=%u}",
			syncMessage.srcId,
			syncMessage.stamp.version,
			syncMessage.stamp.posix_s,
			syncMessage.stamp.posix_ms,
			rootClockId,
			rootTime.version,
			getSynchronizedStamp().posix_s,
			getSynchronizedStamp().posix_ms
	);
	LOGSystemTimeDebug("isNewer=%d isEqual=%d isRoot=%d",
			versionIsNewer,
			versionIsEqual,
			isRootClock(syncMessage.srcId)
	);

	pushSyncMessageToTestSuite(syncMessage);
}

void SystemTime::sendTimeSyncMessage(high_resolution_time_stamp_t stamp, stone_id_t id, bool reliable) {
	cs_mesh_model_msg_time_sync_t timeSyncMsg;
	timeSyncMsg.posix_s  = stamp.posix_s;
	timeSyncMsg.posix_ms = stamp.posix_ms;
	timeSyncMsg.version  = stamp.version;
	timeSyncMsg.overrideRoot = (id == 0);

	LOGSystemTimeDebug("sendTimeSyncMessage s=%u ms=%u version=%u override=%d",
			timeSyncMsg.posix_s,
			timeSyncMsg.posix_ms,
			timeSyncMsg.version,
			timeSyncMsg.overrideRoot);

	// Send with lowest reliability, so that the message is only sent once.
	// This means the timestamp that is sent doesn't have to be updated.
	// But we will have to send a sync message more often.
	cs_mesh_msg_t meshMsg;
	meshMsg.type = CS_MESH_MODEL_TYPE_TIME_SYNC;
	meshMsg.payload = reinterpret_cast<uint8_t*>(&timeSyncMsg);
	meshMsg.size  = sizeof(timeSyncMsg);
	if (reliable) {
		meshMsg.reliability = CS_MESH_RELIABILITY_MEDIUM;
	}
	else {
		meshMsg.reliability = CS_MESH_RELIABILITY_LOWEST;
	}
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
	// After receiving the first message, we should already be in sync with the rest of the mesh.
	// Otherwise, wait for reboot timeout.
	return (uptimeOfLastTimeSyncMessage != 0) || (reboot_sync_timeout_ms() / 1000 <= upTimeSec);
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

void SystemTime::publishSyncMessageForTesting() {
#ifdef DEBUG_SYSTEM_TIME
	// we can just send a normal sync message.
	// Implementation is robust against false root clock claims by nature.
	// However, if the implementation is bugged, this might cover up the bug.
	// Message should be marked to be solely for testing.
//	sendTimeSyncMessage(getSynchronizedStamp(), myId);
#endif // DEBUG_SYSTEM_TIME
}

void SystemTime::pushSyncMessageToTestSuite(time_sync_message_t& syncmessage) {
#ifdef DEBUG_SYSTEM_TIME
	char valuestring [50];

	LOGSystemTimeDebug("push sync message to host: %d %d %d %d",
			syncmessage.stamp.posix_s,
			syncmessage.stamp.posix_ms,
			syncmessage.srcId,
			syncmessage.stamp.version);

	sprintf(valuestring, "%lu,%u,%u,%u",
			syncmessage.stamp.posix_s,
			syncmessage.stamp.posix_ms,
			syncmessage.srcId,
			syncmessage.stamp.version);

	TEST_PUSH_STATIC_S("SystemTime", "timesyncmsg", valuestring);
#endif  // DEBUG_SYSTEM_TIME
}

