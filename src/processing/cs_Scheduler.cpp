/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 23, 2016
 * License: LGPLv3+
 */

#include <processing/cs_Scheduler.h>

#include <storage/cs_StateVars.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_PWM.h>
#include <drivers/cs_RTC.h>
#include <events/cs_EventDispatcher.h>

Scheduler::Scheduler() :
		_rtcTimeStamp(0), _posixTimeStamp(0), _scheduleList(NULL) {

#if SCHEDULER_ENABLED==1
	_scheduleList = new ScheduleList();
	_scheduleList->assign(_schedulerBuffer, sizeof(_schedulerBuffer));
#ifdef PRINT_DEBUG
	_scheduleList->print();
#endif

	readScheduleList();
#endif

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) Scheduler::staticTick);
}

/** Returns the current time as posix time
 * returns 0 when no time was set yet
 */
void Scheduler::setTime(uint32_t time) {
	LOGi("Set time to %i", time);
	Scheduler::getInstance().setTime(time);
	_posixTimeStamp = time;
	_rtcTimeStamp = RTC::getCount();
#if SCHEDULER_ENABLED==1
	_scheduleList->sync(time);
	_scheduleList->print();
#endif
}

void Scheduler::addScheduleEntry(schedule_entry_t* entry) {
#if SCHEDULER_ENABLED==1
	if (_scheduleList->add(entry)) {
		writeScheduleList();
#ifdef PRINT_DEBUG
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
#ifdef PRINT_DEBUG
		_scheduleList->print();
#endif
		publishScheduleEntries();
	}
#endif
}

void Scheduler::tick() {

	//! RTC can overflow every 16s
	uint32_t tickDiff = RTC::difference(RTC::getCount(), _rtcTimeStamp);

	//! If more than 1s elapsed since last rtc timestamp:
	//! add 1s to posix time and substract 1s from tickDiff, by increasing the rtc timestamp 1s
	if (_posixTimeStamp && tickDiff > RTC::msToTicks(1000)) {
		_posixTimeStamp++;
		_rtcTimeStamp += RTC::msToTicks(1000);

		EventDispatcher::getInstance().dispatch(EVT_TIME_UPDATED, &_posixTimeStamp, sizeof(_posixTimeStamp));
		//		LOGd("posix time = %i", (uint32_t)(*_currentTimeCharacteristic));
		//		long int timestamp = *_currentTimeCharacteristic;
		//		tm* datetime = gmtime(&timestamp);
		//		LOGd("day of week = %i", datetime->tm_wday);
	}

#if SCHEDULER_ENABLED==1
	schedule_entry_t* entry = _scheduleList->checkSchedule(_posixTimeStamp);
	if (entry != NULL) {
		switch (ScheduleEntry::getActionType(entry)) {
			case SCHEDULE_ACTION_TYPE_PWM:
			PWM::getInstance().setValue(0, entry->pwm.pwm);
			break;
			case SCHEDULE_ACTION_TYPE_FADE:
			//TODO: implement this, make sure that if something else changes pwm during fade, that the fading is halted.
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
	StateVars::getInstance().setStateVar(SV_SCHEDULE, buffer, length);
#endif
}

void Scheduler::readScheduleList() {
#if SCHEDULER_ENABLED==1
	buffer_ptr_t buffer;
	uint16_t length;
	_scheduleList->getBuffer(buffer, length);
	length = _scheduleList->getMaxLength();

	StateVars::getInstance().getStateVar(SV_SCHEDULE, buffer, length);

	if (!_scheduleList->isEmpty()) {
#ifdef PRINT_DEBUG
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

