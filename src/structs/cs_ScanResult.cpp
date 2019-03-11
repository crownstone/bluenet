/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <structs/cs_ScanResult.h>

//#define PRINT_SCANRESULT_VERBOSE
//#define PRINT_DEBUG

//! returns the number of elements stored so far
uint8_t ScanResult::getSize() const {
	if (_buffer != NULL) {
		return _buffer->size;
	} else {
		return 0;
	}
}

void ScanResult::clear() {
	memset(_buffer, 0, sizeof(peripheral_device_list_t));
}

void ScanResult::update(uint8_t * adrs_ptr, int8_t rssi) {

	if (_buffer == NULL) {
		LOGe(FMT_BUFFER_NOT_ASSIGNED, "Scan result");
		return;
	}

#ifdef PRINT_DEBUG
	LOGd("update: [%02X %02X %02X %02X %02X %02X], rssi: %d", adrs_ptr[5],
					adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
					adrs_ptr[0], rssi);
#endif

	for (int i = 0; i < getSize(); ++i) {

		if (memcmp(adrs_ptr, _buffer->list[i].addr, BLE_GAP_ADDR_LEN) == 0) {
			uint16_t occ = _buffer->list[i].occurrences;
			int8_t oldRssi = _buffer->list[i].rssi ;
			_buffer->list[i].rssi = (oldRssi * occ + rssi) / (occ + 1);
			_buffer->list[i].occurrences++;

#ifdef PRINT_DEBUG
			LOGd("old Advertisement from: [%02X %02X %02X %02X %02X %02X], rssi: %d", adrs_ptr[5],
				adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
				adrs_ptr[0], rssi);
#endif

			// TODO: Any reason not to break here?
			return;
		}
	}

	int8_t minRssi = INT8_MAX;
	int8_t idx  = -1;
	if (getSize() >= SR_MAX_NR_DEVICES) {
		// history full, throw out item with lowest rssi
		for (int i = 0; i < SR_MAX_NR_DEVICES; ++i) {
			if (_buffer->list[i].rssi < minRssi && _buffer->list[i].rssi < rssi) {
				minRssi = _buffer->list[i].rssi;
				idx = i;
			}
		}

#ifdef PRINT_DEBUG
		LOGd("replacing item at idx: %d", idx);
#endif
	}
	else {
		idx = _buffer->size++;
	}

	// If a spot has been found, write it to that position
	if (idx >= 0) {

#ifdef PRINT_SCANRESULT_VERBOSE
		LOGd("NEW Advertisement from: [%02X %02X %02X %02X %02X %02X], rssi: %d", adrs_ptr[5],
				adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
				adrs_ptr[0], rssi);
#endif

#ifdef PRINT_DEBUG
		LOGd("idx: %d, minRssi: %d", idx, minRssi);
#endif

		memcpy(_buffer->list[idx].addr, adrs_ptr, BLE_GAP_ADDR_LEN);
		_buffer->list[idx].occurrences = 1;
		_buffer->list[idx].rssi = rssi;
	}

//	BLEutil::printArray<uint8_t>((uint8_t*)_buffer, SR_MAX_NR_DEVICES * SR_SERIALIZED_DEVICE_SIZE);
}

void ScanResult::print() const {
	for (int i = 0; i < getSize(); ++i) {
		LOGd("[%02X %02X %02X %02X %02X %02X]\trssi: %d\tocc: %d", _buffer->list[i].addr[5],
				_buffer->list[i].addr[4], _buffer->list[i].addr[3], _buffer->list[i].addr[2], _buffer->list[i].addr[1],
				_buffer->list[i].addr[0]);
		LOGd("rssi: %d\tocc: %d",
				_buffer->list[i].rssi, _buffer->list[i].occurrences);
	}

}
