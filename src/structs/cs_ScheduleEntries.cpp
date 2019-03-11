/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 4, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "structs/cs_ScheduleEntries.h"
#include <util/cs_BleError.h>

#define PRINT_DEBUG

bool ScheduleEntry::isActionTime(schedule_entry_t* entry, uint32_t timestamp) {
	if (entry->nextTimestamp == 0) {
		return false;
	}
	if (entry->nextTimestamp >= timestamp) {
		return false;
	}
	bool result = true;
#ifdef PRINT_DEBUG
	LOGd("found:");
	print(entry);
#endif
	switch (getTimeType(entry)) {
	case SCHEDULE_TIME_TYPE_REPEAT:
		break;
	case SCHEDULE_TIME_TYPE_DAILY: {
		// Check day of week
		uint8_t dayOfWeek = getDayOfWeek(entry->nextTimestamp);
		if (!isActionDay(entry->daily.dayOfWeekBitmask, dayOfWeek)) {
			result = false;
		}
		break;
	}
	case SCHEDULE_TIME_TYPE_ONCE:
		break;
	default:
		result = false;
		break;
	}
	return result;
}

void ScheduleEntry::adjustTimestamp(schedule_entry_t* entry, uint32_t timestamp) {
	if (entry->nextTimestamp == 0) {
		return;
	}
	if (entry->nextTimestamp >= timestamp) {
		return;
	}
	switch (getTimeType(entry)) {
	case SCHEDULE_TIME_TYPE_REPEAT:
		entry->nextTimestamp += entry->repeat.repeatTime * 60;
		break;
	case SCHEDULE_TIME_TYPE_DAILY: {
		// Daily task, reschedule in 24h
		entry->nextTimestamp += SECONDS_PER_DAY;
		break;
	}
	case SCHEDULE_TIME_TYPE_ONCE:
		entry->nextTimestamp = 0; // Clear entry
		break;
	default:
		entry->nextTimestamp = 0; // Clear entry
		break;
	}
#ifdef PRINT_DEBUG
	LOGd("Updated nextTimestamp=%u", entry->nextTimestamp);
#endif
}

