/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 23, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <drivers/cs_Timer.h>
#include <structs/cs_ScheduleEntriesAccessor.h>
#include <events/cs_EventListener.h>

#include <stdint.h>


class Scheduler : public EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static Scheduler& getInstance() {
		static Scheduler instance;
		return instance;
	}

	/**
	 * Receives STATE_TIME events and performs the scheduled actions based on that.
	 * Receives EVT_TIME_SET events to adjust the schedule list.
	 */
	void handleEvent(event_t& event);

	cs_ret_code_t setScheduleEntry(uint8_t id, schedule_entry_t* entry);

	cs_ret_code_t clearScheduleEntry(uint8_t id);

	inline ScheduleList* getScheduleList() {
		return _scheduleList;
	}

protected:
	
	void writeScheduleList(bool store);

	void readScheduleList();

	void publishScheduleEntries();
	void processScheduledAction();

	void handleSetTimeEvent(uint32_t prevtime);

	void print();

	void printDebug();

private:
	Scheduler();

	uint8_t _schedulerBuffer[sizeof(schedule_list_t)];

	ScheduleList* _scheduleList;

};

