/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 23, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "common/cs_Types.h"
#include "drivers/cs_Serial.h"

#include "events/cs_EventDispatcher.h"
#include "processing/cs_Scheduler.h"
#include "storage/cs_State.h"
#include "time/cs_SystemTime.h"

//#define SCHEDULER_PRINT_DEBUG

Scheduler::Scheduler() :
	_scheduleList(NULL)
{


	_scheduleList = new ScheduleList();
	_scheduleList->assign(_schedulerBuffer, sizeof(_schedulerBuffer));

	printDebug();
	readScheduleList();

	// Subscribe for events.
	EventDispatcher::getInstance().addListener(this);
}

void Scheduler::handleEvent(event_t & event){
	switch(event.type){
		case CS_TYPE::STATE_TIME:{
			processScheduledAction();
		}
		case CS_TYPE::EVT_TIME_SET:{
			handleSetTimeEvent(
				*reinterpret_cast<TYPIFY(EVT_TIME_SET)*>(event.data));
		}
		default:{
			break;
		}
	}
}

// ================= ScheduleList administration ================== 

void Scheduler::handleSetTimeEvent(uint32_t prevtime) {
	uint32_t time = SystemTime::posix();
	uint64_t diff = time - prevtime;

	// If there is a time jump: sync the entries
	printDebug();
	bool adjusted = _scheduleList->sync(time, diff);
	if (adjusted) {
		writeScheduleList(true);
	}
	printDebug();
}

void Scheduler::processScheduledAction(){
	schedule_entry_t* entry = _scheduleList->isActionTime(SystemTime::posix());

	if (entry != NULL) {
		switch (ScheduleEntry::getActionType(entry)) {
			case SCHEDULE_ACTION_TYPE_PWM: {
				// TODO: use an event instead
				// uint8_t switchState = entry->pwm.pwm;
				// Switch::getInstance().setSwitch(switchState);
				break;
			}
			case SCHEDULE_ACTION_TYPE_FADE: {
				//TODO: implement this, make sure that if something else changes pwm during fade, that the fading is halted.
				//TODO: implement the fade function in the Switch class
				//TODO: if (entry->fade.fadeDuration == 0), then just use SCHEDULE_ACTION_TYPE_PWM
				// uint8_t switchState = entry->fade.pwmEnd;
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
}

cs_ret_code_t Scheduler::setScheduleEntry(uint8_t id, schedule_entry_t* entry) {
	LOGd("set %u", id);
//	if (entry->nextTimestamp == 0) {
//		return clearScheduleEntry(id);
//	}
	if (entry->nextTimestamp <= SystemTime::posix()) {
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

// ============== DEBUG =============

void Scheduler::print() {
	_scheduleList->print();
}

void Scheduler::printDebug() {
#ifdef SCHEDULER_PRINT_DEBUG
	print();
#endif
}

