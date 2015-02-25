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

ScanResult::ScanResult() : _list(NULL), _freeIdx(0) {
}

void ScanResult::init() {
	_freeIdx = 0;
	if (_list) {
		free(_list);
	}
	_list = (peripheral_device_t*)calloc(MAX_NR_DEVICES, sizeof(peripheral_device_t));
}

// returns the number of elements stored so far
uint16_t ScanResult::getSize() const {
	// freeIdx points to the next free index of the array,
	// but because the first element is at index 0,
	// freeIdx is also the number of elements in the array so far
	return _freeIdx;
}

void ScanResult::reset() {
	memset(_list, 0, MAX_NR_DEVICES * sizeof(peripheral_device_t));
	_freeIdx = 0;
}

bool ScanResult::operator!=(const ScanResult& val) {

	if (this->_freeIdx != val._freeIdx) {
		return true;
	}

	for (int i = 0; i < this->_freeIdx; ++i) {
		if (this->_list[i].occurrences != val._list[i].occurrences) {
			return true;
		}
		if (this->_list[i].rssi != val._list[i].rssi) {
			return true;
		}
		if (memcmp(this->_list[i].addr, val._list[i].addr, BLE_GAP_ADDR_LEN) != 0) {
			return true;
		}
	}

	return false;
}

//int count = 1;
void ScanResult::update(uint8_t * adrs_ptr, int8_t rssi) {
//	LOGd("count: %d", count++);
//	char addrs[28];
//	sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", adrs_ptr[5],
//			adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
//			adrs_ptr[0]);

	bool found = false;
//	LOGd("addrs: %s", addrs);
	for (int i = 0; i < _freeIdx; ++i) {
//		LOGd("_history[%d]: [%02X %02X %02X %02X %02X %02X]", i, _list[i].addr[5],
//				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
//				_list[i].addr[0]);

		if (memcmp(adrs_ptr, _list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
//			LOGd("found");
			_list[i].occurrences++;
			_list[i].rssi = rssi;
			found = true;
			// TODO: Any reason not to break here?
		}
	}
	if (!found) {
		uint8_t idx  = -1;
		if (_freeIdx >= MAX_NR_DEVICES) {
			// history full, throw out item with lowest occurence
			uint16_t minOcc = UINT16_MAX;
			for (int i = 0; i < MAX_NR_DEVICES; ++i) {
				if (_list[i].occurrences < minOcc) {
					minOcc = _list[i].occurrences;
					idx = i;
				}
			}
//			LOGd("replacing item at idx: %d", idx);
		} else {
			idx = _freeIdx++;
		}

		LOGi("NEW Advertisement from: [%02X %02X %02X %02X %02X %02X], rssi: %d", adrs_ptr[5],
				adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
				adrs_ptr[0], rssi);
		memcpy(_list[idx].addr, adrs_ptr, BLE_GAP_ADDR_LEN);
		_list[idx].occurrences = 1;
		_list[idx].rssi = rssi;
	} else {
//		LOGd("Advertisement from: %s, rssi: %d, occ: %d", addrs, rssi, occ);
	}
}

void ScanResult::print() const {

	for (int i = 0; i < getSize(); ++i) {
//		char addrs[28];
//		sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", _list[i].addr[5],
//				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
//				_list[i].addr[0]);
		LOGi("[%02X %02X %02X %02X %02X %02X]\trssi: %d\tocc: %d", _list[i].addr[5],
				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
				_list[i].addr[0], _list[i].rssi, _list[i].occurrences);
	}

}

/** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
* it means that the object does not need external buffer space. */
uint16_t ScanResult::getSerializedLength() const {
	return getSize() * SERIALIZED_DEVICE_SIZE + HEADER_SIZE;
}

/** Copy data representing this object into the given buffer. Buffer has to be preallocated with at least
* getSerializedLength() bytes. */
void ScanResult::serialize(uint8_t* buffer, uint16_t length) const {
	uint8_t *ptr;
	ptr = buffer;

	// copy number of elements
	*ptr++ = getSize();
	for (uint16_t i = 0; i < _freeIdx; ++i) {
		// copy address of device
		for (uint8_t j= 1; j <= BLE_GAP_ADDR_LEN; ++j) {
			*ptr++ = _list[i].addr[BLE_GAP_ADDR_LEN - j];
		}

		// copy rssi
		*ptr++ = _list[i].rssi;

		// copy occurrences
		// doesn't work because ptr is not aligned
		*ptr++ = (_list[i].occurrences >> 8) & 0xFF;
		*ptr++ = _list[i].occurrences & 0xFF;
	}
}

/** Copy data from the given buffer into this object. */
void ScanResult::deserialize(uint8_t* buffer, uint16_t length) {
	// TODO -oDE: untested!!
	uint8_t* ptr;
	ptr = buffer;

	_freeIdx = *ptr++;

	for (uint16_t i = 0; i < _freeIdx; ++i) {
		// copy address of device
		peripheral_device_t* dev = &_list[i];
		for (int i= BLE_GAP_ADDR_LEN - 1; i >= 0; --i) {
			dev->addr[i] = *ptr++;
		}

		// copy rssi
		dev->rssi = *ptr++;

		// copy occurrences
		// doesn't work because ptr is not aligned
		dev->occurrences |= (*ptr++ << 8) & 0xFF;
		dev->occurrences = *ptr++;
	}
}
