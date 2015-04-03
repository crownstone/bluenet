/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <string.h>
#include <stdio.h>

#include "characteristics/cs_ScanResult.h"
#include "drivers/cs_Serial.h"

ScanResult::ScanResult() : _buffer(NULL) {
}

//void ScanResult::init() {
//	_freeIdx = 0;
//	if (_buffer) {
//		free(_buffer);
//	}
//	LOGi("LIST alloc");
//	BLEutil::print_heap("list alloc:");
////	_buffer = (peripheral_device_t*)calloc(SR_MAX_NR_DEVICES, sizeof(peripheral_device_t));
//	BLEutil::print_heap("list alloc done:");
//}

//void ScanResult::release() {
//	if (_buffer) {
//		LOGi("LIST free");
//		free(_buffer);
//	}
//	_buffer = NULL;
//}

// returns the number of elements stored so far
uint16_t ScanResult::getSize() const {
	// freeIdx points to the next free index of the array,
	// but because the first element is at index 0,
	// freeIdx is also the number of elements in the array so far
	if (_buffer != NULL) {
		return _buffer->size;
	} else {
		return 0;
	}
}

void ScanResult::reset() {
	memset(_buffer, 0, sizeof(peripheral_device_list_t));
}

bool ScanResult::operator!=(const ScanResult& val) {

	if (_buffer == NULL) {
		LOGe("scan result buffer was not assigned!!!");
		return false;
	}

	if (this->getSize() != val.getSize()) {
		return true;
	}

	for (int i = 0; i < this->getSize(); ++i) {
		if (this->_buffer->list[i].occurrences != val._buffer->list[i].occurrences) {
			return true;
		}
		if (this->_buffer->list[i].rssi != val._buffer->list[i].rssi) {
			return true;
		}
		if (memcmp(this->_buffer->list[i].addr, val._buffer->list[i].addr, BLE_GAP_ADDR_LEN) != 0) {
			return true;
		}
	}

	return false;
}

//int count = 1;
void ScanResult::update(uint8_t * adrs_ptr, int8_t rssi) {

	if (_buffer == NULL) {
		LOGe("scan result buffer was not assigned!!!");
		return;
	}

//	LOGd("count: %d", count++);
//	char addrs[28];
//	sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", adrs_ptr[5],
//			adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
//			adrs_ptr[0]);

	bool found = false;
//	LOGd("addrs: %s", addrs);
	for (int i = 0; i < getSize(); ++i) {
//		LOGd("_history[%d]: [%02X %02X %02X %02X %02X %02X]", i, _buffer->list[i].addr[5],
//				_buffer->list[i].addr[4], _buffer->list[i].addr[3], _buffer->list[i].addr[2], _buffer->list[i].addr[1],
//				_buffer->list[i].addr[0]);

		if (memcmp(adrs_ptr, _buffer->list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
//			LOGd("found");
			_buffer->list[i].occurrences++;
			_buffer->list[i].rssi = rssi;
			found = true;
			// TODO: Any reason not to break here?
		}
	}
	if (!found) {
		uint8_t idx  = -1;
		if (getSize() >= SR_MAX_NR_DEVICES) {
			// history full, throw out item with lowest occurence
			uint16_t minOcc = UINT16_MAX;
			for (int i = 0; i < SR_MAX_NR_DEVICES; ++i) {
				if (_buffer->list[i].occurrences < minOcc) {
					minOcc = _buffer->list[i].occurrences;
					idx = i;
				}
			}
//			LOGd("replacing item at idx: %d", idx);
		} else {
			idx = _buffer->size++;
		}

		LOGi("NEW Advertisement from: [%02X %02X %02X %02X %02X %02X], rssi: %d", adrs_ptr[5],
				adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
				adrs_ptr[0], rssi);
		memcpy(_buffer->list[idx].addr, adrs_ptr, BLE_GAP_ADDR_LEN);
		_buffer->list[idx].occurrences = 1;
		_buffer->list[idx].rssi = rssi;
	} else {
//		LOGd("Advertisement from: %s, rssi: %d, occ: %d", addrs, rssi, occ);
	}

//	BLEutil::printArray<uint8_t>((uint8_t*)_buffer, SR_MAX_NR_DEVICES * SR_SERIALIZED_DEVICE_SIZE);
}

void ScanResult::print() const {
	LOGd("print: %p", this);

	for (int i = 0; i < getSize(); ++i) {
//		char addrs[28];
//		sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", _buffer->list[i].addr[5],
//				_buffer->list[i].addr[4], _buffer->list[i].addr[3], _buffer->list[i].addr[2], _buffer->list[i].addr[1],
//				_buffer->list[i].addr[0]);
		LOGi("[%02X %02X %02X %02X %02X %02X]\trssi: %d\tocc: %d", _buffer->list[i].addr[5],
				_buffer->list[i].addr[4], _buffer->list[i].addr[3], _buffer->list[i].addr[2], _buffer->list[i].addr[1],
				_buffer->list[i].addr[0], _buffer->list[i].rssi, _buffer->list[i].occurrences);
	}

}

/** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
* it means that the object does not need external buffer space. */
