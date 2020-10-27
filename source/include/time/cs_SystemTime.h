/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <drivers/cs_Timer.h>
#include <events/cs_EventListener.h>

#include <protocol/cs_Typedefs.h>

#include <time/cs_Time.h>
#include <time/cs_TimeOfDay.h>
#include <time/cs_TimeSyncMessage.h>

#include <util/cs_Coroutine.h>

#include <stdint.h>

/**
 * This class keeps track of the real time in the current time zone.
 * It may obtain its data through the mesh, or some other way and try
 * to keep up to date using on board timing functionality.
 */
class SystemTime : public EventListener {
public:
	/**
	 * Creates and starts the first tick timer.
	 */
	static void init();

	virtual void handleEvent(event_t& event);

	// ======================== Utility functions ========================

	/**
	 * Get the current time as posix timestamp in seconds.
	 *
	 * If the synchronized stamp has version 0, this
	 * function returns 0.
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

	/**
	 * Returns the current time, computed from the last received root stamp
	 * by adding the difference between the local rtc time upon reception
	 * and the current local rtc time.
	 *
	 * When version == 0, this stamp contains a synchronized value which is
	 * not posix based. Nonetheless it's seconds an miliseconds fields are
	 * the current best synchronized values known accross the mesh.
	 */
	static high_resolution_time_stamp_t getSynchronizedStamp();

	// ======================== Setters ========================

	/**
	 * Set the system wide time to given posix timestamp.
	 * Dispatches EVT_TIME_SET.
	 * Also sets it to state.
	 *
	 * @param[in] time            Posix time in seconds.
	 * @param[in] throttled       When true, a new suntime won't be allowed to be set for a while.
	 *                            Unless it's set with throttled false.
	 * @param[in] sendToMesh      Setting this to false will prevent updating other mesh nodes
	 */
	static void setTime(uint32_t time, bool throttled, bool sendToMesh);

	/**
	 * Set the sunrise and sunset times.
	 * Also sets it to state.
	 *
	 * @param[in] sunTimes   Sun rise and set times.
	 * @param[in] throttled  When true, a new suntime won't be allowed to be set for a while.
	 *                       Unless it's set with throttled false.
	 */
	static cs_ret_code_t setSunTimes(const sun_time_t& sunTimes, bool throttled = true);

private:
	// ========================== Run time data and constants ============================
	// state data
	static uint32_t upTimeSec;

	// throttling: when not 0, block command
	static uint16_t throttleSetTimeCountdownTicks;
	static uint16_t throttleSetSunTimesCountdownTicks;

	// settings
	static constexpr auto TICK_TIME_MS = 500;

	// Time shouldn't differ more than 1 minute.
	static constexpr uint16_t THROTTLE_SET_TIME_TICKS = (60 * 1000 / TICK_TIME_MS);

	// Sun time shouldn't differ more than 30 minutes.
	static constexpr uint16_t THROTTLE_SET_SUN_TIMES_TICKS = (30 * 60 * 1000 / TICK_TIME_MS);

	// timing features
	static app_timer_t              appTimerData;
	static app_timer_id_t           appTimerId;

	static void scheduleNextTick();
	static void tick(void* unused);

	// ===================== mesh posix time sync implementation =====================

	// ---------------- constants ----------------

	/**
	 * Timeout period before considering device should have had time sync messages.
	 *
	 * During this period the device will not participate for election, even if it has
	 * received a valid sync message from another device and has lower crownstone id.
	 */
	static constexpr uint32_t reboot_sync_timeout_ms();

	/**
	 * Time between sync messages from the root clock.
	 */
	static constexpr uint32_t root_clock_update_period_ms();

	/**
	 * If no sync message has been received from the root clock for this time, a new root clock will be selected.
	 */
	static constexpr uint32_t root_clock_reelection_timeout_ms();

	/**
	 * Stone id to use for initialization.
	 */
	static constexpr stone_id_t stone_id_init();

	/**
	 * The time version is lollipop versioned. This constant defines its roll over point.
	 */
	static constexpr uint8_t timestamp_version_lollipop_max();


	// -------------- runtime variables ----------------

	/**
	 * RTC count of last time a second had passed.
	 */
	static uint32_t rtcCountOfLastSecondIncrement;

	/**
	 * Cached ID of this stone.
	 */
	static stone_id_t myId;

	/**
	 * Root clock.
	 */
	static high_resolution_time_stamp_t rootTime;

	/**
	 * RTC count of last time the root clock was set.
	 */
	static uint32_t rtcCountOfLastRootTimeUpdate;

	/**
	 * Uptime of last time a sync message was received.
	 */
	static uint32_t uptimeOfLastTimeSyncMessage;

	/**
	 * Stone ID of the root clock.
	 */
	static stone_id_t rootClockId;

	static Coroutine syncTimeCoroutine;

	// ------------------ Method definitions ------------------

	static uint32_t syncTimeCoroutineAction();

	static uint8_t timeStampVersion();

	static void onTimeSyncMessageReceive(time_sync_message_t syncmessage);
	static void setRootTimeStamp(high_resolution_time_stamp_t stamp, stone_id_t id, uint32_t rtcCount);

	// adjusts the local_time_of_last_received_root_stamp_rtc_ticks
	// and last_received_root_stamp accordingly. Calling this function
	// every now and then is necessary when this crownstone claims root
	// clock in order to prevent roll over issues,
	// but it currently looses some precision which is tricky to avoid,
	// so don't call it too often.
	static void updateRootTimeStamp(uint32_t rtcCount);

	/**
	 * Send a sync message for given stamp/id combo to the mesh.
	 */
	static void sendTimeSyncMessage(high_resolution_time_stamp_t stamp, stone_id_t id);

	/**
	 * Returns true if the id of this stone is considered to be root clock.
	 */
	static bool meIsRootClock();

	/**
	 * Returns true if the candidate is considered to be root clock.
	 */
	static bool isRootClock(stone_id_t candidate);

	/**
	 * Returns true if the command source is uart or ble through connection.
	 */
	bool isOnlyReceiveByThisDevice(cmd_source_with_counter_t counted_source);

	/**
	 * Returns true if onTimeSyncMessageReceive hasn't received any sync messages from a
	 * clock authority in the last root_clock_reelection_timeout_ms milliseconds.
	 */
	static bool reelectionPeriodTimedOut();

	/**
	 * Returns true when reboot period is passed.
	 */
	static bool rebootTimedOut();


	// ==========================================
	// =========== Debug functions ==============
	// ==========================================

	static constexpr uint32_t debugSyncTimeMessagePeriodMs();
	static Coroutine debugSyncTimeCoroutine;

	/**
	 * Sets up the debug symc time coroutine when DEBUG_SYSTEM_TIME
	 * is defined in order to broadcast sync messages to the mesh
	 * regardless of being root clock or not.
	 */
	static void initDebug();

	static void publishSyncMessageForTesting();
	static void pushSyncMessageToTestSuite(time_sync_message_t& syncmessage);
};

