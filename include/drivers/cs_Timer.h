/**
 * Author: Anne van Rossum
 * Copyrights: Distributed Organism B.V. (DoBots, http://dobots.nl)
 * Date: 21 Apr., 2015
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <util/cs_BleError.h>

#include <cfg/cs_Config.h>

class Timer {

private:
//	Timer();
	Timer() {
		APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
		APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, true);
	}

	Timer(Timer const&);
	void operator=(Timer const &);

public:
	static Timer& getInstance() {
		static Timer instance;
		return instance;
	}

	/* Create single shot timer. function will only be called once and after that timer will be
	 * stopped
	 * @timer_handle            An id or handle to reference the timer (actually, just a Uint32_t)
	 * @func                    The function to be called
	 *
	 * Create a timer for a specific purpose.
	 */
//	void createSingleShot(app_timer_id_t & timer_handle, app_timer_timeout_handler_t func);
	inline void createSingleShot(app_timer_id_t & timer_handle, app_timer_timeout_handler_t func) {
		BLE_CALL(app_timer_create, (&timer_handle, APP_TIMER_MODE_SINGLE_SHOT, func));
	}

	/* Create repeated timer. Timer will continue to trigger and function will be called until the
	 * timer is stopped
	 * @timer_handle            An id or handle to reference the timer (actually, just a Uint32_t)
	 * @func                    The function to be called
	 *
	 * Create a timer for a specific purpose.
	 */
//	void createRepeated(app_timer_id_t & timer_handle, app_timer_timeout_handler_t func);
	inline void createRepeated(app_timer_id_t & timer_handle, app_timer_timeout_handler_t func) {
		BLE_CALL(app_timer_create, (&timer_handle, APP_TIMER_MODE_REPEATED, func));
	}

	/* Start a previously created timer
	 * @timer_handle            Reference to previously created timer
	 * @ticks                   Number of ticks till timeout (minimum is 5)
	 * @obj                     Reference to the object on which the function should be executed
	 */
//	void start(app_timer_id_t & timer_handle, uint32_t ticks, void* obj);
	inline void start(app_timer_id_t & timer_handle, uint32_t ticks, void* obj) {
		BLE_CALL(app_timer_start, (timer_handle, ticks, obj));
	}

	/* Stop a timer
	 * @timer_handle            Reference to previously created timer
	 */
//	void stop(app_timer_id_t & timer_handle);
	inline void stop(app_timer_id_t & timer_handle) {
		BLE_CALL(app_timer_stop, (timer_handle));
	}

//	void dummyFunction(void * p_context);
};

