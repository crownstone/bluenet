/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 25, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <logging/cs_Logger.h>

#include <cstdlib>

#include "common/cs_Types.h"
#include "util/cs_BleError.h"

/** Struct with dynamic length, used by StackBuffer class.
 */
template <typename T>
struct __attribute__((__packed__)) differential_buffer_t {
	/** Number of elements stored in this struct */
	uint16_t length;

	/** Value of the oldest element in the buffer */
	T firstVal;

	/** Value of the newest element in the buffer */
	T lastVal;

	/** Pointer to the array storing the difference of elements compared to the previous element */
	int8_t array[1];  // Dummy length
};

/** Struct with fixed length, useful when sending as payload.
 *
 */
template <typename T, uint16_t S>
struct __attribute__((__packed__)) differential_buffer_fixed_t {
	/** Number of elements stored in this struct */
	uint16_t length;

	/** Value of the oldest element in the buffer */
	T firstVal;

	/** Value of the newest element in the buffer */
	T lastVal;

	/** Pointer to the array storing the difference of elements compared to the previous element */
	int8_t array[S - 1];
};

/** Differential Buffer implementation
 * @param T primitive type such as uint8_t
 *
 * Elements are added at the back and removed from the front.
 *
 */
template <typename T>
class DifferentialBuffer {
public:
	DifferentialBuffer(uint16_t capacity) : _buffer(NULL), _capacity(capacity), _allocatedSelf(false) {}

	virtual ~DifferentialBuffer() {}

	uint16_t getMaxByteSize(uint16_t capacity) { return 2 + 2 * sizeof(T) + capacity - 1; }
	uint16_t getMaxByteSize() { return getMaxByteSize(_capacity); }
	uint16_t getMaxSize(uint16_t byteSize) { return byteSize - 2 - 2 * sizeof(T) + 1; }

	bool init() {
		if (_buffer != NULL) {
			return false;
		}
		// Allocate memory
		_buffer = (differential_buffer_t<T>*)calloc(getMaxByteSize(), sizeof(uint8_t));
		if (_buffer == NULL) {
			LOGw("Could not allocate memory");
			return false;
		}
		//		LOGd("Allocated memory at %u", _buffer);
		_allocatedSelf = true;
		// Also call clear to make sure we start with a clean buffer
		clear();
		return true;
	}

	bool deinit() {
		if (_buffer != NULL && _allocatedSelf) {
			free(_buffer);
		}
		_allocatedSelf = false;
		_buffer        = NULL;
		return true;
	}

	bool assign(buffer_ptr_t buffer, uint16_t bufferSize) {
		if (getMaxSize(bufferSize) < _capacity || _allocatedSelf) {
			LOGd("Could not assign at %u", buffer);
			LOGd("%u < %u", getMaxSize(bufferSize), _capacity);
			return false;
		}
		//		LOGd("assign at %u", buffer);
		_buffer = (differential_buffer_t<T>*)buffer;
		// Also call clear to make sure we start with a clean buffer
		clear();
		return true;
	}

	bool release() {
		if (_allocatedSelf) {
			return false;
		}
		_buffer = NULL;
		return true;
	}

	differential_buffer_t<T>* getBuffer() { return _buffer; }

	void clear() { _buffer->length = 0; }

	uint16_t size() const { return _buffer->length; }

	uint16_t capacity() const { return _capacity; }

	bool empty() const { return _buffer->length == 0; }

	bool full() const { return size() >= _capacity; }

	bool push(T value) {
		if (full()) {
			return false;
		}
		//		LOGd("push %u at %u buffer=%u", value, _buffer->length, _buffer);
		if (empty()) {
			_buffer->firstVal = value;
			_buffer->lastVal  = value;
			_buffer->length   = 1;
			return true;
		}

		int32_t diff = (int32_t)value - _buffer->lastVal;
		//		LOGd("diff=%i", diff);
		if (diff > 127 || diff < -127) {
			LOGw("diff too large! %u - %u", value, _buffer->lastVal);
			clear();
			return false;
		}
		_buffer->array[_buffer->length - 1] = diff;
		_buffer->length++;
		_buffer->lastVal = value;
		return true;
	}

	T pop() {
		assert(!empty(), "Buffer is empty");
		T value = _buffer->lastVal;
		if (size() == 1) {
			clear();
		}
		else {
			_buffer->lastVal -= _buffer->array[_buffer->length-- - 1];
		}
		return value;
	}

	T peekBack() const {
		assert(!empty(), "Buffer is empty");
		return _buffer->lastVal;
	}

	T peekFront() const {
		assert(!empty(), "Buffer is empty");
		return _buffer->firstVal;
	}

	bool getValue(T& value, const uint16_t index) const {
		if (index >= size()) {
			return false;
		}
		if (index == 0) {
			//			LOGd("getVal %u", index);
			value = _buffer->firstVal;
			return true;
		}
		//		LOGd("getVal %u diff=%i buffer=%u array=%u", index, _buffer->array[index - 1], _buffer, _buffer->array);
		value += _buffer->array[index - 1];
		return true;
	}

private:
	differential_buffer_t<T>* _buffer;
	uint16_t _capacity;
	bool _allocatedSelf;
};
