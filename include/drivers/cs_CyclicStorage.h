/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 28, 2016
 * License: LGPLv3+
 */
#pragma once

#include <drivers/cs_Storage.h>
#include <pstorage_platform.h>
#include <stddef.h>
#include <structs/buffer/cs_CircularBuffer.h>
#include <cstdint>

//#define PRINT_DEBUG

template <class T, int U>
class CyclicStorage {

public:

	struct __attribute__((__packed__)) storage_element_t {
		uint32_t seqNumber;
		T value;
	};

	CyclicStorage(pstorage_handle_t handle, pstorage_size_t offset, T defaultValue) :
		_storage(NULL), _tail(-1), _seqNumber(0),
		_value(defaultValue), _storageHandle(handle), _storageOffset(offset)
	{
		_storage = &Storage::getInstance();
		loadFromStorage();
	}

	void store(T value) {
		incTail();

		element.seqNumber = ++_seqNumber;
		element.value = value;

#ifdef PRINT_DEBUG
		LOGd("element:");
		BLEutil::printArray(&element, sizeof(element));
#endif

		pstorage_size_t offset = _storageOffset + (_tail * sizeof(storage_element_t));

#ifdef PRINT_DEBUG
		LOGd("offset: %d", offset);
#endif

		_storage->writeItem(_storageHandle, offset, (uint8_t*)&element, sizeof(storage_element_t));
		_value = value;
	}

	void print() {
		storage_element_t buffer[U];
		_storage->readItem(_storageHandle, _storageOffset, (uint8_t*)buffer, U * sizeof(storage_element_t));

		LOGd("cyclic storage:");
		BLEutil::printArray(buffer, sizeof(buffer));
	}

	T read() {
		return _value;
	}

	void loadFromStorage() {
		storage_element_t buffer[U];

		_storage->readItem(_storageHandle, _storageOffset, (uint8_t*)buffer, U * sizeof(storage_element_t));

//		LOGd("tail: %d", _tail);
//		LOGd("_value: %d", _value);
//		LOGd("_seqNumber: %d", _seqNumber);

#ifdef PRINT_DEBUG
		LOGd("init cyclic storage:");
		BLEutil::printArray(buffer, sizeof(buffer));
#endif

		for (int i = 0; i < U; ++i) {
			if ((buffer[i].seqNumber != UINT32_MAX) && (buffer[i].seqNumber >= _seqNumber)) {
				_seqNumber = buffer[i].seqNumber;
				_value = buffer[i].value;
				_tail = i;
			}
		}

#ifdef PRINT_DEBUG
		LOGd("tail: %d", _tail);
		LOGd("_value: %d", _value);
		LOGd("_seqNumber: %d", _seqNumber);
#endif

	}

private:

	Storage* _storage;

	uint16_t _tail;
	uint32_t _seqNumber;
	T _value;

	pstorage_handle_t _storageHandle;
	pstorage_size_t _storageOffset;

	//! needs to be word aligned!
	storage_element_t element __attribute__ ((aligned (4)));

	void incTail() {
		++_tail;
		_tail %= U;
	}


};

