/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+
 */

#include "string.h"
#include "stdio.h"

#include "ScanResult.h"
#include "log.h"
#include "utils.h"

ScanResult::ScanResult() : _list(NULL), _size(0), _freeIdx(0) {
}

ScanResult::~ScanResult() {
	LOG_INFO("~ScanResult");
	if (_list) {
		free(_list);
	}
}

bool ScanResult::operator!=(const ScanResult& val) {
	return true;
}

void ScanResult::init(size_t size) {
	LOG_INFO("ScanResult.init()");

	if (_list) {
		free(_list);
	}

	_list = (peripheral_device_t*)malloc(size * sizeof(peripheral_device_t));
	memset(_list, 0, size * sizeof(peripheral_device_t));

	_freeIdx = 0;
	_size = size;
}

size_t ScanResult::getSize() const {
	return _size;
}

peripheral_device_t* ScanResult::getList() const {
	return _list;
}

void ScanResult::update(uint8_t * adrs_ptr, int8_t rssi) {
	LOG_INFO("ScanResult.update()");

	char addrs[28];
	sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", adrs_ptr[5],
			adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
			adrs_ptr[0]);

//	LOG_INFO("Advertisement from: %s, rssi: %d", addrs, rssi);

	uint16_t occ;
	bool found = false;
	LOG_INFO("addrs: %s", addrs);
	for (int i = 0; i < _freeIdx; ++i) {
		LOG_INFO("_history[%d]: %s", i, _list[i].addrs);
		if (strcmp(addrs, _list[i].addrs) == 0) {
			LOG_INFO("found");
			occ = ++_list[i].occurences;
			//					occ = _history[i].occurences;
			//					int avg_rssi = ((occ-1)*_history[i].rssi + p_adv_report->rssi)/occ;
			//					_history[i].rssi = avg_rssi;
			_list[i].rssi = rssi;
			found = true;
		}
	}
	if (!found) {
		uint8_t idx;
		if (_freeIdx >= _size) {
			// history full, throw out item with lowest occurence
			uint16_t minOcc = UINT16_MAX;
			for (int i = 0; i < _size; ++i) {
				if (_list[i].occurences < minOcc) {
					minOcc = _list[i].occurences;
					idx = i;
				}
			}
		} else {
			idx = _freeIdx++;
			//					peripheral_device_t peripheral;
			//					_history[idx] = peripheral;
		}

		LOG_INFO("NEW:\tAdvertisement from: %s, rssi: %d", addrs, rssi);
		memcpy(_list[idx].addr, adrs_ptr, BLE_GAP_ADDR_LEN);
		strcpy(_list[idx].addrs, addrs);
		_list[idx].occurences = 1;
		_list[idx].rssi = rssi;
	} else {
		LOG_INFO("\tAdvertisement from: %s, rssi: %d, occ: %d", addrs, rssi, occ);
	}
}

void ScanResult::print() const {

	LOG_INFO("##################################################");
	LOG_INFO("### listing detected peripherals #################");
	LOG_INFO("##################################################");
	for (int i = 0; i < _freeIdx; ++i) {
		LOG_INFO("%s\trssi: %d\tocc: %d", _list[i].addrs, _list[i].rssi, _list[i].occurences);
	}
	LOG_INFO("##################################################");

}

/** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
* it means that the object does not need external buffer space. */
uint32_t ScanResult::getSerializedLength() const {
	return _freeIdx * SERIALIZED_DEVICE_SIZE + 1; // + 1 byte for header size (number of elements in array)
}

/** Copy data representing this object into the given buffer.  Buffer will be preallocated with at least
* getLength() bytes. */
void ScanResult::serialize(Buffer& buffer) const {
	uint8_t *ptr;
	ptr = buffer.data;

	// copy number of elements
//	*ptr++ = _size;
	*ptr++ = _freeIdx;
	for (uint16_t i = 0; i < _freeIdx; ++i) {
		// copy address of device
		peripheral_device_t* dev = &_list[i];

//		memcpy(ptr, dev->addr, BLE_GAP_ADDR_LEN);
//		ptr += BLE_GAP_ADDR_LEN;
		for (int i= BLE_GAP_ADDR_LEN - 1; i >= 0; --i) {
			*ptr++ = dev->addr[i];
		}

		// copy rssi
		*ptr = dev->rssi;
		ptr++;

		// copy occurences
		// doesn't work because ptr is not aligned
		//		*((uint16_t*)ptr) = (uint16_t)dev->occurences;
		*ptr++ = (dev->occurences >> 8) & 0xFF;
		*ptr++ = dev->occurences & 0xFF;

	}
//
//    _LOG_INFO("serialize value, len:%d, : ", buffer.length);
//    for (int i = 0; i < buffer.length; ++i) {
//    	_LOG_INFO("%.2X ", buffer.data[i]);
//    }
//    LOG_INFO("");
}

/** Copy data from the given buffer into this object. */
void ScanResult::deserialize(Buffer& buffer) {
	LOG_INFO("ScanResult.deserialize()");

	uint8_t* ptr;
	ptr = buffer.data;
	_size = *ptr++;
	_freeIdx = _size;

	if (_list != NULL) {
		free(_list);
	}
	_list = (peripheral_device_t*)malloc(_size * sizeof(peripheral_device_t));

	for (uint16_t i = 0; i < _size; ++i) {
		// copy address of device
		peripheral_device_t* dev = &_list[i];
//		memcpy(dev->addr, ptr, BLE_GAP_ADDR_LEN);
//		ptr += BLE_GAP_ADDR_LEN;
		for (int i= BLE_GAP_ADDR_LEN - 1; i >= 0; --i) {
			dev->addr[i] = *ptr++;
		}
		sprintf(dev->addrs, "[%02X %02X %02X %02X %02X %02X]",
				dev->addr[5], dev->addr[4], dev->addr[3], dev->addr[2],
				dev->addr[1], dev->addr[0]);

		// copy rssi
		dev->rssi = *ptr++;

		// copy occurences
		// doesn't work because ptr is not aligned
		//		dev->occurences = *((uint16_t*) ptr);
		dev->occurences |= (*ptr++ << 8) & 0xFF;
		dev->occurences = *ptr++;

	}
	LOG_INFO("32");
}
