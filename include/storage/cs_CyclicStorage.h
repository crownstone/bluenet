/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 28, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <drivers/cs_Storage.h>
#include <limits>

// Define for debug prints
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

		// Value can simply be written if the value in flash is currently all 1s.
		storage_element_t curElement;
		_storage->readItem(_storageHandle, offset, (uint8_t*)(&curElement), sizeof(storage_element_t));
//		// TODO: only set update = true when _all_ bytes are FF
//		// TODO: use something like: T val = 0; if (~val ^ curElement == 0) { curElement is all ones }
//		bool update = false;
//		uint8_t ones = 0xFF;
//		for (size16_t i=0; i<sizeof(storage_element_t); ++i) {
//			if (memcmp(&curElement, &ones, 1) != 0) {
//				update = true;
//				break;
//			}
//		}
		// TODO: sure this works for any type of int?
		T temp = 0;
		T ones = ~temp;
		bool update = ((ones ^ curElement.value) != 0);

#ifdef PRINT_DEBUG_CYCLIC_STORAGE
		LOGd("offset: %d", offset);
		LOGd("current val: %u %u == %u %u", curElement.seqNumber, curElement.value, _buffer[_tail].seqNumber, _buffer[_tail].value);
		LOGd("update=%u", update);
#endif

		if (update && _tail == 0) {
			LOGi("clear cyclic storage");
			memset(&_buffer, 0xFF, sizeof(_buffer));
			_buffer[_tail] = element;
			_storage->writeItem(_storageHandle, _storageOffset, (uint8_t*)&_buffer, sizeof(_buffer), true);
		}
		else {
			_storage->writeItem(_storageHandle, offset, (uint8_t*)&element, sizeof(storage_element_t), update);
		}
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
//		storage_element_t _buffer[U];

		_storage->readItem(_storageHandle, _storageOffset, (uint8_t*)_buffer, U * sizeof(storage_element_t));

//		LOGd("tail: %d", _tail);
//		LOGd("_value: %d", _value);
//		LOGd("_seqNumber: %d", _seqNumber);

#ifdef PRINT_DEBUG_CYCLIC_STORAGE
		LOGd("init cyclic storage:");
		BLEutil::printArray(_buffer, sizeof(_buffer));
#endif

		for (int i = 0; i < U; ++i) {
			if ((_buffer[i].seqNumber != std::numeric_limits<V>::max()) && (_buffer[i].seqNumber >= _seqNumber)) {
				_seqNumber = _buffer[i].seqNumber;
				_value = _buffer[i].value;
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

	// Store the whole buffer, as in flash.
	storage_element_t _buffer[U];

	void incTail() {
		++_tail;
		_tail %= U;
	}


};

