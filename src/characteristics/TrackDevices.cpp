/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 24, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "string.h"
#include "stdio.h"

#include "characteristics/TrackDevices.h"
#include "drivers/serial.h"
#include "util/utils.h"

TrackedDeviceList::TrackedDeviceList() : _list(NULL), _freeIdx(0) {
}

void TrackedDeviceList::init() {
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

void TrackedDeviceList::reset() {
	memset(_list, 0, TRACKDEVICES_MAX_NR_DEVICES * sizeof(tracked_device_t));
	_freeIdx = 0;
}

bool TrackedDeviceList::operator!=(const TrackedDeviceList& val) {

	if (_freeIdx != val._freeIdx) {
		return true;
	}

	for (int i = 0; i < getSize(); ++i) {
		if (_list[i].rssi_threshold != val._list[i].rssi_threshold) {
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

void TrackedDeviceList::update(uint8_t * adrs_ptr, int8_t rssi) {
//	LOGd("addrs: %s", addrs);
	for (int i = 0; i < getSize(); ++i) {
//		LOGd("_history[%d]: [%02X %02X %02X %02X %02X %02X]", i, _list[i].addr[5],
//				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
//				_list[i].addr[0]);
		for (int j=0; j < BLE_GAP_ADDR_LEN; ++j) {
			if (_list[i].addr[j] == 0) {
//				log(FATAL, "FATAL ERROR!!! i: %d, j:% d", i, j);
				print();
				__asm("BKPT");
				while(1);
			}
		}
		if (memcmp(adrs_ptr, _list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
			if (rssi < _list[i].rssi_threshold) {
				_list[i].counter = 0;
				LOGd("Tracked device present");
			}
			break;
		}
	}

}

void TrackedDeviceList::print() const {

	for (int i = 0; i < getSize(); ++i) {
		LOGi("[%02X %02X %02X %02X %02X %02X]\trssi: %d\tstate: %d", _list[i].addr[5],
				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
				_list[i].addr[0], _list[i].rssi_threshold, _list[i].counter);
	}

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
		*ptr++ = _list[i].rssi_threshold;
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
		dev->rssi_threshold = *ptr++;
	}
}









TrackedDevice::TrackedDevice() {
}

void TrackedDevice::init() {
}

bool TrackedDevice::operator!=(const TrackedDevice& val) {
	if (_trackedDevice.rssi_threshold != val._trackedDevice.rssi_threshold) {
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

/** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
* it means that the object does not need external buffer space. */
uint32_t TrackedDevice::getSerializedLength() const {
	return TRACKDEVICES_SERIALIZED_SIZE;
}

/** Copy data representing this object into the given buffer.  Buffer will be preallocated with at least
* getLength() bytes. */
void TrackedDevice::serialize(uint8_t* buffer, uint16_t length) const {
	uint8_t *ptr;
	ptr = buffer;

	// copy address of device
	for (uint8_t j= 1; j <= BLE_GAP_ADDR_LEN; ++j) {
		*ptr++ = _trackedDevice.addr[BLE_GAP_ADDR_LEN - j];
	}
	// copy rssi threshold
	*ptr++ = _trackedDevice.rssi_threshold;
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
	_trackedDevice.rssi_threshold = *ptr++;
}








