/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 4, 2016
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "structs/cs_ScheduleEntries.h"

uint8_t ScheduleEntry::getTimeType(const schedule_entry_t* entry) {
	uint8_t timeType = entry->type & 0x00FF;
	return timeType;
}

uint8_t ScheduleEntry::getActionType(const schedule_entry_t* entry) {
	uint8_t actionType = (entry->type & 0xFF00) >> 4;
	return actionType;
}

void ScheduleEntry::print(const schedule_entry_t* entry) {
	LOGd("id=%03u type=%02X override=%02X nextTimestamp=%u", entry->id, entry->type, entry->overrideMask, entry->nextTimestamp);
	switch (getTimeType(entry)) {
	case SCHEDULE_TIME_TYPE_REPEAT:
		LOGd("repeatTime=%u", entry->repeat.repeatTime);
		break;
	case SCHEDULE_TIME_TYPE_DAILY:
		LOGd("days=%02X nextDay=%u", entry->daily.dayOfWeek, entry->daily.nextDayOfWeek);
		break;
	}

	switch (getActionType(entry)) {
	case SCHEDULE_ACTION_TYPE_PWM:
		LOGd("pwm=%u", entry->pwm.pwm);
		break;
	case SCHEDULE_ACTION_TYPE_FADE:
		LOGd("pwmEnd=%u fadeTime=%u", entry->fade.pwmEnd, entry->fade.fadeDuration);
		break;
	}
}



void ScheduleList::clear() {
	_buffer->size = 0;
}

bool ScheduleList::add(const schedule_entry_t* entry) {
	bool success = false;
	for (uint8_t i=0; i<_buffer->size; i++) {
		if (_buffer->list[i].id == entry->id) {
			LOGd("update id %u", entry->id);
			_buffer->list[i] = *entry;
			success = true;
			break;
		}
	}
	if (!success && getSize() < MAX_SCHEDULE_ENTRIES) {
		_buffer->list[_buffer->size++] = *entry;
		LOGd("add id %u", entry->id);
		success = true;
	}
	return success;
}

bool ScheduleList::rem(const schedule_entry_t* entry) {
	bool success = false;
	for (uint8_t i=0; i<_buffer->size; i++) {
		if (_buffer->list[i].id == entry->id) {
			LOGd("rem id %u", entry->id);
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
	for (uint8_t i=0; i<getSize(); i++) {
		if (_buffer->list[i].nextTimestamp == 0) {
			//TODO: remove this entry
		}
		if (_buffer->list[i].nextTimestamp < currentTime) {
			result = &(_buffer->list[i]);

			switch (ScheduleEntry::getTimeType(result)) {
			case SCHEDULE_TIME_TYPE_REPEAT:
				result->nextTimestamp += result->repeat.repeatTime * 60;
				break;
			case SCHEDULE_TIME_TYPE_DAILY:
				// Daily task, reschedule in 24h
				result->nextTimestamp += SECONDS_PER_DAY;
				// Check day of week
				if (!(result->daily.dayOfWeek & DAILY_REPEAT_ALL_DAYS) && !(result->daily.dayOfWeek & (1 << result->daily.nextDayOfWeek))) {
					result = NULL;
				}
				// Keep up the next day of week
				_buffer->list[i].daily.nextDayOfWeek = (_buffer->list[i].daily.nextDayOfWeek + 1) % 7;
				break;
			case SCHEDULE_TIME_TYPE_ONCE:
				result->nextTimestamp = 0; // Mark for deletion
				break;
			}
		}
	}
	return result;
}

void ScheduleList::sync(uint32_t currentTime) {
	for (uint8_t i=0; i<getSize(); i++) {
		if (_buffer->list[i].nextTimestamp == 0) {
			//TODO: remove this entry
		}
		//! Make sure nextTimestamp > currentTime
		if (_buffer->list[i].nextTimestamp < currentTime) {
			switch (ScheduleEntry::getTimeType(result)) {
			case SCHEDULE_TIME_TYPE_REPEAT:
				_buffer->list[i].nextTimestamp += ((currentTime - _buffer->list[i].nextTimestamp) / _buffer->list[i].repeat.repeatTime + 1) * _buffer->list[i].repeat.repeatTime;
				break;
			case SCHEDULE_TIME_TYPE_DAILY:
				uint32_t daysDiff = (currentTime - _buffer->list[i].nextTimestamp) / SECONDS_PER_DAY + 1;
				_buffer->list[i].nextTimestamp += daysDiff * SECONDS_PER_DAY;

				// Keep up the next day of week
				// TODO: is this correct?
				_buffer->list[i].daily.nextDayOfWeek = (_buffer->list[i].daily.nextDayOfWeek + daysDiff) % 7;
				break;
			case SCHEDULE_TIME_TYPE_ONCE:
				// TODO: remove this entry immediately
				_buffer->list[i].nextTimestamp = 0; // Mark for deletion
				break;
			}
		}
	}
}

void ScheduleList::print() const {
	LOGd("Schedule list size=%u", _buffer->size);
	for (uint8_t i=0; i<getSize(); i++) {
		ScheduleEntry::print(&(_buffer->list[i]));
	}
}
