/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 24, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>

#include "structs/cs_BufferAccessor.h"

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <util/cs_BleError.h>
#include <cfg/cs_Strings.h>

//#define PRINT_TRACKEDDEVICES_VERBOSE

/**
 * The track device timeout stops tracking after a certain number of ticks.
 * A count of 300 stands for 30 seconds
 * A count of 2000 stands for 3 minutes.
 */
#define TRACKDEVICE_DEFAULT_TIMEOUT_COUNT 300

//! Initialize counter of tracked devices with this number.
#define TDL_COUNTER_INIT                         0

#define TDL_NONE_NEARBY                          1
#define TDL_IS_NEARBY                            2
#define TDL_NOT_TRACKING                         3

//! 1 BYTE for the header = number of elements in the list
#define TRACKDEVICES_HEADER_SIZE                 1

#define TRACKDEVICES_MAX_NR_DEVICES              5

/**
 * The tracked_device_t struct contains address and RSSI value (or threshold).
 */
struct __attribute__((__packed__)) tracked_device_t {
	//! bluetooth address
	uint8_t addr[BLE_GAP_ADDR_LEN];
	//! rssi threshold
	int8_t rssiThreshold;
};

#define TRACKDEVICES_SERIALIZED_SIZE sizeof(tracked_device_t)

/**
 * The tracked_device_list_t struct defines the layout of the memory structure in the BufferAccessor object in the
 * TrackedDeviceList.
 */
struct tracked_device_list_t {
	/** number of elements */
	uint8_t size;
	/** list of tracked devices */
	tracked_device_t list[TRACKDEVICES_MAX_NR_DEVICES];
	/** list of counters (one counter per tracked device,
	 * index corresponds to tracked device list */
	uint16_t counters[TRACKDEVICES_MAX_NR_DEVICES];
};

/**
 * The TrackedDeviceList is a list of tracked devices and timing information.
 */
class TrackedDeviceList : public BufferAccessor {

private:
	/** buffer used to store the tracked devices and their counters */
	tracked_device_list_t* _buffer;
	/** defines timeout threshold */
	uint16_t _timeoutCount;

public:
	/** Default Constructor */
	TrackedDeviceList() : _buffer(NULL), _timeoutCount(TRACKDEVICE_DEFAULT_TIMEOUT_COUNT) {};

	/** Release the assigned buffer */
	void release() {

#ifdef PRINT_TRACKEDDEVICES_VERBOSE
		LOGd("release");
#endif

		_buffer = NULL;
	}

	/** Initialize tracked device list
	 *
	 * list of counters used for timeout has to be initialized to <TDL_COUNTER_INIT>
	 */
	void init() {
		if (_buffer) {
			memset(_buffer->counters, TDL_COUNTER_INIT, sizeof(_buffer->counters));
		} else {
			LOGe(FMT_BUFFER_NOT_ASSIGNED, "Buffer");
		}
	}

	/** Returns TDL_IS_NEARBY if a tracked device is nearby, also increases counters */
	uint8_t isNearby();

	/** Print a single address */
	void print(uint8_t *addr) const;

	/** Prints the list. */
	void print() const;

	void update(const uint8_t * adrs_ptr, int8_t rssi);

	/** Returns the current size of the list */
	uint16_t getSize() const;

	bool isEmpty() const;

	/** Clears the list. */
	void clear();

	/** Adds/updates an address with given rssi threshold to/in the list. Returns true on success. */
	bool add(const uint8_t* adrs_ptr, int8_t rssi_threshold);

	/** Removes an address from the list. Returns true on success, false when it's not in the list. */
	bool rem(const uint8_t* adrs_ptr);

	/** Sets the number of ticks the rssi of a device is not above threshold before a device is considered not nearby. */
	void setTimeout(uint16_t counts);

	/** Returns the number of ticks the rssi of a device is not above threshold before a device is considered not nearby. */
	uint16_t getTimeout();

	/** Resets the timeout counters **/
	void resetTimeoutCounters();

	//////////// BufferAccessor ////////////////////////////

	/** @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
		assert(sizeof(tracked_device_list_t) <= maxLength, STR_ERR_BUFFER_NOT_LARGE_ENOUGH);

#ifdef PRINT_TRACKEDDEVICES_VERBOSE
		LOGd(FMT_ASSIGN_BUFFER_LEN, buffer, maxLength);
#endif

		_buffer = (tracked_device_list_t*)buffer;
		return 0;
	}

	/** @inherit */
	uint16_t getDataLength() const {
		return TRACKDEVICES_HEADER_SIZE + TRACKDEVICES_SERIALIZED_SIZE * getSize();
	}

	/** @inherit */
	uint16_t getMaxLength() const {
    	return TRACKDEVICES_HEADER_SIZE + TRACKDEVICES_MAX_NR_DEVICES * TRACKDEVICES_SERIALIZED_SIZE;
	}

	/** @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};

/**
 * The TrackedDevice object has a single tracked_device_t struct as memory object.
 *
 * This includes RSSI value and address
 */
class TrackedDevice : public BufferAccessor {

private:
	tracked_device_t* _buffer;

public:
	/** Default Constructor */
	TrackedDevice() : _buffer(NULL) {};

	/** Release the assigned buffer */
	void release() {

#ifdef PRINT_TRACKEDDEVICES_VERBOSE
		LOGd("release");
#endif

		_buffer = NULL;
	}

	void print() const;

	int8_t getRSSI() const { return _buffer->rssiThreshold; }

	const uint8_t* getAddress() const { return _buffer->addr; }

	//////////// BufferAccessor ////////////////////////////

	/** @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
		assert(sizeof(tracked_device_t) <= maxLength, STR_ERR_BUFFER_NOT_LARGE_ENOUGH);

#ifdef PRINT_TRACKEDDEVICES_VERBOSE
		LOGd(FMT_ASSIGN_BUFFER_LEN, buffer, maxLength);
#endif

		_buffer = (tracked_device_t*)buffer;
		return 0;
	}

	/** @inherit */
	uint16_t getDataLength() const {
		return TRACKDEVICES_SERIALIZED_SIZE;
	}

	/** @inherit */
	uint16_t getMaxLength() const {
		return TRACKDEVICES_SERIALIZED_SIZE;
	}

	/** @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {

#ifdef PRINT_TRACKEDDEVICES_VERBOSE
		LOGd("getBuffer: %p", this);
#endif

		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};

