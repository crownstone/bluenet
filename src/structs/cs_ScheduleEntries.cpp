/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 4, 2016
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "structs/cs_ScheduleEntries.h"
#include <util/cs_BleError.h>

//#define PRINT_DEBUG

bool ScheduleEntry::isActionTime(schedule_entry_t* entry, uint32_t timestamp) {
	bool result = false;
	if (entry->nextTimestamp == 0) {
		//TODO: remove this entry
		return false;
	}
	if (entry->nextTimestamp < timestamp) {
		result = true;
#ifdef PRINT_DEBUG
		LOGd("found:");
		print(entry);
#endif
		switch (getTimeType(entry)) {
		case SCHEDULE_TIME_TYPE_REPEAT:
			entry->nextTimestamp += entry->repeat.repeatTime * 60;
			break;
		case SCHEDULE_TIME_TYPE_DAILY: {
			// Get day of week, before increasing it
			uint8_t dayOfWeek = getDayOfWeek(entry->nextTimestamp);
			// Daily task, reschedule in 24h
			entry->nextTimestamp += SECONDS_PER_DAY;
			// Check day of week
			if (!isActionDay(entry->daily.dayOfWeekBitmask, dayOfWeek)) {
				result = false;
			}
			break;
		}
		case SCHEDULE_TIME_TYPE_ONCE:
			entry->nextTimestamp = 0; // Mark for deletion
			break;
		default:
			entry->nextTimestamp = 0; // Mark for deletion
			result = false;
			break;
		}
#ifdef PRINT_DEBUG
		LOGd("Update nextTimestamp=%u", entry->nextTimestamp);
#endif
	}
	return result;
}

bool ScheduleEntry::syncTime(schedule_entry_t* entry, uint32_t timestamp) {
	if (entry->nextTimestamp == 0) {
		//TODO: remove this entry
		return false;
	}
	bool adjusted = false;
	//! Make sure nextTimestamp >= currentTime
	if (entry->nextTimestamp < timestamp) {
		switch (ScheduleEntry::getTimeType(entry)) {
		case SCHEDULE_TIME_TYPE_REPEAT: {
			uint32_t secondsPerRepeat = entry->repeat.repeatTime * 60;
			uint32_t repeatDiff = (timestamp - entry->nextTimestamp + secondsPerRepeat - 1) / secondsPerRepeat;
			entry->nextTimestamp += repeatDiff * secondsPerRepeat;
			adjusted = (repeatDiff != 0);
			break;
		}
		case SCHEDULE_TIME_TYPE_DAILY:{
			uint32_t daysDiff = (timestamp - entry->nextTimestamp + SECONDS_PER_DAY - 1) / SECONDS_PER_DAY;
			entry->nextTimestamp += daysDiff * SECONDS_PER_DAY;
			adjusted = (daysDiff != 0);
			break;
		}
		case SCHEDULE_TIME_TYPE_ONCE:
			// TODO: remove this entry immediately
			entry->nextTimestamp = 0; // Mark for deletion
			break;
		default:
			entry->nextTimestamp = 0; // Mark for deletion
			break;
		}
	}
	return adjusted;
}

uint8_t ScheduleEntry::getTimeType(const schedule_entry_t* entry) {
	uint8_t timeType = entry->type & 0x0F;
	return timeType;
}

uint8_t ScheduleEntry::getActionType(const schedule_entry_t* entry) {
	uint8_t actionType = (entry->type & 0xF0) >> 4;
	return actionType;
}

uint8_t ScheduleEntry::getDayOfWeek(uint32_t timestamp) {
	return (timestamp / SECONDS_PER_DAY + 4) % 7;
}

bool ScheduleEntry::isActionDay(uint8_t bitMask, uint8_t dayOfWeek) {
#ifdef PRINT_DEBUG
	LOGd("isActionDay: 0x%x %u", bitMask, dayOfWeek);
#endif
	return (bitMask & (1 << DAILY_REPEAT_BIT_ALL_DAYS)) || (bitMask & (1 << dayOfWeek));
}

void ScheduleEntry::print(const schedule_entry_t* entry) {

	LOGd("id=%03u type=%02X override=%02X nextTimestamp=%u", entry->id, entry->type, entry->overrideMask, entry->nextTimestamp);

	switch (getTimeType(entry)) {
	case SCHEDULE_TIME_TYPE_REPEAT:
		LOGd("  repeatTime=%u", entry->repeat.repeatTime);
		break;
	case SCHEDULE_TIME_TYPE_DAILY:
		LOGd("  days=%02X", entry->daily.dayOfWeekBitmask);
		break;
	}

	switch (getActionType(entry)) {
	case SCHEDULE_ACTION_TYPE_PWM:
		LOGd("  pwm=%u", entry->pwm.pwm);
		break;
	case SCHEDULE_ACTION_TYPE_FADE:
		LOGd("  pwmEnd=%u fadeTime=%u", entry->fade.pwmEnd, entry->fade.fadeDuration);
		break;
	}
}



uint16_t ScheduleList::getSize() const {
	assert(_buffer != NULL, FMT_BUFFER_NOT_ASSIGNED);
	return _buffer->size;
}

bool ScheduleList::isEmpty() const {
	return getSize() == 0;
}

bool ScheduleList::isFull() const {
	return getSize() >= MAX_SCHEDULE_ENTRIES;
}

void ScheduleList::clear() {
	assert(_buffer != NULL, FMT_BUFFER_NOT_ASSIGNED);
	_buffer->size = 0;
}

bool ScheduleList::add(const schedule_entry_t* entry) {
	bool success = false;
	for (uint16_t i=0; i<getSize(); i++) {
		if (_buffer->list[i].id == entry->id) {
			LOGi("update id %u", entry->id);
			_buffer->list[i] = *entry;
			success = true;
			break;
		}
	}
	if (!success && !isFull()) {
		_buffer->list[_buffer->size++] = *entry;
		LOGi("add id %u", entry->id);
		success = true;
	}
	return success;
}

bool ScheduleList::rem(const schedule_entry_t* entry) {
	bool success = false;
	for (uint16_t i=0; i<getSize(); i++) {
		if (_buffer->list[i].id == entry->id) {
			LOGi("rem id %u", entry->id);
			success = true;
			for (;i<_buffer->size-1; i++) {
				_buffer->list[i] = _buffer->list[i+1];
			}
			_buffer->size--;
			break;
		}
	}
	return success;
}

schedule_entry_t* ScheduleList::checkSchedule(uint32_t currentTime) {
	schedule_entry_t* result = NULL;
	for (uint16_t i=0; i<getSize(); i++) {
		if (ScheduleEntry::isActionTime(&(_buffer->list[i]), currentTime)) {
			result = &(_buffer->list[i]);
#ifdef PRINT_DEBUG
			LOGd("trigger:");
#endif
			ScheduleEntry::print(result);
			break; // Break: if we have 2 entries with the same timestamp, it will be triggered next time.
		}
	}
	return result;
}

bool ScheduleList::sync(uint32_t currentTime) {
	bool adjusted = false;
	for (uint16_t i=0; i<getSize(); i++) {
		adjusted = adjusted || ScheduleEntry::syncTime(&(_buffer->list[i]), currentTime);
	}
	return adjusted;
}

void ScheduleList::print() const {
	LOGd("Schedule list size=%u", getSize());
	for (uint16_t i=0; i<getSize(); i++) {
		ScheduleEntry::print(&(_buffer->list[i]));
	}
}
