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
//	EventListener(EVT_ALL),
	_appTimerId(NULL),
	_rtcTimeStamp(0),
	_posixTimeStamp(0),
	_scheduleList(NULL)
{
	_appTimerData = { {0} };
	_appTimerId = &_appTimerData;

	_scheduleList = new ScheduleList();
	_scheduleList->assign(_schedulerBuffer, sizeof(_schedulerBuffer));

	printDebug();
	readScheduleList();

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

	// Update time
	int64_t diff = time - _posixTimeStamp;
	_posixTimeStamp = time;
	_rtcTimeStamp = RTC::getCount();

	// If there is a time jump: sync the entries
	printDebug();
	bool adjusted = _scheduleList->sync(time, diff);
	if (adjusted) {
		writeScheduleList(true);
	}
	printDebug();
	EventDispatcher::getInstance().dispatch(EVT_TIME_SET);
}

ERR_CODE Scheduler::setScheduleEntry(uint8_t id, schedule_entry_t* entry) {
	LOGd("set %u", id);
//	if (entry->nextTimestamp == 0) {
//		return clearScheduleEntry(id);
//	}
	if (entry->nextTimestamp <= _posixTimeStamp) {
		return ERR_WRONG_PARAMETER;
	}
	if (!_scheduleList->set(id, entry)) {
		return ERR_WRONG_PARAMETER;
	}
	writeScheduleList(true);
	printDebug();
	publishScheduleEntries();
	return ERR_SUCCESS;
}

ERR_CODE Scheduler::clearScheduleEntry(uint8_t id) {
	LOGd("clear %u", id);
	if (!_scheduleList->clear(id)) {
		return ERR_WRONG_PARAMETER;
	}
	writeScheduleList(true);
	printDebug();
	publishScheduleEntries();
	return ERR_SUCCESS;
}

void Scheduler::tick() {
	//! RTC can overflow every 512s
	uint32_t tickDiff = RTC::difference(RTC::getCount(), _rtcTimeStamp);

	//! If more than 1s elapsed since last set rtc timestamp:
	//! add 1s to posix time and subtract 1s from tickDiff, by increasing the rtc timestamp 1s
	if (_posixTimeStamp && tickDiff > RTC::msToTicks(1000)) {
		_posixTimeStamp++;
		_rtcTimeStamp += RTC::msToTicks(1000);

		State::getInstance().set(STATE_TIME, _posixTimeStamp);
	}

	schedule_entry_t* entry = _scheduleList->isActionTime(_posixTimeStamp);
	if (entry != NULL) {
		switch (ScheduleEntry::getActionType(entry)) {
			case SCHEDULE_ACTION_TYPE_PWM: {
				//! TODO: use an event instead
				uint8_t switchState = entry->pwm.pwm;
				Switch::getInstance().setSwitch(switchState);
				State::getInstance().set(STATE_IGNORE_BITMASK, entry->overrideMask);
				break;
			}
			case SCHEDULE_ACTION_TYPE_FADE: {
				//TODO: implement this, make sure that if something else changes pwm during fade, that the fading is halted.
				//TODO: implement the fade function in the Switch class
				//TODO: if (entry->fade.fadeDuration == 0), then just use SCHEDULE_ACTION_TYPE_PWM
				uint8_t switchState = entry->fade.pwmEnd;
				Switch::getInstance().setSwitch(switchState);
				State::getInstance().set(STATE_IGNORE_BITMASK, entry->overrideMask);
				break;
			}
			case SCHEDULE_ACTION_TYPE_TOGGLE: {
				Switch::getInstance().toggle();
				State::getInstance().set(STATE_IGNORE_BITMASK, entry->overrideMask);
				break;
			}
		}
		writeScheduleList(false);
	}

	scheduleNextTick();
}

void Scheduler::writeScheduleList(bool store) {
	LOGd("store");
	buffer_ptr_t buffer;
	uint16_t length;
	_scheduleList->getBuffer(buffer, length);
	State::getInstance().set(STATE_SCHEDULE, buffer, length);
	if (store) {
		State::getInstance().savePersistentStorageItem(STATE_SCHEDULE);
	}
}

void Scheduler::readScheduleList() {
	buffer_ptr_t buffer;
	uint16_t length;
	_scheduleList->getBuffer(buffer, length);
	length = _scheduleList->getMaxLength();

	State::getInstance().get(STATE_SCHEDULE, buffer, length);
	bool adjusted = _scheduleList->checkAllEntries();
	if (adjusted) {
		writeScheduleList(true);
	}

	LOGi("restored schedule list (%d):", _scheduleList->getSize());
	print();
	publishScheduleEntries();
}

void Scheduler::publishScheduleEntries() {
	buffer_ptr_t buffer;
	uint16_t size;
	_scheduleList->getBuffer(buffer, size);
	EventDispatcher::getInstance().dispatch(EVT_SCHEDULE_ENTRIES_UPDATED, buffer, size);
}

void Scheduler::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case STATE_TIME: {
		// Time was set via State.set().
		// This may have been set by us! So only use it when no time is set yet?
		if (_posixTimeStamp == 0 && length == sizeof(uint32_t)) {
			setTime(*((uint32_t*)p_data));
		}
		break;
	}
	case EVT_MESH_TIME: {
		// Only set the time if there is currently no time set, as these timestamps may be old
		if (_posixTimeStamp == 0 && length == sizeof(uint32_t)) {
			setTime(*((uint32_t*)p_data));
		}
		break;
	}
	}
}

void Scheduler::print() {
	_scheduleList->print();
}

void Scheduler::printDebug() {
#ifdef SCHEDULER_PRINT_DEBUG
	print();
#endif
}

