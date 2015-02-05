/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 24, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <string.h>
#include <stdio.h>

#include "characteristics/cs_TrackDevices.h"
#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"

/**********************************************************************************************************************
 * List of tracked devices
 *********************************************************************************************************************/

TrackedDeviceList::TrackedDeviceList() : _list(NULL), _freeIdx(0) {
}

void TrackedDeviceList::init() {
	LOGd("Initialize tracked device list");
	_freeIdx = 0;
	if (_list) {
		free(_list);
	}
	_list = (tracked_device_t*)calloc(TRACKDEVICES_MAX_NR_DEVICES, sizeof(tracked_device_t));
}

// returns the number of elements stored so far
uint16_t TrackedDeviceList::getSize() const {
	// freeIdx points to the next free index of the array,
	// but because the first element is at index 0,
	// freeIdx is also the number of elements in the array so far
	return _freeIdx;
}

bool TrackedDeviceList::operator!=(const TrackedDeviceList& val) {
	if (_freeIdx != val._freeIdx) {
		return true;
	}
	for (int i = 0; i < getSize(); ++i) {
		if (_list[i].rssiThreshold != val._list[i].rssiThreshold) {
			return true;
		}
		if (_list[i].counter != val._list[i].counter) {
			return true;
		}
		if (memcmp(this->_list[i].addr, val._list[i].addr, BLE_GAP_ADDR_LEN) != 0) {
			return true;
		}
	}
	return false;
}

