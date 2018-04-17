/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "services/cs_ScheduleService.h"

#include <cfg/cs_UuidConfig.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_Scheduler.h>
#include <structs/buffer/cs_MasterBuffer.h>
#include <protocol/cs_StateTypes.h>

ScheduleService::ScheduleService() :
		_currentTimeCharacteristic(NULL),
		_writeScheduleEntryCharacteristic(NULL),
		_listScheduleEntriesCharacteristic(NULL)
{
	EventDispatcher::getInstance().addListener(this);

	setUUID(UUID(SCHEDULE_UUID));

	setName(BLE_SERVICE_SCHEDULE);

	createCharacteristics();
}

void ScheduleService::createCharacteristics() {
	LOGi(FMT_SERVICE_INIT, BLE_SERVICE_SCHEDULE);

	LOGi(FMT_CHAR_ADD, STR_CHAR_CURRENT_TIME);
	addCurrentTimeCharacteristic();

#if CHAR_SCHEDULE==1
	LOGi(FMT_CHAR_ADD, STR_CHAR_SCHEDULE);
	addWriteScheduleEntryCharacteristic();
	addListScheduleEntriesCharacteristic();
#else
	LOGi(FMT_CHAR_SKIP, STR_CHAR_SCHEDULE);
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
	_currentTimeCharacteristic->onWrite([&](const uint8_t accessLevel, const uint32_t& value, uint16_t length) -> void {
		CommandHandler::getInstance().handleCommand(CMD_SET_TIME, (buffer_ptr_t)&value, sizeof(value));
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
	_writeScheduleEntryCharacteristic->setMaxGattValueLength(maxLength);
	_writeScheduleEntryCharacteristic->setValueLength(0);

	_writeScheduleEntryCharacteristic->onWrite([&](const uint8_t accessLevel, const buffer_ptr_t& value, uint16_t length) -> void {
		if (length == 1) {
			CommandHandler::getInstance().handleCommand(CMD_SCHEDULE_ENTRY_CLEAR, _writeScheduleEntryCharacteristic->getValue(), length);
		}
		else {
			CommandHandler::getInstance().handleCommand(CMD_SCHEDULE_ENTRY_SET, _writeScheduleEntryCharacteristic->getValue(), length);
		}
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
	_listScheduleEntriesCharacteristic->setMaxGattValueLength(maxLength);
	_listScheduleEntriesCharacteristic->setValueLength(0);
}


void ScheduleService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case STATE_TIME: {
		if (_currentTimeCharacteristic) {
			*_currentTimeCharacteristic = *(uint32_t*)p_data;
		}
		break;
	}
	case EVT_SCHEDULE_ENTRIES_UPDATED: {
		if (_listScheduleEntriesCharacteristic) {
			_listScheduleEntriesCharacteristic->setValue((buffer_ptr_t)p_data);
			_listScheduleEntriesCharacteristic->setValueLength(length);
			_listScheduleEntriesCharacteristic->notify();
		}
	}
	}
}
