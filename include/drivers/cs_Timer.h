/*
 * Author: Anne van Rossum
 * Copyrights: Distributed Organism B.V. (DoBots, http://dobots.nl)
 * Date: 21 Apr., 2015
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <util/cs_BleError.h>

#include <cfg/cs_Config.h>

extern "C" {
#include <app_scheduler.h>
}

#define HZ_TO_TICKS(hz) APP_TIMER_TICKS(1000/hz, APP_TIMER_PRESCALER)
#define MS_TO_TICKS(ms) APP_TIMER_TICKS(ms, APP_TIMER_PRESCALER)

/** Timer on top of the timer peripheral.
 */
class Timer {

private:
	Timer() {};

	Timer(Timer const&);
	void operator=(Timer const &);

public:
	static Timer& getInstance() {
		static Timer instance;
		return instance;
	}

	inline void init() {
		APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
		APP_TIMER_APPSH_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, true);
	}

	/** Create single shot timer. function will only be called once and after that timer will be
	 * stopped
	 * @timer_handle            An id or handle to reference the timer, set by this function (actually, just a Uint32_t)
	 * @func                    The function to be called
	 *
	 * Create a timer for a specific purpose.
	 */
	inline void createSingleShot(app_timer_id_t& timer_handle, app_timer_timeout_handler_t func) {
		BLE_CALL(app_timer_create, (&timer_handle, APP_TIMER_MODE_SINGLE_SHOT, func));
	}


	/** Start a previously created timer
	 * @timer_handle            Reference to previously created timer
	 * @ticks                   Number of ticks till timeout (minimum is 5)
	 * @obj                     Reference to the object on which the function should be executed
	 */
	inline void start(app_timer_id_t& timer_handle, uint32_t ticks, void* obj) {
		if (ticks < APP_TIMER_MIN_TIMEOUT_TICKS) {
			LOGe("Tried to start a timer with %d ticks");
			return;
		}
		BLE_CALL(app_timer_start, (timer_handle, ticks, obj));
	}

	/** Stop a timer
	 * @timer_handle            Reference to previously created timer
	 */
	inline void stop(app_timer_id_t& timer_handle) {
		BLE_CALL(app_timer_stop, (timer_handle));
	}

	/** Resets a timer (if already active) to the new ticks
	 * @timer_handle            Reference to previously created timer
	 * @ticks                   Number of ticks till timeout (minimum is 5)
	 * @obj                     Reference to the object on which the function should be executed
	 */
	inline void reset(app_timer_id_t& timer_handle, uint32_t ticks, void* obj) {
		BLE_CALL(app_timer_stop, (timer_handle));
		BLE_CALL(app_timer_start, (timer_handle, ticks, obj));
	}

};

