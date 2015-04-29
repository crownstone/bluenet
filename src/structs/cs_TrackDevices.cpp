/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 24, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

//#include <string.h>
//#include <stdio.h>
//
#include "structs/cs_TrackDevices.h"
//#include "drivers/cs_Serial.h"

/**********************************************************************************************************************
 * List of tracked devices
 *********************************************************************************************************************/

// returns the number of elements stored so far
uint16_t TrackedDeviceList::getSize() const {
	if (_buffer != NULL) {
		return _buffer->size;
	} else {
		return 0;
	}
}

bool TrackedDeviceList::isEmpty() const {
	return getSize() == 0;
}

void TrackedDeviceList::update(const uint8_t * addrs_ptr, int8_t rssi) {
	//bool found = false;
//	LOGd("scanned: [%02X %02X %02X %02X %02X %02X], rssi: %d",
//			addrs_ptr[5], addrs_ptr[4], addrs_ptr[3], addrs_ptr[2],
//			addrs_ptr[1], addrs_ptr[0], rssi);
	for (int i = 0; i < getSize(); ++i) {
		if (memcmp(addrs_ptr, _buffer->list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
			if (rssi >= _buffer->list[i].rssiThreshold) {
				_buffer->counters[i] = 0;
				LOGd("Tracked device present nearby (%i >= %i)", rssi, _buffer->list[i].rssiThreshold);
			} else {
//				LOGd("Tracked device found, but not nearby (%i < %i)", rssi, _buffer->list[i].rssiThreshold);
			}
			//found = true;
			break;
		}
	}
	/*
	if (!found) {
		LOGd("Device found, but not to-be tracked:");
		print(addrs_ptr);
		LOGd("To-be tracked devices:");
		for (int i = 0; i < getSize(); ++i) {
			print(_buffer->list[i].addr);
		}
	}
	*/
}

uint8_t TrackedDeviceList::isNearby() {
	// special case in which nothing needs to be tracked
	if (!getSize()) return TDL_NOT_TRACKING;
	
	bool is_nearby = false;
	uint16_t result;
//	print();
	for (int i = 0; i < getSize(); ++i) {
		if (_buffer->counters[i] >= _timeoutCount) {
		} else {
			_buffer->counters[i]++;
			is_nearby = true;
		}
	}
	result = (is_nearby ? TDL_IS_NEARBY : TDL_NONE_NEARBY);
	return result;
}

void TrackedDeviceList::print(uint8_t *addr) const {
	LOGd("[%02X %02X %02X %02X %02X %02X]",
			addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

void TrackedDeviceList::print() const {
	for (int i = 0; i < getSize(); ++i) {
		LOGd("[%02X %02X %02X %02X %02X %02X]\trssi: %d\tstate: %d", _buffer->list[i].addr[5],
				_buffer->list[i].addr[4], _buffer->list[i].addr[3], _buffer->list[i].addr[2], _buffer->list[i].addr[1],
				_buffer->list[i].addr[0], _buffer->list[i].rssiThreshold, _buffer->counters[i]);
	}
}

void TrackedDeviceList::clear() {
	memset(_buffer, 0, sizeof(tracked_device_list_t));
}

bool TrackedDeviceList::add(const uint8_t* adrs_ptr, int8_t rssi_threshold) {
	for (int i=0; i<getSize(); ++i) {
		// check if it is already in the list, then update
		if (memcmp(adrs_ptr, _buffer->list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
			_buffer->list[i].rssiThreshold = rssi_threshold;
			//_buffer->counters[i] = TDL_COUNTER_INIT; // Don't update counter
			LOGi("Updated [%02X %02X %02X %02X %02X %02X], rssi threshold: %d",
					adrs_ptr[5], adrs_ptr[4], adrs_ptr[3], adrs_ptr[2],
					adrs_ptr[1], adrs_ptr[0], rssi_threshold);
			return true;
		}
	}
	if (getSize() >= TRACKDEVICES_MAX_NR_DEVICES) {
		return false; // List is full
	}

	// get next index, then increase size by one
	int idx = _buffer->size++;

	memcpy(_buffer->list[idx].addr, adrs_ptr, BLE_GAP_ADDR_LEN);
	_buffer->list[idx].rssiThreshold = rssi_threshold;
	_buffer->counters[idx] = TDL_COUNTER_INIT;
//	LOGi("Added [%02X %02X %02X %02X %02X %02X], rssi threshold: %d",
//			_buffer->list[_freeIdx].addr[5], _buffer->list[_freeIdx].addr[4], _buffer->list[_freeIdx].addr[3], _buffer->list[_freeIdx].addr[2],
//			_buffer->list[_freeIdx].addr[1], _buffer->list[_freeIdx].addr[0], rssi_threshold);
	LOGi("Added [%02X %02X %02X %02X %02X %02X], rssi threshold: %d",
			adrs_ptr[5], adrs_ptr[4], adrs_ptr[3], adrs_ptr[2],
			adrs_ptr[1], adrs_ptr[0], rssi_threshold);

	if (memcmp(adrs_ptr, _buffer->list[idx].addr, BLE_GAP_ADDR_LEN)) {
		LOGi("Adding the address did not work, very likely due to memory issues!");
		return false;
	}

	return true;
}

bool TrackedDeviceList::rem(const uint8_t* adrs_ptr) {
	for (int i=0; i<getSize(); ++i) {
		if (memcmp(adrs_ptr, _buffer->list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
			// Decrease size
			_buffer->size--;
			// Shift array
			// TODO: Anne: order within the array shouldn't matter isn't it? so just copy the last item on slot i
			for (int j=i; j<getSize(); ++j) {
				memcpy(_buffer->list[j].addr, _buffer->list[j+1].addr, BLE_GAP_ADDR_LEN);
				_buffer->list[j].rssiThreshold = _buffer->list[j+1].rssiThreshold;
				_buffer->counters[j] = _buffer->counters[j+1];

				// TODO: does this work too? might be faster..
				//memcpy(&(_buffer->list[j]), &(_buffer->list[j+1]), sizeof(tracked_device_t));
			}
			LOGi("Removed [%02X %02X %02X %02X %02X %02X]", adrs_ptr[5],
					adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1], adrs_ptr[0]);
			return true;
		}
	}
	return false; // Address is not in the list
}

void TrackedDeviceList::setTimeout(uint16_t counts) {
	if (counts > TDL_COUNTER_INIT) {
		return;
	}

	// Make sure that devices that are not nearby get the new threshold count
	for (int i=0; i<getSize(); ++i) {
		if (_buffer->counters[i] >= _timeoutCount) {
			_buffer->counters[i] = counts;
		}
	}

	_timeoutCount = counts;
	LOGi("Set timeout count to %i", counts);
}

uint16_t TrackedDeviceList::getTimeout() {
	return _timeoutCount;
}


/**********************************************************************************************************************
 * A specific tracked device
 *********************************************************************************************************************/

/**
 * From a device we track
 *   RSSI level
 *   the Bluetooth address
 */

void TrackedDevice::print() const {
	LOGi("[%02X %02X %02X %02X %02X %02X]\trssi: %d", _buffer->addr[5],
		_buffer->addr[4], _buffer->addr[3], _buffer->addr[2], _buffer->addr[1],
		_buffer->addr[0], _buffer->rssiThreshold);
}

