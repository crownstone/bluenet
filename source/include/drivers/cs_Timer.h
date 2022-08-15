/*
 * Author: Crownstone Team
 * Copyrights: Distributed Organism B.V. (DoBots, http://dobots.nl)
 * Date: 21 Apr., 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <cfg/cs_Config.h>

extern "C" {
#include <components/libraries/scheduler/app_scheduler.h>
#include <components/libraries/timer/app_timer.h>
}

#define HZ_TO_TICKS(hz) APP_TIMER_TICKS(1000 / hz)
#define MS_TO_TICKS(ms) APP_TIMER_TICKS(ms)

/** Timer on top of the timer peripheral.
 */
class Timer {
public:
	static Timer& getInstance();

	void init();

	/** Create single shot timer. function will only be called once and after that timer will be
	 * stopped
	 * @timer_handle            An id or handle to reference the timer, set by this function (actually, just a Uint32_t)
	 * @func                    The function to be called
	 *
	 * Create a timer for a specific purpose.
	 */
	void createSingleShot(app_timer_id_t& timer_handle, app_timer_timeout_handler_t func);

	/** Start a previously created timer
	 * @timer_handle            Reference to previously created timer
	 * @ticks                   Number of ticks till timeout (minimum is 5)
	 * @obj                     Reference to the object on which the function should be executed
	 */
	void start(app_timer_id_t& timer_handle, uint32_t ticks, void* obj);

	/** Stop a timer
	 * @timer_handle            Reference to previously created timer
	 */
	void stop(app_timer_id_t& timer_handle);

	/** Resets a timer (if already active) to the new ticks
	 * @timer_handle            Reference to previously created timer
	 * @ticks                   Number of ticks till timeout (minimum is 5)
	 * @obj                     Reference to the object on which the function should be executed
	 */
	void reset(app_timer_id_t& timer_handle, uint32_t ticks, void* obj);

private:
	Timer(){};

	Timer(Timer const&);
	void operator=(Timer const&);
};
