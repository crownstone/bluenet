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
 *
 * This class also synchronizes time with other nodes in the mesh.
 * The node with the lowest ID is considered to be the root clock.
 * Other nodes will update their time on receival of time sync messages from the root clock.
 *
 * There are some scenarios to consider here:
 * 1. No node knows the actual posix time: in this case we still want a synchronized clock.
 * 2. Nodes are all just booted, but not at the same time: they should still synchronize quickly.
 * 3. Nodes are all booted at the same time.
 * 4. A node that's not the root clock receives a set time command, this time should overrule the root clock time.
 *
 * This is solved by:
 * 1. Update the clock even when no posix time is known, but mark it to be invalid with a low version.
 * 2. Only consider ourselves to be root ID after the first time sync message is received.
 * 2. On boot, each node will request to be synchronized (a mechanism already in the mesh). Every other node that
 *    considers themselves to be in sync, will send their clock.
 *    It's up to the receiving node to device which clock to use.
 *    Nodes will consider themselves to be in sync after hearing 1 time sync message, or after boot timeout.
 * 3. In this case we expect all nodes to be waiting for the first message to be sent.
 * 4. Add a lollipop version to the clock: a newer version will always be accepted, even if it's not the root ID.
 *
 * For robustness, not only the root clock node, but all nodes will regularly send a time sync message.
 * It's up to the receiving node to device which clock is the root clock.
 * Not sure if this is necessary.
 */
class SystemTime : public EventListener {
public:
	/**
	 * Creates and starts the first tick timer.
	 */
	void init();

	virtual void handleEvent(event_t& event);

	// ======================== Utility functions ========================

	/**
	 * Get the current time as posix timestamp in seconds.
	 *
	 * Returns 0 if no valid posix time is set.
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
	 * Get the synchronized timestamp, updated to current time.
	 *
	 * When version == 0, the timestamp is not posix based.
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
	 * @return ERR_SUCCESS             When time is set successfully.
	 * @return ERR_BUSY                When set time is throttled, try again later.
	 * @return ERR_WRONG_PARAMETER     When trying to set time to 0.
	 */
	static cs_ret_code_t setTime(uint32_t time, bool throttled, bool sendToMesh);

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

	// Time stamp will be updated at a low rate, to maintain precision. Must be lower than RTC overflow time.
	static constexpr uint16_t TIME_UPDATE_PERIOD_MS = (60 * 1000);

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
	 * Timeout period before considering this stone to be root clock.
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

	/**
	 * Lowest time version at which a valid posix time is set.
	 */
	static constexpr uint8_t timestamp_version_min_valid();


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
	 *
	 * Updated at a low rate, use a function to get an up to date timestamp.
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

	/**
	 * Keep up the root clock time.
	 *
	 * Should be called at least every second.
	 * But it currently looses some precision, so don't call it too often.
	 */
	static void updateRootTimeStamp(uint32_t rtcCount);

	/**
	 * Send a sync message for given stamp/id combo to the mesh.
	 */
	static void sendTimeSyncMessage(high_resolution_time_stamp_t stamp, stone_id_t id, bool reliable = false);

	/**
	 * Returns true if the id of this stone can considered to be root clock.
	 */
	static bool meIsRootClock();

	/**
	 * Returns true if the candidate can considered to be root clock.
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

