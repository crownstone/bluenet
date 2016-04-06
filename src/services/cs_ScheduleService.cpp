/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "services/cs_ScheduleService.h"
//
#include "cfg/cs_Boards.h"
#include "cfg/cs_UuidConfig.h"
#include "cfg/cs_Settings.h"
#include "drivers/cs_RTC.h"
#include "drivers/cs_PWM.h"
#include "structs/buffer/cs_MasterBuffer.h"
//#include <time.h>

using namespace BLEpp;

ScheduleService::ScheduleService() :
		_currentTimeCharacteristic(NULL)
{

	setUUID(UUID(SCHEDULE_UUID));

	setName(BLE_SERVICE_SCHEDULE);

	init();

	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)ScheduleService::staticTick);
}

void ScheduleService::init() {
	LOGi("Create schedule service");

	LOGi("add current time characteristic");
	addCurrentTimeCharacteristic();

#if CHAR_SCHEDULE==1
	LOGi("add schedule characteristic");
	addWriteScheduleEntryCharacteristic();
	addListScheduleEntriesCharacteristic();
#endif
}

uint32_t ScheduleService::getTime() {
	return *_currentTimeCharacteristic;
}

void ScheduleService::setTime(uint32_t time) {
	LOGi("Set time to %i", time);
	*_currentTimeCharacteristic = time;
	_rtcTimeStamp = RTC::getCount();

#if CHAR_SCHEDULE==1
	_scheduleList->sync(time);
#endif
}

void ScheduleService::tick() {
	//! RTC can overflow every 16s
	uint32_t tickDiff = RTC::difference(RTC::getCount(), _rtcTimeStamp);

	//! If more than 1s elapsed since last rtc timestamp:
	//! add 1s to posix time and substract 1s from tickDiff, by increasing the rtc timestamp 1s
	if (_currentTimeCharacteristic && *_currentTimeCharacteristic && tickDiff > RTC::msToTicks(1000)) {
		(*_currentTimeCharacteristic)++;
		_rtcTimeStamp += RTC::msToTicks(1000);
//		LOGd("posix time = %i", (uint32_t)(*_currentTimeCharacteristic));
//		long int timestamp = *_currentTimeCharacteristic;
//		tm* datetime = gmtime(&timestamp);
//		LOGd("day of week = %i", datetime->tm_wday);
	}

#if CHAR_SCHEDULE==1
	schedule_entry_t* entry = _scheduleList->checkSchedule(*_currentTimeCharacteristic);
	if (entry != NULL) {
		switch (ScheduleEntry::getActionType(entry)) {
		case SCHEDULE_ACTION_TYPE_PWM:
			PWM::getInstance().setValue(0, entry->pwm.pwm);
			break;
		case SCHEDULE_ACTION_TYPE_FADE:
			//TODO: implement this, make sure that if something else changes pwm during fade, that the fading is halted.
			break;
		case SCHEDULE_ACTION_TYPE_TOGGLE:
			//TODO: implement this.
			break;
		}
	}
#endif

	scheduleNextTick();
}

void ScheduleService::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(SCHEDULE_SERVICE_UPDATE_FREQUENCY), this);
}

void ScheduleService::addCurrentTimeCharacteristic() {
	_currentTimeCharacteristic = new Characteristic<uint32_t>();
	addCharacteristic(_currentTimeCharacteristic);
	_currentTimeCharacteristic->setUUID(UUID(getUUID(), CURRENT_TIME_UUID));
	_currentTimeCharacteristic->setName(BLE_CHAR_CURRENT_TIME);
	_currentTimeCharacteristic->setDefaultValue(0);
	_currentTimeCharacteristic->setWritable(true);
	_currentTimeCharacteristic->onWrite([&](const uint32_t& value) -> void {
		setTime(value);
	});
}

void ScheduleService::addWriteScheduleEntryCharacteristic() {
	MasterBuffer& mb = MasterBuffer::getInstance();
	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;
	mb.getBuffer(buffer, maxLength);

	_writeScheduleEntryCharacteristic = new Characteristic<buffer_ptr_t>();
	addCharacteristic(_writeScheduleEntryCharacteristic);

	_writeScheduleEntryCharacteristic->setUUID(UUID(getUUID(), WRITE_SCHEDULE_ENTRY_UUID));
	_writeScheduleEntryCharacteristic->setName(BLE_CHAR_WRITE_SCHEDULE);
	_writeScheduleEntryCharacteristic->setWritable(true);
	_writeScheduleEntryCharacteristic->setNotifies(false);

	_writeScheduleEntryCharacteristic->setValue(buffer);
	_writeScheduleEntryCharacteristic->setMaxLength(maxLength);
	_writeScheduleEntryCharacteristic->setDataLength(0);
	LOGd("serialized size: %u", SCHEDULE_ENTRY_SERIALIZED_SIZE);

	_writeScheduleEntryCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {
		ScheduleEntry entry;

		if (entry.assign(_writeScheduleEntryCharacteristic->getValue(), _writeScheduleEntryCharacteristic->getValueLength())) {
			return;
		}
		schedule_entry_t* entryStruct = entry.getStruct();
		if (!entryStruct->nextTimestamp) {
			_scheduleList->rem(entryStruct);
		}
		else {

			//! Check if entry is correct
			if (entryStruct->nextTimestamp < *_currentTimeCharacteristic) {
				return;
			}
			switch (ScheduleEntry::getTimeType(entryStruct)) {
			case SCHEDULE_TIME_TYPE_REPEAT:
				if (entryStruct->repeat == 0) {
					return;
				}
				break;
			case SCHEDULE_TIME_TYPE_DAILY:
				if (entryStruct->daily.nextDayOfWeek > 6) {
					return;
				}
				break;
			case SCHEDULE_TIME_TYPE_ONCE:
				break;
			}

			_scheduleList->add(entryStruct);
		}
		_scheduleList->print();
		_listScheduleEntriesCharacteristic->setDataLength(_scheduleList->getDataLength());
		_listScheduleEntriesCharacteristic->notify();
	});
}

void ScheduleService::addListScheduleEntriesCharacteristic() {
	_scheduleList = new ScheduleList();

	uint16_t size = sizeof(schedule_list_t);
	buffer_ptr_t buffer = (buffer_ptr_t)calloc(size, sizeof(uint8_t));
	_scheduleList->assign(buffer, size);

	_listScheduleEntriesCharacteristic = new Characteristic<uint8_t*>();
	addCharacteristic(_listScheduleEntriesCharacteristic);
	_listScheduleEntriesCharacteristic->setUUID(UUID(getUUID(), LIST_SCHEDULE_ENTRIES_UUID));
	_listScheduleEntriesCharacteristic->setName(BLE_CHAR_LIST_SCHEDULE);
	_listScheduleEntriesCharacteristic->setWritable(false);
	_listScheduleEntriesCharacteristic->setNotifies(false);

	_listScheduleEntriesCharacteristic->setValue(buffer);
	_listScheduleEntriesCharacteristic->setMaxLength(_scheduleList->getMaxLength());
	_listScheduleEntriesCharacteristic->setDataLength(0);
}

