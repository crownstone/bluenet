/**
 * Author: Anne van Rossum
 * Copyrights: Distributed Organism B.V. (DoBots, http://dobots.nl)
 * Date: 21 Apr., 2015
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <drivers/cs_Timer.h>
//
//#include <drivers/cs_RTC.h>
//
//#include <app_scheduler.h>

//app_timer_id_t dummyId;
//uint8_t count = 0;

//void Timer::dummyFunction(void * p_context) {
//	LOGi("dummy tick at: %d", RTC::now());
//	if (count++ == 50) {
//		Timer::getInstance().stop(dummyId);
//	}
//}

//Timer::Timer() {
//	APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
//	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, true);
////	// Store member function and the instance using std::bind.
////	Callback<void(void*)>::func = std::bind(&Timer::dummyFunction, *this, std::placeholders::_1);
////
////	// Convert callback-function to c-pointer.
////	void (*c_func)(void*) = static_cast<decltype(c_func)>(Callback<void(void*)>::callback);
////
////
////	app_timer_create(&dummyId, APP_TIMER_MODE_REPEATED, c_func);
////
////
//////	create(dummyId, dummyFunction);
////	uint32_t delay_ticks = APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER);
////	LOGi("delay_ticks: %d", delay_ticks);
////	start(dummyId, delay_ticks);
//}

//void Timer::createSingleShot(app_timer_id_t& timer_handle, app_timer_timeout_handler_t func) {
//	BLE_CALL(app_timer_create, (&timer_handle, APP_TIMER_MODE_SINGLE_SHOT, func));
//}

//void Timer::createRepeated(app_timer_id_t& timer_handle, app_timer_timeout_handler_t func) {
//	BLE_CALL(app_timer_create, (&timer_handle, APP_TIMER_MODE_REPEATED, func));
//}

//void Timer::start(app_timer_id_t& timer_handle, uint32_t ticks, void* obj) {
//	BLE_CALL(app_timer_start, (timer_handle, ticks, obj));
//}

//void Timer::stop(app_timer_id_t& timer_handle) {
//	BLE_CALL(app_timer_stop, (timer_handle));
//}