void TrackedDeviceList::update(const uint8_t * addrs_ptr, int8_t rssi) {
	//bool found = false;
	for (int i = 0; i < getSize(); ++i) {
		if (memcmp(addrs_ptr, _list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
			if (rssi >= _list[i].rssiThreshold) {
				_list[i].counter = 0;
				LOGd("Tracked device present nearby (%i >= %i)", rssi, _list[i].rssiThreshold);
			} else {
				LOGd("Tracked device found, but not nearby (%i < %i)", rssi, _list[i].rssiThreshold);
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
			print(_list[i].addr);
		}
	}
	*/
}

uint8_t TrackedDeviceList::isAlone() {
	// special case in which nothing needs to be tracked
	if (!getSize()) return TDL_NOT_TRACKING;
	
	bool none_nearby = true;
	uint16_t result;
	for (int i = 0; i < getSize(); ++i) {
		uint16_t count = _list[i].counter;
		_list[i].counter++;
		none_nearby = none_nearby && (count > ALONE_THRESHOLD);
	}
	result = (none_nearby ? TDL_IS_ALONE : TDL_SOMEONE_NEARBY);
	return result;
}

void TrackedDeviceList::print(uint8_t *addr) const {
	LOGd("[%02X %02X %02X %02X %02X %02X]", 
			addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

void TrackedDeviceList::print() const {
	for (int i = 0; i < getSize(); ++i) {
		LOGi("[%02X %02X %02X %02X %02X %02X]\trssi: %d\tstate: %d", _list[i].addr[5],
				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
				_list[i].addr[0], _list[i].rssiThreshold, _list[i].counter);
	}
}

void TrackedDeviceList::reset() {
	memset(_list, 0, TRACKDEVICES_MAX_NR_DEVICES * sizeof(tracked_device_t));
	_freeIdx = 0;
}

bool TrackedDeviceList::add(const uint8_t* adrs_ptr, int8_t rssi_threshold) {
	for (int i=0; i<getSize(); ++i) {
		if (memcmp(adrs_ptr, _list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
			_list[i].rssiThreshold = rssi_threshold;
			_list[i].counter = ALONE_THRESHOLD;
			LOGi("Updated [%02X %02X %02X %02X %02X %02X], rssi threshold: %d",
					adrs_ptr[5], adrs_ptr[4], adrs_ptr[3], adrs_ptr[2],
					adrs_ptr[1], adrs_ptr[0], rssi_threshold);
			return true;
		}
	}
	if (getSize() >= TRACKDEVICES_MAX_NR_DEVICES) {
		return false; // List is full
	}
	memcpy(_list[_freeIdx].addr, adrs_ptr, BLE_GAP_ADDR_LEN);
	_list[_freeIdx].rssiThreshold = rssi_threshold;
	_list[_freeIdx].counter = ALONE_THRESHOLD;
	LOGi("Added [%02X %02X %02X %02X %02X %02X], rssi threshold: %d",
			_list[_freeIdx].addr[5], _list[_freeIdx].addr[4], _list[_freeIdx].addr[3], _list[_freeIdx].addr[2],
			_list[_freeIdx].addr[1], _list[_freeIdx].addr[0], rssi_threshold);
	LOGi("Added [%02X %02X %02X %02X %02X %02X], rssi threshold: %d",
			adrs_ptr[5], adrs_ptr[4], adrs_ptr[3], adrs_ptr[2],
			adrs_ptr[1], adrs_ptr[0], rssi_threshold);

	if (memcmp(adrs_ptr, _list[_freeIdx].addr, BLE_GAP_ADDR_LEN)) {
		LOGi("Adding the address did not work, very likely due to memory issues!");
		return false;
	}

	++_freeIdx;
	return true;
}

bool TrackedDeviceList::rem(uint8_t* adrs_ptr) {
	for (int i=0; i<getSize(); ++i) {
		if (memcmp(adrs_ptr, _list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
			// Decrease size
			--_freeIdx;
			// Shift array
			// TODO: Anne: order within the array shouldn't matter isn't it? so just copy the last item on slot i
			for (int j=i; j<getSize(); ++j) {
				memcpy(_list[j].addr, _list[j+1].addr, BLE_GAP_ADDR_LEN);
				_list[j].rssiThreshold = _list[j+1].rssiThreshold;
				_list[j].counter = _list[j+1].counter;

				// TODO: does this work too? might be faster..
				//memcpy(&(_list[j]), &(_list[j+1]), sizeof(tracked_device_t));
			}
			LOGi("Removed [%02X %02X %02X %02X %02X %02X]", adrs_ptr[5],
					adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1], adrs_ptr[0]);
			return true;
		}
	}
	return false; // Address is not in the list
}

/** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
* it means that the object does not need external buffer space. */
uint32_t TrackedDeviceList::getSerializedLength() const {
	return getSize() * TRACKDEVICES_SERIALIZED_SIZE + TRACKDEVICES_HEADER_SIZE;
}

/** Copy data representing this object into the given buffer.  Buffer will be preallocated with at least
* getLength() bytes. */
void TrackedDeviceList::serialize(uint8_t* buffer, uint16_t length) const {
	uint8_t *ptr;
	ptr = buffer;

	// copy number of elements
	*ptr++ = getSize();
	for (int i = 0; i < getSize(); ++i) {
		// copy address of device
		for (uint8_t j= 1; j <= BLE_GAP_ADDR_LEN; ++j) {
			*ptr++ = _list[i].addr[BLE_GAP_ADDR_LEN - j];
		}
		// copy rssi threshold
		*ptr++ = _list[i].rssiThreshold;
	}
}

/** Copy data from the given buffer into this object. */
void TrackedDeviceList::deserialize(uint8_t* buffer, uint16_t length) {
	// TODO -oDE: untested!!
	uint8_t* ptr;
	ptr = buffer;

	// copy number of elements
	_freeIdx = *ptr++;
	for (int i = 0; i < getSize(); ++i) {
		// copy address of device
		tracked_device_t* dev = &_list[i];
		for (int i= BLE_GAP_ADDR_LEN - 1; i >= 0; --i) {
			dev->addr[i] = *ptr++;
		}
		// copy rssi threshold
		dev->rssiThreshold = *ptr++;
	}
}

/**********************************************************************************************************************
 * A specific tracked device
 *********************************************************************************************************************/

/**
 * From a device we track
 *   RSSI level
 *   presence counter
 *   the Bluetooth address
 */

TrackedDevice::TrackedDevice() {
}

void TrackedDevice::init() {
}

bool TrackedDevice::operator!=(const TrackedDevice& val) {
	if (_trackedDevice.rssiThreshold != val._trackedDevice.rssiThreshold) {
		return true;
	}
	if (_trackedDevice.counter != val._trackedDevice.counter) {
		return true;
	}
	if (memcmp(this->_trackedDevice.addr, val._trackedDevice.addr, BLE_GAP_ADDR_LEN) != 0) {
		return true;
	}
	return false;
}

/** 
 * Return length of buffer required to store the serialized form of this object. If this method returns 0,
 * it means that the object does not need external buffer space. 
 */
uint32_t TrackedDevice::getSerializedLength() const {
	return TRACKDEVICES_SERIALIZED_SIZE;
}

void TrackedDevice::print() const {
	LOGi("[%02X %02X %02X %02X %02X %02X]\trssi: %d", _trackedDevice.addr[5],
		_trackedDevice.addr[4], _trackedDevice.addr[3], _trackedDevice.addr[2], _trackedDevice.addr[1],
		_trackedDevice.addr[0], _trackedDevice.rssiThreshold);
}

/** 
 * Copy data representing this object into the given buffer. Buffer will be preallocated with at least
 * getLength() bytes. 
 */
void TrackedDevice::serialize(uint8_t* buffer, uint16_t length) const {
	uint8_t *ptr;
	ptr = buffer;

	// copy address of device
	for (uint8_t j= 1; j <= BLE_GAP_ADDR_LEN; ++j) {
		*ptr++ = _trackedDevice.addr[BLE_GAP_ADDR_LEN - j];
	}
	// copy rssi threshold
	*ptr++ = _trackedDevice.rssiThreshold;
}

/** Copy data from the given buffer into this object. */
void TrackedDevice::deserialize(uint8_t* buffer, uint16_t length) {
	// TODO -oDE: untested!!
	uint8_t* ptr;
	ptr = buffer;

	// copy address of device
	for (int i= BLE_GAP_ADDR_LEN - 1; i >= 0; --i) {
		_trackedDevice.addr[i] = *ptr++;
	}
	// copy rssi threshold
	_trackedDevice.rssiThreshold = *ptr++;
}

