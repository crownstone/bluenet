/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "string.h"
#include "stdio.h"

#include "characteristics/ScanResult.h"
#include "drivers/serial.h"
#include "util/utils.h"

ScanResult::ScanResult() : _freeIdx(0) {
}

ScanResult::~ScanResult() {
}

// returns the number of elements stored so far
uint16_t ScanResult::getSize() const {
	// freeIdx points to the next free index of the array,
	// but because the first element is at index 0,
	// freeIdx is also the number of elements in the array so far
	return _freeIdx;
}

void ScanResult::reset() {
	memset(_list, 0, sizeof(_list));
	_freeIdx = 0;
}

bool ScanResult::operator!=(const ScanResult& val) {

	if (this->_freeIdx != val._freeIdx) {
		return true;
	}

	for (int i = 0; i < this->_freeIdx; ++i) {
		if (this->_list[i].occurences != val._list[i].occurences) {
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
//	log(DEBUG, "count: %d", count++);
	char addrs[28];
	sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", adrs_ptr[5],
			adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
			adrs_ptr[0]);

	bool found = false;
//	log(DEBUG, "addrs: %s", addrs);
	for (int i = 0; i < _freeIdx; ++i) {
//		log(DEBUG, "_history[%d]: [%02X %02X %02X %02X %02X %02X]", i, _list[i].addr[5],
//				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
//				_list[i].addr[0]);
		for (int j=0; j < BLE_GAP_ADDR_LEN; ++j) {
			if (_list[i].addr[j] == 0) {
				log(FATAL, "FATAL ERROR!!! i: %d, j:% d", i, j);
				print();
				__asm("BKPT");
				while(1);
			}
		}
		if (memcmp(adrs_ptr, _list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
//			log(DEBUG, "found");
			_list[i].occurences++;
			_list[i].rssi = rssi;
			found = true;
		}
	}
	if (!found) {
		uint8_t idx  = -1;
		if (_freeIdx >= MAX_NR_DEVICES) {
			// history full, throw out item with lowest occurence
			uint16_t minOcc = UINT16_MAX;
			for (int i = 0; i < MAX_NR_DEVICES; ++i) {
				if (_list[i].occurences < minOcc) {
					minOcc = _list[i].occurences;
					idx = i;
				}
			}
//			log(DEBUG, "replacing item at idx: %d", idx);
		} else {
			idx = _freeIdx++;
		}

		log(INFO, "NEW Advertisement from: %s, rssi: %d", addrs, rssi);
		memcpy(_list[idx].addr, adrs_ptr, BLE_GAP_ADDR_LEN);
		_list[idx].occurences = 1;
		_list[idx].rssi = rssi;
	} else {
//		log(DEBUG, "Advertisement from: %s, rssi: %d, occ: %d", addrs, rssi, occ);
	}
}

void ScanResult::print() const {

	log(INFO, "##################################################");
	log(INFO, "### listing detected peripherals (%2d) ############", getSize());
	log(INFO, "##################################################");
	for (int i = 0; i < getSize(); ++i) {
		char addrs[28];
		sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", _list[i].addr[5],
				_list[i].addr[4], _list[i].addr[3], _list[i].addr[2], _list[i].addr[1],
				_list[i].addr[0]);
		log(INFO, "%s\trssi: %d\tocc: %d", addrs, _list[i].rssi, _list[i].occurences);
	}
	log(INFO, "##################################################");

}

/** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
* it means that the object does not need external buffer space. */
uint32_t ScanResult::getSerializedLength() const {
	return getSize() * SERIALIZED_DEVICE_SIZE + HEADER_SIZE;
}

/** Copy data representing this object into the given buffer.  Buffer will be preallocated with at least
* getLength() bytes. */
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

		// copy occurences
		// doesn't work because ptr is not aligned
		*ptr++ = (_list[i].occurences >> 8) & 0xFF;
		*ptr++ = _list[i].occurences & 0xFF;
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

		// copy occurences
		// doesn't work because ptr is not aligned
		dev->occurences |= (*ptr++ << 8) & 0xFF;
		dev->occurences = *ptr++;
	}
}
