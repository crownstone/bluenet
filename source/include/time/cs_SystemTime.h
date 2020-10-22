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

#ifdef DEBUG
// Define this symbol for more mesh traffic but allow for easier debugging.
// Please make sure to keep it out of release builds.
#undef DEBUG_SYSTEM_TIME
#endif

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
	 * @param[in] unsynchronize   setting this to true will prevent updating other mesh nodes
	 *                            This enables purposefully unsynchronizing this node for testing.
	 */
	static void setTime(uint32_t time, bool throttled = true, bool unsynchronize = false);

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

	// -------------- runtime variables and constants ----------------

	// timeout period before considering device should have had updates
	// during this period the device will not participate for election
	// even if it has received a valid sync message from another device
	// and has lower crownstone id.
	static constexpr uint32_t reboot_sync_timeout_ms = 60 * 1000;

	/**
	 * timing settings for the algorithm.
	 * - update period defines the time between two sync messages from the master clock.
	 * - reelection period defines the time a crownstone will wait before claiming to
	 *   be a master clock itself and start sending sync messages.
	 *
	 * Note: reelection should be larger than master_clock_update_period_ms
	 * by a fair margin. This will be checked at startup in ::assertTimeSyncParameters.
	 */
#ifdef DEBUG_SYSTEM_TIME
	// period of sync messages sent by the master clock in debug builds
	static constexpr uint32_t master_clock_update_period_ms = 5* 1000;
	static constexpr uint32_t master_clock_reelection_timeout_ms = 60 * 1000;
#else
	// period of sync messages sent by the master clock in release builds
	static constexpr uint32_t master_clock_update_period_ms = 60*60* 1000;
	static constexpr uint32_t master_clock_reelection_timeout_ms = 5 * master_clock_update_period_ms;
#endif

	static constexpr uint32_t stone_id_unknown_value = 0xff;

	static constexpr uint8_t time_stamp_version_lollipop_max = (2 << 6) - 1;

	// kept track off in ::tick(void*), seeded at first call for best accuracy.
	static uint32_t last_statetimeevent_stamp_rtc;

	// clock synchronization data (updated on sync)
	static high_resolution_time_stamp_t last_received_root_stamp;
	static uint32_t local_time_of_last_received_root_stamp_rtc_ticks;
	static stone_id_t currentMasterClockId;
	static stone_id_t myId;

	static Coroutine syncTimeCoroutine;

	// ------------------ Method definitions ------------------

	static uint32_t syncTimeCoroutineAction();
	static void onTimeSyncMessageReceive(time_sync_message_t syncmessage);
	static void logRootTimeStamp(high_resolution_time_stamp_t stamp, stone_id_t id);

	// adjusts the local_time_of_last_received_root_stamp_rtc_ticks
	// and last_received_root_stamp accordingly. Calling this function
	// every now and then is necessary when this crownstone claims root
	// clock in order to prevent roll over issues,
	// but it currently looses some precision which is tricky to avoid,
	// so don't call it too often.
	static void updateRootTimeStamp();

	/**
	 * Send a sync message for given stamp/id combo to the mesh.
	 */
	static void sendTimeSyncMessage(high_resolution_time_stamp_t stamp, stone_id_t id);

	static void clearMasterClockId();
	static void assertTimeSyncParameters();

	/**
	 * Returns true if the device id of this device is lower than or equal to the currentMasterClockId.
	 */
	static bool thisDeviceClaimsMasterClock();

	/**
	 * Returns true if the candidate is considered a clock authority relative to us.
	 */
	static bool isClockAuthority(stone_id_t candidate);

	static inline uint8_t timeStampVersion() { return last_received_root_stamp.version; }

	/**
	 * Returns true if the command source is uart or ble through connection.
	 */
	bool isOnlyReceiveByThisDevice(cmd_source_with_counter_t counted_source);

	/**
	 * Returns true if onTimeSyncMessageReceive hasn't received any sync messages from a
	 * clock authority in the last master_clock_reelection_timeout_ms miliseconds.
	 */
	static bool reelectionPeriodTimedOut();


	// ==========================================
	// =========== Debug functions ==============
	// ==========================================

#ifdef DEBUG_SYSTEM_TIME
	static Coroutine debugSyncTimeCoroutine;
	static constexpr uint32_t debugSyncTimeMessagePeriodMs = 5*1000;
#endif

	static void publishSyncMessageForTesting();
	static void pushSyncMessageToTestSuite(time_sync_message_t& syncmessage);
};

