/*
 * Author: Crownstone Team
 * Copyright: Crownstone 
 * Date: Apr 28, 2016
 * License: LGPLv3+, Apache, MIT
 */
#pragma once

#include <drivers/cs_Storage.h>
#include <limits>

//#define PRINT_DEBUG_CYCLIC_STORAGE

/**
 * Cyclic storage is a wrapper around flash memory.
 *
 * The buffer stores individual elements together with a sequence number V to reduce wear on flash memory.
 */
template <class T, int U, class V>
class CyclicStorage {

public:
	//! Storage element, sequence number combined with a value.
	struct __attribute__((__packed__)) storage_element_t {
		V seqNumber;
		T value;
	};

	//! Construct the cyclic storage and load values from memory
	CyclicStorage(pstorage_handle_t handle, pstorage_size_t offset, T defaultValue) :
		_storage(NULL), _tail(-1), _seqNumber(0),
		_value(defaultValue), _defaultValue(defaultValue),
		_storageHandle(handle), _storageOffset(offset)
	{
		_storage = &Storage::getInstance();
		loadFromStorage();
	}

	//! Store individual value in memory
	void store(T value) {
		incTail();

		element.seqNumber = ++_seqNumber;
		element.value = value;

#ifdef PRINT_DEBUG_CYCLIC_STORAGE
		LOGd("element:");
		BLEutil::printArray(&element, sizeof(element));
#endif

		pstorage_size_t offset = _storageOffset + (_tail * sizeof(storage_element_t));

#ifdef PRINT_DEBUG_CYCLIC_STORAGE
		LOGd("offset: %d", offset);
#endif

		_storage->writeItem(_storageHandle, offset, (uint8_t*)&element, sizeof(storage_element_t));
		_value = value;
	}

	/** Print a value from memory.
	 *
	 * The print function displays the entire buffer representing the value in FLASH. It is not ordered in the sense
	 * that the most recent is displayed first.
	 */
	void print() const {
		storage_element_t buffer[U];
		_storage->readItem(_storageHandle, _storageOffset, (uint8_t*)buffer, U * sizeof(storage_element_t));

		LOGd("cyclic storage:");
		BLEutil::printArray(buffer, sizeof(buffer));
	}

	//! Read value locally.
	T read() const {
		return _value;
	}

	/** Load all values from memory.
	 *
	 * A value can be stored at different locations in FLASH. The size of the buffer is given by the template parameter
	 * U. The loadFromStorage() function iterates through the values and sets the one with the largest sequence number
	 * and the sequence number itself in fields of the class.
	 *
	 * The load function is automatically called from the constructor. After a store() it is not necessary to call
	 * loadFromStorage() before calling read().
	 *
	 * TODO: Minor issue. The wrap-around for sequence numbers near TYPE_MAX would return those numbers even if the
	 * sequence wrapped around to 0 again. It would be enough to assume an incremental counter. Say, if TYPE_MAX would
	 * be 16, then a buffer with U=10 can contain [14 15 0 1 2 3 10 11 12 13] and would need to return 3, not 15. If
	 * we calculate diff = ([(i-1)%U]+U-[i])%U it would be:
	 * [30-13 29-14 16-15 17-16 18-17 19-18 26-19 27-26 28-27 29-28]%U = [1 1 1 1 1 1 7 1 1 1]. The highest value is 3.
	 * Even if the sequence counter skips so now and then, picking the maximum would be pretty robust.
	 */
	void loadFromStorage() {
		storage_element_t buffer[U];

		_storage->readItem(_storageHandle, _storageOffset, (uint8_t*)buffer, U * sizeof(storage_element_t));

//		LOGd("tail: %d", _tail);
//		LOGd("_value: %d", _value);
//		LOGd("_seqNumber: %d", _seqNumber);

#ifdef PRINT_DEBUG_CYCLIC_STORAGE
		LOGd("init cyclic storage:");
		BLEutil::printArray(buffer, sizeof(buffer));
#endif

		for (int i = 0; i < U; ++i) {
			if ((buffer[i].seqNumber != std::numeric_limits<V>::max()) && (buffer[i].seqNumber >= _seqNumber)) {
				_seqNumber = buffer[i].seqNumber;
				_value = buffer[i].value;
				_tail = i;
			}
		}

#ifdef PRINT_DEBUG_CYCLIC_STORAGE
		LOGd("tail: %d", _tail);
		LOGd("_value: %d", _value);
		LOGd("_seqNumber: %d", _seqNumber);
#endif

	}

	//! Reset value and corresponding counter.
	void reset() {
		_value = _defaultValue;
		_seqNumber = 0;
		// leave the tail where it is, otherwise we lose some information about the wear leveling
		// _tail = -1;
	}

private:

	Storage* _storage;

	uint16_t _tail;
	V _seqNumber;
	T _value;
	T _defaultValue;

	pstorage_handle_t _storageHandle;
	pstorage_size_t _storageOffset;

	//! needs to be word aligned!
	storage_element_t element __attribute__ ((aligned (4)));

	void incTail() {
		++_tail;
		_tail %= U;
	}


};