bool ScheduleEntry::syncTime(schedule_entry_t* entry, uint32_t timestamp, int64_t timeDiff) {
	if (entry->nextTimestamp == 0) {
		return false;
	}
	bool adjusted = false;

	// TODO: what to do with a large negative timeDiff ?

	// Make sure nextTimestamp >= currentTime
	if (entry->nextTimestamp < timestamp) {
		switch (ScheduleEntry::getTimeType(entry)) {
		case SCHEDULE_TIME_TYPE_REPEAT: {
			uint32_t secondsPerRepeat = entry->repeat.repeatTime * 60;
			if (timeDiff <= SCHEDULE_BIG_TIME_JUMP && secondsPerRepeat/2 > timeDiff) {
				// For small time jumps, we don't want to skip the action of entries that may have happened in between.
				// But the repeat time has to be large enough not to trigger twice.
				break;
			}
			uint32_t repeatDiff = (timestamp - entry->nextTimestamp + secondsPerRepeat - 1) / secondsPerRepeat;
			entry->nextTimestamp += repeatDiff * secondsPerRepeat;
			adjusted = (repeatDiff != 0);
			break;
		}
		case SCHEDULE_TIME_TYPE_DAILY:{
			if (timeDiff <= SCHEDULE_BIG_TIME_JUMP) {
				// For small time jumps, we don't want to skip the action of entries that may have happened in between.
				break;
			}
			uint32_t daysDiff = (timestamp - entry->nextTimestamp + SECONDS_PER_DAY - 1) / SECONDS_PER_DAY;
			entry->nextTimestamp += daysDiff * SECONDS_PER_DAY;
			adjusted = (daysDiff != 0);
			break;
		}
		case SCHEDULE_TIME_TYPE_ONCE:
			if (timeDiff <= SCHEDULE_BIG_TIME_JUMP) {
				// For small time jumps, we don't want to skip the action of entries that may have happened in between.
				break;
			}
			entry->nextTimestamp = 0; // Clear entry
			break;
		default:
			entry->nextTimestamp = 0; // Clear entry
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

bool ScheduleEntry::isValidEntry(const schedule_entry_t* entry) {
//	if (entry->nextTimestamp == 0) {
//		return false;
//	}
	switch (getTimeType(entry)) {
	case SCHEDULE_TIME_TYPE_REPEAT:
		if (entry->repeat.repeatTime < 1) {
			return false;
		}
		break;
	case SCHEDULE_TIME_TYPE_DAILY:
		if (entry->daily.dayOfWeekBitmask == 0) {
			return false;
		}
		break;
	case SCHEDULE_TIME_TYPE_ONCE:
		break;
	default:
		return false;
	}

	switch (getActionType(entry)) {
	case SCHEDULE_ACTION_TYPE_PWM:
		break;
	case SCHEDULE_ACTION_TYPE_FADE:
		break;
	case SCHEDULE_ACTION_TYPE_TOGGLE:
		break;
	default:
		return false;
	}
	return true;
}

// Calculate day of week:
// See: http://stackoverflow.com/questions/36357013/day-of-week-from-seconds-since-epoch
// With timestamp=0 = Thursday 1-January-1970 00:00:00
// (timestamp/(24*3600)+4)%7
uint8_t ScheduleEntry::getDayOfWeek(uint32_t timestamp) {
	return (timestamp / SECONDS_PER_DAY + 4) % 7;
}

bool ScheduleEntry::isActionDay(uint8_t bitMask, uint8_t dayOfWeek) {
#ifdef PRINT_DEBUG
	LOGd("isActionDay: 0x%02x %u", bitMask, dayOfWeek);
#endif
	return (bitMask & (1 << DAILY_REPEAT_BIT_ALL_DAYS)) || (bitMask & (1 << dayOfWeek));
}

void ScheduleEntry::print(const schedule_entry_t* entry) {

	LOGd("type=0x%02x override=0x%02x nextTimestamp=%u", entry->type, entry->overrideMask, entry->nextTimestamp);

	switch (getTimeType(entry)) {
	case SCHEDULE_TIME_TYPE_REPEAT:
		LOGd("  repeatTime=%u", entry->repeat.repeatTime);
		break;
	case SCHEDULE_TIME_TYPE_DAILY:
		LOGd("  days=0x%02x", entry->daily.dayOfWeekBitmask);
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
	return _buffer->size;
}

void ScheduleList::clear() {
	for (uint16_t i=0; i<MAX_SCHEDULE_ENTRIES; ++i) {
		_buffer->list[i].nextTimestamp = 0;
	}
	_buffer->size = MAX_SCHEDULE_ENTRIES;
}

bool ScheduleList::checkAllEntries() {
	bool adjusted = _buffer->size != MAX_SCHEDULE_ENTRIES;

	// If entry is invalid: clear the entry
	for (uint8_t i=0; i<_buffer->size; ++i) {
		if (!ScheduleEntry::isValidEntry(&(_buffer->list[i]))) {
			clear(i);
			adjusted = true;
		}
	}

	// Make sure all entries exist
	for (uint8_t i=_buffer->size; i<MAX_SCHEDULE_ENTRIES; ++i) {
		_buffer->list[i].nextTimestamp = 0;
	}
	_buffer->size = MAX_SCHEDULE_ENTRIES;

	return adjusted;
}

bool ScheduleList::set(uint8_t index, const schedule_entry_t* entry) {
	if (index >= MAX_SCHEDULE_ENTRIES) {
		return false;
	}
	LOGi("set index %u", index);
	_buffer->size = MAX_SCHEDULE_ENTRIES;
	if (!ScheduleEntry::isValidEntry(entry)) {
		return false;
	}
	memcpy(&(_buffer->list[index]), entry, sizeof(schedule_entry_t));
	return true;
}

bool ScheduleList::clear(uint8_t index) {
	if (index >= MAX_SCHEDULE_ENTRIES) {
		return false;
	}
	_buffer->list[index].nextTimestamp = 0;
	return true;
}

schedule_entry_t* ScheduleList::isActionTime(uint32_t currentTime) {
	schedule_entry_t* result = NULL;
	uint32_t largestTimestamp = 0;
	for (uint16_t i=0; i<getSize(); ++i) {
		if (ScheduleEntry::isActionTime(&(_buffer->list[i]), currentTime)) {
			if (_buffer->list[i].nextTimestamp > largestTimestamp) {
				result = &(_buffer->list[i]);
				largestTimestamp = _buffer->list[i].nextTimestamp;
			}
		}
		ScheduleEntry::adjustTimestamp(&(_buffer->list[i]), currentTime);
	}
#ifdef PRINT_DEBUG
	if (result != NULL) {
		LOGd("trigger:");
		ScheduleEntry::print(result);
	}
#endif
	return result;
}

bool ScheduleList::sync(uint32_t currentTime, int64_t timeDiff) {
	LOGd("sync");
	bool adjusted = false;
	for (uint16_t i=0; i<getSize(); i++) {
		bool entryAdjusted = ScheduleEntry::syncTime(&(_buffer->list[i]), currentTime, timeDiff);
		adjusted = entryAdjusted || adjusted;
	}
	return adjusted;
}

void ScheduleList::print() const {
	LOGd("Schedule list size=%u", getSize());
	for (uint16_t i=0; i<getSize(); i++) {
		ScheduleEntry::print(&(_buffer->list[i]));
	}
}
