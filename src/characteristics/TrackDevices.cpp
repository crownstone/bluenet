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

TrackDevices::TrackDevices() : _list(NULL), _freeIdx(0) {
}

void TrackDevices::init() {
	_freeIdx = 0;
	if (_list) {
		free(_list);
	}
	_list = (tracked_device_t*)calloc(TRACKDEVICES_MAX_NR_DEVICES, sizeof(tracked_device_t));
}

// returns the number of elements stored so far
uint16_t TrackDevices::getSize() const {
	// freeIdx points to the next free index of the array,
	// but because the first element is at index 0,
	// freeIdx is also the number of elements in the array so far
	return _freeIdx;
}

void TrackDevices::reset() {
	memset(_list, 0, TRACKDEVICES_MAX_NR_DEVICES * sizeof(tracked_device_t));
	_freeIdx = 0;
}

bool TrackDevices::operator!=(const TrackDevices& val) {

	if (_freeIdx != val._freeIdx) {
		return true;
	}

	for (int i = 0; i < getSize(); ++i) {
		if (_list[i].rssi_threshold != val._list[i].rssi_threshold) {
			return true;
		}
		if (_list[i].state != val._list[i].state) {
			return true;
		}
		if (memcmp(this->_list[i].addr, val._list[i].addr, BLE_GAP_ADDR_LEN) != 0) {
			return true;
		}
	}

	return false;
}

void TrackDevices::update(uint8_t * adrs_ptr, int8_t rssi) {
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
				if (_list[i].state) {
					LOGd("Device is no longer present: switch off light or so");
				}
			}
			else {
				if (!_list[i].state) {
					LOGd("Device is now present: switch on light or so");
				}
			}
			break;
		}
	}

}

void TrackDevices::print() const {

	for (int i = 0; i < getSize(); ++i) {
//		char addrs[28];
//		sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", _list[i].addr[5],
//				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
//				_list[i].addr[0]);
		LOGi("[%02X %02X %02X %02X %02X %02X]\trssi: %d\tstate: %d", _list[i].addr[5],
				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
				_list[i].addr[0], _list[i].rssi_threshold, _list[i].state);
	}

}

/** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
* it means that the object does not need external buffer space. */
uint32_t TrackDevices::getSerializedLength() const {
	return getSize() * TRACKDEVICES_SERIALIZED_SIZE + TRACKDEVICES_HEADER_SIZE;
}

/** Copy data representing this object into the given buffer.  Buffer will be preallocated with at least
* getLength() bytes. */
void TrackDevices::serialize(uint8_t* buffer, uint16_t length) const {
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

		// copy state
		*ptr++ = _list[i].state;
	}
}

/** Copy data from the given buffer into this object. */
void TrackDevices::deserialize(uint8_t* buffer, uint16_t length) {
	// TODO -oDE: untested!!
	uint8_t* ptr;
	ptr = buffer;

	_freeIdx = *ptr++;

	for (int i = 0; i < getSize(); ++i) {
		// copy address of device
		tracked_device_t* dev = &_list[i];
		for (int i= BLE_GAP_ADDR_LEN - 1; i >= 0; --i) {
			dev->addr[i] = *ptr++;
		}

		// copy rssi
		dev->rssi_threshold = *ptr++;

		// copy state
		dev->state = *ptr++;
	}
}
