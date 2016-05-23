/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "services/cs_ScheduleService.h"

#include <cfg/cs_UuidConfig.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_Scheduler.h>
#include <structs/buffer/cs_MasterBuffer.h>

using namespace BLEpp;

ScheduleService::ScheduleService() :
		_currentTimeCharacteristic(NULL),
		_writeScheduleEntryCharacteristic(NULL),
		_listScheduleEntriesCharacteristic(NULL)
{
	EventDispatcher::getInstance().addListener(this);

	setUUID(UUID(SCHEDULE_UUID));

	setName(BLE_SERVICE_SCHEDULE);

	init();
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

	addCharacteristicsDone();
}

void ScheduleService::addCurrentTimeCharacteristic() {
	_currentTimeCharacteristic = new Characteristic<uint32_t>();
	addCharacteristic(_currentTimeCharacteristic);
	_currentTimeCharacteristic->setUUID(UUID(getUUID(), CURRENT_TIME_UUID));
	_currentTimeCharacteristic->setName(BLE_CHAR_CURRENT_TIME);
	_currentTimeCharacteristic->setDefaultValue(0);
	_currentTimeCharacteristic->setWritable(true);
	_currentTimeCharacteristic->onWrite([&](const uint32_t& value) -> void {
		Scheduler::getInstance().setTime(value);
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

	_writeScheduleEntryCharacteristic->onWrite([&](const buffer_ptr_t& value) -> void {
		CommandHandler::getInstance().handleCommand(CMD_SCHEDULE_ENTRY, _writeScheduleEntryCharacteristic->getValue(),
				_writeScheduleEntryCharacteristic->getValueLength());
	});
}

void ScheduleService::addListScheduleEntriesCharacteristic() {
	_listScheduleEntriesCharacteristic = new Characteristic<uint8_t*>();
	addCharacteristic(_listScheduleEntriesCharacteristic);
	_listScheduleEntriesCharacteristic->setUUID(UUID(getUUID(), LIST_SCHEDULE_ENTRIES_UUID));
	_listScheduleEntriesCharacteristic->setName(BLE_CHAR_LIST_SCHEDULE);
	_listScheduleEntriesCharacteristic->setWritable(false);
	_listScheduleEntriesCharacteristic->setNotifies(false);

	ScheduleList* list = Scheduler::getInstance().getScheduleList();
	buffer_ptr_t buffer = NULL;
	uint16_t maxLength = 0;
	list->getBuffer(buffer, maxLength);
	maxLength = list->getMaxLength();

	_listScheduleEntriesCharacteristic->setValue(buffer);
	_listScheduleEntriesCharacteristic->setMaxLength(maxLength);
	_listScheduleEntriesCharacteristic->setDataLength(0);
}


void ScheduleService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case EVT_TIME_UPDATED: {
		if (_currentTimeCharacteristic) {
			*_currentTimeCharacteristic = *(uint32_t*)p_data;
		}
		break;
	}
	case EVT_SCHEDULE_ENTRIES_UPDATED: {
		if (_listScheduleEntriesCharacteristic) {
			_listScheduleEntriesCharacteristic->setValue((buffer_ptr_t)p_data);
			_listScheduleEntriesCharacteristic->setDataLength(length);
			_listScheduleEntriesCharacteristic->notify();
		}
	}
	}
}
