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
	 * Get the current time as posix timestamp.
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

	virtual void handleEvent(event_t& event);

	/**
	 * Set the system wide time to given posix timestamp.
	 * Dispatches EVT_TIME_SET.
	 * Also sets it to state.
	 *
	 * @param[in] time       Posix time in seconds.
	 * @param[in] throttled  When true, a new suntime won't be allowed to be set for a while.
	 *                       Unless it's set with throttled false.
	 */
	static void setTime(uint32_t time, bool throttled = true);

	/**
	 * Set the sunrise and sunset times.
	 * Also sets it to state.
	 *
	 * @param[in] sunTimes   Sun rise and set times.
	 * @param[in] throttled  When true, a new suntime won't be allowed to be set for a while.
	 *                       Unless it's set with throttled false.
	 */
	static cs_ret_code_t setSunTimes(const sun_time_t& sunTimes, bool throttled = true);

	/**
	 * Creates and starts the first tick timer.
	 */
	static void init();

private:
	// state data
	static uint32_t rtcTimeStamp;       // high resolution device local time
	static uint32_t uptime_sec;

	// throttling: when not 0, block command
	static uint16_t throttleSetTimeCountdownTicks;
	static uint16_t throttleSetSunTimesCountdownTicks;

	// timing features
	static app_timer_t              appTimerData;
	static app_timer_id_t           appTimerId;

	static void scheduleNextTick();
	static void tick(void* unused);

	// settings
	static constexpr auto TICK_TIME_MS = 500;

	// Time shouldn't differ more than 1 minute.
	static constexpr uint16_t THROTTLE_SET_TIME_TICKS = (60 * 1000 / TICK_TIME_MS);

	// Sun time shouldn't differ more than 30 minutes.
	static constexpr uint16_t THROTTLE_SET_SUN_TIMES_TICKS = (30 * 60 * 1000 / TICK_TIME_MS);


	// ===================== mesh posix time sync implementation =====================
	// timeout period before considering device should have had updates
	// during this period the device will not participate for election
	// even if it has received a valid sync message from another device
	// and has lower crownstone id.
	static constexpr uint32_t reboot_sync_timeout_ms = ;

	// period of sync messages sent by the master clock
	static constexpr uint32_t master_clock_update_period_ms = ;

	// time out period for master clock invalidation.
	// not hearing sync messages for this amount of time will invalidate the
	// current master clock id.
	// should be larger than master_clock_update_period_ms by a fair margin.
	static constexpr uint32_t master_clock_reelection_period_ms = 60* 1000;

	static constexpr uint32_t master_clock_reset_value = 0xffffffff;

	static uint32_t posixTimeStamp; // old time stamp implementation.

	time_sync_message_t lastMasterClockSyncMessage;
	high_resolution_time_stamp_t local_time; // (updated in the tick method, adjusted when sync happens)
	uint32_t lastSyncMessageLocalRtcTickCount; // when we are the master clock, this contains our tick count at last sync message generation.

	Coroutine syncTimeCoroutine;

	uint32_t syncTimeCoroutineAction();
	void onTimeSyncMessageReceive(time_sync_message_t syncmessage);

	/**
	 * Send a sync message as if we are the master clock.
	 */
	void sendTimeSyncMessage();

	void clearMasterClockId();
	void assertTimeSyncParameters();

	/**
	 * Returns true if the device id of this device is lower than or equal to the currentMasterClockId.
	 */
	bool thisDeviceClaimsMasterClock();

	/**
	 * Returns true if the candidate is considered a clock authority relative to us.
	 */
	bool isClockAuthority(stone_id_t candidate);

	/**
	 * Returns true if onTimeSyncMessageReceive hasn't received any sync messages from a
	 * clock authority in the last master_clock_reelection_timeout_ms miliseconds.
	 */
	bool reelectionPeriodTimedOut();
};

