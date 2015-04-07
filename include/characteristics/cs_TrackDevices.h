/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 24, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include "ble_gap.h"
#include "ble_gatts.h"

#include "cs_BluetoothLE.h"
#include "characteristics/cs_Serializable.h"

using namespace BLEpp;

struct __attribute__((__packed__)) tracked_device_t {
	uint8_t addr[BLE_GAP_ADDR_LEN];
	int8_t rssiThreshold;
	uint16_t counter;
};

// About 3 minutes
//#define TRACKDEVICE_DEFAULT_TIMEOUT_COUNT 2000
// About 20 seconds
#define TRACKDEVICE_DEFAULT_TIMEOUT_COUNT 100


#define TRACKDEVICES_HEADER_SIZE 1 // 1 BYTE for the header = number of elements in the list
#define TRACKDEVICES_SERIALIZED_SIZE (sizeof(tracked_device_t)-2) // only works if struct packed. Subtract 2, as counter is not serialized

#define TRACKDEVICES_MAX_NR_DEVICES              5

// Initialize counter of tracked devices with this number.
#define TDL_COUNTER_INIT                         ((uint16_t)-1)

#define TDL_NONE_NEARBY                          1
#define TDL_IS_NEARBY                            2
#define TDL_NOT_TRACKING                         3

class TrackedDeviceList : public Serializable {

private:
	tracked_device_t* _list;
	uint8_t _freeIdx;
	uint16_t _timeoutCount;

public:
	TrackedDeviceList();

	~TrackedDeviceList() {
		if (_list) {
			free(_list);
		}
	}

	/** Initializes the list, must be called before any other function! */
	void init();

	bool operator!=(const TrackedDeviceList& val);

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
	void reset();

	/** Adds/updates an address with given rssi threshold to/in the list. Returns true on success. */
	bool add(const uint8_t* adrs_ptr, int8_t rssi_threshold);

	/** Removes an address from the list. Returns true on success, false when it's not in the list. */
	bool rem(const uint8_t* adrs_ptr);

	/** Sets the number of ticks the rssi of a device is not above threshold before a device is considered not nearby. */
	void setTimeout(uint16_t counts);

	/** Returns the number of ticks the rssi of a device is not above threshold before a device is considered not nearby. */
	uint16_t getTimeout();

	//////////// serializable ////////////////////////////

	/* @inherit */
    uint16_t getSerializedLength() const;

	/* @inherit */
    uint16_t getMaxLength() const {
    	return TRACKDEVICES_HEADER_SIZE + TRACKDEVICES_MAX_NR_DEVICES * TRACKDEVICES_SERIALIZED_SIZE;
    }

	/* @inherit */
    void serialize(uint8_t* buffer, uint16_t length) const;

	/* @inherit */
    void deserialize(uint8_t* buffer, uint16_t length);

};



class TrackedDevice : public Serializable {

private:
	tracked_device_t _trackedDevice;

public:
	TrackedDevice();

	void init();

	bool operator!=(const TrackedDevice& val);

	void print() const;

	int8_t getRSSI() const { return _trackedDevice.rssiThreshold; }
	
	const uint8_t* getAddress() const { return _trackedDevice.addr; }

	//////////// serializable ////////////////////////////

	/* @inherit */
    uint16_t getSerializedLength() const;

	/* @inherit */
    uint16_t getMaxLength() const {
		return TRACKDEVICES_SERIALIZED_SIZE;
    }

	/* @inherit */
    void serialize(uint8_t* buffer, uint16_t length) const;

	/* @inherit */
    void deserialize(uint8_t* buffer, uint16_t length);

};

