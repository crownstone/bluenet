/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 23, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "common/cs_Types.h"
#include "drivers/cs_Serial.h"
#include "drivers/cs_RTC.h"
#include "events/cs_EventDispatcher.h"
#include "processing/cs_Scheduler.h"
#include "processing/cs_Switch.h"
#include "storage/cs_State.h"

//#define SCHEDULER_PRINT_DEBUG

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

	// Subscribe for events.
	EventDispatcher::getInstance().addListener(this);

	// Init and start the timer.
	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t) Scheduler::staticTick);
	scheduleNextTick();
}


// TODO: move time / set time to separate class
void Scheduler::setTime(uint32_t time) {
	LOGi("Set time to %i", time);
	if (time == 0) {
		return;
	}

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
	event_t event(CS_TYPE::EVT_TIME_SET);
	EventDispatcher::getInstance().dispatch(event);
}

cs_ret_code_t Scheduler::setScheduleEntry(uint8_t id, schedule_entry_t* entry) {
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

cs_ret_code_t Scheduler::clearScheduleEntry(uint8_t id) {
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
	// RTC can overflow every 512s
	uint32_t tickDiff = RTC::difference(RTC::getCount(), _rtcTimeStamp);

	// If more than 1s elapsed since last set rtc timestamp:
	// add 1s to posix time and subtract 1s from tickDiff, by increasing the rtc timestamp 1s
	if (_posixTimeStamp && tickDiff > RTC::msToTicks(1000)) {
		_posixTimeStamp++;
		_rtcTimeStamp += RTC::msToTicks(1000);

		State::getInstance().set(CS_TYPE::STATE_TIME, &_posixTimeStamp, sizeof(_posixTimeStamp));
	}

	schedule_entry_t* entry = _scheduleList->isActionTime(_posixTimeStamp);
	if (entry != NULL) {
		switch (ScheduleEntry::getActionType(entry)) {
			case SCHEDULE_ACTION_TYPE_PWM: {
				// TODO: use an event instead
				uint8_t switchState = entry->pwm.pwm;
				// Switch::getInstance().setSwitch(switchState);
				break;
			}
			case SCHEDULE_ACTION_TYPE_FADE: {
				//TODO: implement this, make sure that if something else changes pwm during fade, that the fading is halted.
				//TODO: implement the fade function in the Switch class
				//TODO: if (entry->fade.fadeDuration == 0), then just use SCHEDULE_ACTION_TYPE_PWM
				uint8_t switchState = entry->fade.pwmEnd;
				// Switch::getInstance().setSwitch(switchState);
				break;
			}
			case SCHEDULE_ACTION_TYPE_TOGGLE: {
				// Switch::getInstance().toggle();
				break;
			}
		}
		writeScheduleList(false);
	}

	scheduleNextTick();
}

void Scheduler::writeScheduleList(bool store) {
	LOGe("store flash=%u", store);
	// TODO: schedule list is in ram 2x, should be possible to not have a copy.
//	buffer_ptr_t buffer;
//	uint16_t size;
//	_scheduleList->getBuffer(buffer, size);
	cs_state_data_t stateData(CS_TYPE::STATE_SCHEDULE, NULL, 0);
	_scheduleList->getBuffer(stateData.value, stateData.size);
	State::getInstance().set(stateData);

//	if (store) {
	// TODO: only write to flash when store is true
//	}
}

void Scheduler::readScheduleList() {
	buffer_ptr_t buffer;
	uint16_t length;
	_scheduleList->getBuffer(buffer, length);
	length = _scheduleList->getMaxLength();
	State::getInstance().get(CS_TYPE::STATE_SCHEDULE, buffer, length);
	bool adjusted = _scheduleList->checkAllEntries();
	if (adjusted) {
		writeScheduleList(true);
	}

#ifdef SCHEDULER_PRINT_DEBUG
	LOGi("restored schedule list (%d):", _scheduleList->getSize());
	print();
#endif
	publishScheduleEntries();
}

void Scheduler::publishScheduleEntries() {
	buffer_ptr_t buffer;
	uint16_t size;
	_scheduleList->getBuffer(buffer, size);
	event_t event(CS_TYPE::EVT_SCHEDULE_ENTRIES_UPDATED, buffer, size);
	EventDispatcher::getInstance().dispatch(event);
}

void Scheduler::handleEvent(event_t & event) {
	switch(event.type) {
		case CS_TYPE::STATE_TIME: {
			// Time was set via State.set().
			// This may have been set by us! So only use it when no time is set yet?
			if (_posixTimeStamp == 0 && event.size == sizeof(TYPIFY(STATE_TIME))) {
				setTime(*((TYPIFY(STATE_TIME)*)event.data));
			}
			break;
		}
		case CS_TYPE::EVT_MESH_TIME: {
			// Only set the time if there is currently no time set, as these timestamps may be old
			if (_posixTimeStamp == 0 && event.size == sizeof(TYPIFY(EVT_MESH_TIME))) {
				setTime(*((TYPIFY(EVT_MESH_TIME)*)event.data));
			}
			break;
		}
		case CS_TYPE::CMD_SET_TIME: {
			if (event.size == sizeof(TYPIFY(CMD_SET_TIME))) {
				setTime(*((TYPIFY(CMD_SET_TIME)*)event.data));
			}
			break;
		}
		default: {}
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

