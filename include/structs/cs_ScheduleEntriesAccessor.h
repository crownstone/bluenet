/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 1, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/cs_ScheduleEntries.h>
#include "cfg/cs_Config.h"
#include "structs/cs_BufferAccessor.h"
#include <drivers/cs_Serial.h>
#include <util/cs_BleError.h>
#include <cfg/cs_Strings.h>

#define PRINT_SCHEDULEENTRIES_VERBOSE

#define SECONDS_PER_DAY         86400


// More action ideas:
// scanning
// respond to linked devices
// advertising
// recovery
// lock
// device type
// max power usage





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

#ifdef PRINT_SCHEDULEENTRIES_VERBOSE
		LOGd("release");
#endif

		_buffer = NULL;
	}

	schedule_entry_t* getStruct() { return _buffer; }

	static void print(const schedule_entry_t* entry);
	static uint8_t getTimeType(const schedule_entry_t* entry);
	static uint8_t getActionType(const schedule_entry_t* entry);
	static bool isValidEntry(const schedule_entry_t* entry);

	/** Checks if this schedule entry should be executed. */
	static bool isActionTime(schedule_entry_t* entry, uint32_t currentTime);

	/** Adjusts the timestamp to next scheduled time (but only if timestamp < currentTime) */
	static void adjustTimestamp(schedule_entry_t* entry, uint32_t currentTime);

	/** After large time jumps, this function adjusts the timestamp to next scheduled time.
	 * Returns whether the schedule was adjusted
	 */
	static bool syncTime(schedule_entry_t* entry, uint32_t currentTime, int64_t timeDiff);

	/** Get the day of week of a given timestamp. Sun=0, Sat=6. */
	static uint8_t getDayOfWeek(uint32_t timestamp);

	/** Whether or not the day is one in the dayOfWeek bitmask */
	static bool isActionDay(uint8_t bitMask, uint8_t dayOfWeek);


	//////////// BufferAccessor ////////////////////////////

	/** @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
		assert(sizeof(schedule_entry_t) <= maxLength, STR_ERR_BUFFER_NOT_LARGE_ENOUGH);

#ifdef PRINT_SCHEDULEENTRIES_VERBOSE
		LOGd(FMT_ASSIGN_BUFFER_LEN, buffer, maxLength);
#endif

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

#ifdef PRINT_SCHEDULEENTRIES_VERBOSE
		LOGd("getBuffer: %p", this);
#endif

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
	};

	/** Release the assigned buffer */
	void release() {

#ifdef PRINT_SCHEDULEENTRIES_VERBOSE
		LOGd("release");
#endif

		_buffer = NULL;
	}

	/** Returns the current size of the list */
	uint16_t getSize() const {
		return _buffer->size;
	}

	/** Clears the list. */
	void clear();

	/** Adds/updates a schedule entry to/in the list. Returns true on success. */
	bool set(uint8_t index, const schedule_entry_t* entry);

	/** Removes a schedule entry from the list. Returns true on success. */
	bool clear(uint8_t index);

	/** Checks validity of all entries */
	bool checkAllEntries();

	/** Checks the schedule entries with the current time.
	 * Also reschedules the entries if they are repeated.
	 * If multiple entries are found, the one with the biggest timestamp will be returned.
	 * Returns an entry if its action has to be executed, NULL otherwise.
	 */
	schedule_entry_t* isActionTime(uint32_t currentTime);

	/** If there is a time jump, this function makes sure all entries are corrected.
	 * Returns whether any schedule was adjusted.
	 */
	bool sync(uint32_t currentTime, int64_t timeDiff);

	/** Prints the schedule entry list */
	void print() const;

	//////////// BufferAccessor ////////////////////////////

	/** @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
		assert(sizeof(schedule_list_t) <= maxLength, STR_ERR_BUFFER_NOT_LARGE_ENOUGH);

#ifdef PRINT_SCHEDULEENTRIES_VERBOSE
		LOGd(FMT_ASSIGN_BUFFER_LEN, buffer, maxLength);
#endif

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
