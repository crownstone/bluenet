/**
 * Author: Anne van Rossum
 * Copyrights: Distributed Organism B.V. (DoBots, http://dobots.nl)
 * Date: 21 Apr., 2015
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <common/cs_Timer.h>

#include <common/cs_Config.h>

Timer::Timer() {
	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
}

void Timer::create(app_timer_id_t & timer_handle, app_timer_timeout_handler_t func) {
	app_timer_create(&timer_handle, APP_TIMER_MODE_SINGLE_SHOT, func);
}

void Timer::start(app_timer_id_t & timer_handle, uint32_t ticks) {
	app_timer_start(timer_handle, ticks, NULL);
}

void Timer::stop(app_timer_id_t & timer_handle) {
	app_timer_stop(timer_handle);
}

