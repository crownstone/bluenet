/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 23, 2016
 * License: LGPLv3+
 */

#include <processing/cs_Scheduler.h>

#include <drivers/cs_Serial.h>
#include <drivers/cs_RTC.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <processing/cs_Switch.h>

#define SCHEDULER_PRINT_DEBUG

Scheduler::Scheduler() :
	EventListener(EVT_ALL),
	_appTimerId(NULL),
	_rtcTimeStamp(0),
	_posixTimeStamp(0),
	_scheduleList(NULL)
{
	_appTimerData = { {0} };
	_appTimerId = &_appTimerData;

#if SCHEDULER_ENABLED==1
	_scheduleList = new ScheduleList();
	_scheduleList->assign(_schedulerBuffer, sizeof(_schedulerBuffer));
#ifdef SCHEDULER_PRINT_DEBUG
	_scheduleList->print();
#endif

	readScheduleList();
#endif

	//! Subscribe for events.
	EventDispatcher::getInstance().addListener(this);

	//! Init and start the timer.
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) Scheduler::staticTick);
	scheduleNextTick();
}


// todo: move time / set time to separate class
/** Returns the current time as posix time
 * returns 0 when no time was set yet
 */
void Scheduler::setTime(uint32_t time) {
	LOGi("Set time to %i", time);
	_posixTimeStamp = time;
	_rtcTimeStamp = RTC::getCount();
#if SCHEDULER_ENABLED==1
	_scheduleList->sync(time);
#ifdef SCHEDULER_PRINT_DEBUG
	_scheduleList->print();
#endif
#endif
}

void Scheduler::addScheduleEntry(schedule_entry_t* entry) {
#if SCHEDULER_ENABLED==1
	if (_scheduleList->add(entry)) {
		writeScheduleList();
#ifdef SCHEDULER_PRINT_DEBUG
		_scheduleList->print();
#endif
		publishScheduleEntries();
	}
#endif
}

void Scheduler::removeScheduleEntry(schedule_entry_t* entry) {
#if SCHEDULER_ENABLED==1
	if (_scheduleList->rem(entry)) {
		writeScheduleList();
#ifdef SCHEDULER_PRINT_DEBUG
		_scheduleList->print();
#endif
		publishScheduleEntries();
	}
#endif
}

void Scheduler::tick() {

	//! RTC can overflow every 16s
	uint32_t tickDiff = RTC::difference(RTC::getCount(), _rtcTimeStamp);

	//! If more than 1s elapsed since last set rtc timestamp:
	//! add 1s to posix time and subtract 1s from tickDiff, by increasing the rtc timestamp 1s
	if (_posixTimeStamp && tickDiff > RTC::msToTicks(1000)) {
		_posixTimeStamp++;
		_rtcTimeStamp += RTC::msToTicks(1000);

		State::getInstance().set(STATE_TIME, _posixTimeStamp);

//		EventDispatcher::getInstance().dispatch(EVT_TIME_UPDATED, &_posixTimeStamp, sizeof(_posixTimeStamp));
		//		LOGd("posix time = %i", (uint32_t)(*_currentTimeCharacteristic));
		//		long int timestamp = *_currentTimeCharacteristic;
		//		tm* datetime = gmtime(&timestamp);
		//		LOGd("day of week = %i", datetime->tm_wday);
	}

#if SCHEDULER_ENABLED==1
	schedule_entry_t* entry = _scheduleList->checkSchedule(_posixTimeStamp);
	if (entry != NULL) {
		switch (ScheduleEntry::getActionType(entry)) {
			case SCHEDULE_ACTION_TYPE_PWM: {
				//! TODO: use pwm here later on, for now: just switch on and off relay.
				//! TODO: use an event instead
				uint8_t switchState = entry->pwm.pwm == 0 ? 0 : 100;
				Switch::getInstance().setSwitch(switchState);
				break;
			}
			case SCHEDULE_ACTION_TYPE_FADE:
				//TODO: implement this, make sure that if something else changes pwm during fade, that the fading is halted.
				//TODO: implement the fade function in the Switch class
				break;
		}
	}
#endif

	scheduleNextTick();
}

void Scheduler::writeScheduleList() {
#if SCHEDULER_ENABLED==1
	buffer_ptr_t buffer;
	uint16_t length;
	_scheduleList->getBuffer(buffer, length);
	State::getInstance().set(STATE_SCHEDULE, buffer, length);
#endif
}

void Scheduler::readScheduleList() {
#if SCHEDULER_ENABLED==1
	buffer_ptr_t buffer;
	uint16_t length;
	_scheduleList->getBuffer(buffer, length);
	length = _scheduleList->getMaxLength();

	State::getInstance().get(STATE_SCHEDULE, buffer, length);

	if (!_scheduleList->isEmpty()) {
#ifdef SCHEDULER_PRINT_DEBUG
		LOGi("restored schedule list (%d):", _scheduleList->getSize());
		_scheduleList->print();
#endif
		publishScheduleEntries();
	}
#endif
}

void Scheduler::publishScheduleEntries() {
#if SCHEDULER_ENABLED==1
	buffer_ptr_t buffer;
	uint16_t size;
	_scheduleList->getBuffer(buffer, size);
	EventDispatcher::getInstance().dispatch(EVT_SCHEDULE_ENTRIES_UPDATED, buffer, size);
#endif
}

void Scheduler::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case STATE_TIME: {
		//! Time was set via State.set().
		//! This may have been us! So only use it when no time is set yet?
		if (_posixTimeStamp == 0 && length == sizeof(uint32_t)) {
			setTime(*((uint32_t*)p_data));
		}
		break;
	}
	case EVT_MESH_TIME: {
		//! Only set the time if there is currently no time set, as these timestamps may be old
		if (_posixTimeStamp == 0 && length == sizeof(uint32_t)) {
			setTime(*((uint32_t*)p_data));
		}
		break;
	}
	}
}

