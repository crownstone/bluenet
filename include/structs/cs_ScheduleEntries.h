/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 1, 2016
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include "cfg/cs_Config.h"
#include "structs/cs_BufferAccessor.h"
#include <drivers/cs_Serial.h>
#include <util/cs_BleError.h>

//#include <time.h>

#define SECONDS_PER_DAY         86400

#define DAILY_REPEAT_SUNDAYS    1
#define DAILY_REPEAT_MONDAYS    2
#define DAILY_REPEAT_TUESDAYS   4
#define DAILY_REPEAT_WEDNESDAYS 8
#define DAILY_REPEAT_THURSDAYS  16
#define DAILY_REPEAT_FRIDAYS    32
#define DAILY_REPEAT_SATURDAYS  64
#define DAILY_REPEAT_ALL_DAYS   128

#define SCHEDULE_TIME_TYPE_REPEAT      0
#define SCHEDULE_TIME_TYPE_DAILY       1
#define SCHEDULE_TIME_TYPE_ONCE        2

#define SCHEDULE_ACTION_TYPE_PWM       0
#define SCHEDULE_ACTION_TYPE_FADE      1
#define SCHEDULE_ACTION_TYPE_TOGGLE    2


#define SCHEDULE_OVERRIDE_PRESENCE     1
#define SCHEDULE_OVERRIDE_RESERVED     2
#define SCHEDULE_OVERRIDE_RESERVED2    4
#define SCHEDULE_OVERRIDE_RESERVED3    8
#define SCHEDULE_OVERRIDE_RESERVED4    16
#define SCHEDULE_OVERRIDE_RESERVED5    32
#define SCHEDULE_OVERRIDE_RESERVED6    64
#define SCHEDULE_OVERRIDE_RESERVED7    128

struct __attribute__((__packed__)) schedule_time_daily_t {
	//! Only perform action on certain days these days of the week. Bitmask, see DAILY_REPEAT_*.
	//! Check against (1 << current_day_of_week)
	//! If (dayOfWeek & DAILY_REPEAT_ALL_DAYS), then the other bits are ignored.
	uint8_t dayOfWeek;
	uint8_t nextDayOfWeek; //! [0-6] 0=Sunday. Remember what day of week the nextTimestamp is.
};

struct __attribute__((__packed__)) schedule_time_repeat_t {
	uint16_t repeatTime; //! Repeat every X minutes. 0 is not allowed.
};

struct __attribute__((__packed__)) schedule_action_pwm_t {
	uint8_t pwm;
	uint8_t reserved[2];
};

struct __attribute__((__packed__)) schedule_action_fade_t {
	uint8_t pwmEnd;
	uint16_t fadeDuration; //! Number of seconds it takes to get to pwmEnd.
};

//struct __attribute__((__packed__)) schedule_action_t {
//	union {
//		schedule_action_pwm_t pwm;
//		schedule_action_fade_t fade;
//	};
//};

struct __attribute__((__packed__)) schedule_entry_t {
	uint8_t id;
	//! Combined time and action type.
	//! Defined as SCHEDULE_TIME_TYPE_.. + (SCHEDULE_ACTION_TYPE_.. << 4)
	uint8_t type;

	uint8_t overrideMask; //! What to override. Bitmask, see SCHEDULE_OVERRIDE_*

	uint32_t nextTimestamp;
	union {
		schedule_time_repeat_t repeat;
		schedule_time_daily_t daily;
	};
	union {
		schedule_action_pwm_t pwm;
		schedule_action_fade_t fade;
	};
};


//! Size: 2 + 12*MAX_SCHEDULE_ENTRIES
struct __attribute__((__packed__)) schedule_list_t {
	uint8_t size;
	schedule_entry_t list[MAX_SCHEDULE_ENTRIES];
};


// Calculate day of week:
// See: http://stackoverflow.com/questions/36357013/day-of-week-from-seconds-since-epoch
// With timestamp=0 = Thursday 1-January-1970 00:00:00
// (timestamp/(24*3600)+4)%7




#define SCHEDULE_ENTRY_SERIALIZED_SIZE sizeof(schedule_entry_t)

#define SCHEDULE_LIST_HEADER_SIZE 1
#define SCHEDULE_LIST_SERIALIZED_SIZE sizeof(schedule_list_t)

/**
 * The ScheduleEntry object has a single schedule_entry_t struct as memory object.
 */
class ScheduleEntry : public BufferAccessor {

private:
	schedule_entry_t* _buffer;

public:
	/** Default Constructor */
	ScheduleEntry() : _buffer(NULL) {};

	/** Release the assigned buffer */
	void release() {
		LOGd("release");
		_buffer = NULL;
	}

	schedule_entry_t* getStruct() { return _buffer; }

	static void print(const schedule_entry_t* entry);
	static uint8_t getTimeType(const schedule_entry_t* entry);
	static uint8_t getActionType(const schedule_entry_t* entry);


	//////////// BufferAccessor ////////////////////////////

	/** @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
		LOGd("assign buff: %p, len: %d", buffer, maxLength);
		assert(sizeof(schedule_entry_t) <= maxLength, "buffer not large enough to hold schedule entry!");
		if (sizeof(schedule_entry_t) > maxLength) {
			return 1;
		}
		_buffer = (schedule_entry_t*)buffer;
		return 0;
	}

	/** @inherit */
	uint16_t getDataLength() const {
		return SCHEDULE_ENTRY_SERIALIZED_SIZE;
	}

	/** @inherit */
	uint16_t getMaxLength() const {
		return SCHEDULE_ENTRY_SERIALIZED_SIZE;
	}

	/** @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		LOGd("getBuffer: %p", this);
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};


/**
 * The ScheduleList object is a list of schedule entries.
 */
class ScheduleList : public BufferAccessor {

private:
	/** Buffer used to store the schedule entries */
	schedule_list_t* _buffer;

public:
	/** Default Constructor */
	ScheduleList() : _buffer(NULL) {
		_buffer->size = 0;
	};

	/** Release the assigned buffer */
	void release() {
		LOGd("release");
		_buffer = NULL;
	}

	/** Returns the current size of the list */
	uint16_t getSize() const { return _buffer->size; }

	/** Returns true if the list is empty. */
	bool isEmpty() const { return _buffer->size == 0; }

	/** Clears the list. */
	void clear();

	/** Adds/updates a schedule entry to/in the list. Returns true on success. */
	bool add(const schedule_entry_t* entry);

	/** Removes a schedule entry from the list. Returns true on success, false when it's not in the list. */
	bool rem(const schedule_entry_t* entry);

	/** Checks the schedule entries with the current time. Returns an entry if its action has to be executed, NULL otherwise. */
	schedule_entry_t* checkSchedule(uint32_t currentTime);

	/** Prints the schedule entry list */
	void print() const;

	////////////! BufferAccessor ////////////////////////////

	/** @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
		LOGd("assign buff: %p, len: %d", buffer, maxLength);
		assert(sizeof(schedule_list_t) <= maxLength, "buffer not large enough to hold schedule entries list!");
		_buffer = (schedule_list_t*)buffer;
		return 0;
	}

	/** @inherit */
	uint16_t getDataLength() const {
		return SCHEDULE_LIST_HEADER_SIZE + SCHEDULE_ENTRY_SERIALIZED_SIZE * getSize();
	}

	/** @inherit */
	uint16_t getMaxLength() const {
    	return SCHEDULE_LIST_SERIALIZED_SIZE;
	}

	/** @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};
