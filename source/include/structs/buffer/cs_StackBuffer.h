/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 27, 2016
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
struct __attribute__((__packed__)) stack_buffer_t {
	uint16_t length;
	T array[1];  //! Dummy size
};

/** Struct with fixed length, useful when sending as payload.
 *
 */
template <typename T, uint16_t S>
struct __attribute__((__packed__)) stack_buffer_fixed_t {
	uint16_t length;
	T array[S];
};

template <typename T>
class StackBuffer {
public:
	StackBuffer(uint16_t capacity) : _buffer(NULL), _capacity(capacity), _allocatedSelf(false) {}

	virtual ~StackBuffer() {}

	uint16_t getMaxByteSize(uint16_t capacity) { return 2 + capacity * sizeof(T); }
	uint16_t getMaxByteSize() { return getMaxByteSize(_capacity); }
	uint16_t getMaxSize(uint16_t byteSize) { return (byteSize - 2) / sizeof(T); }

	bool init() {
		if (_buffer != NULL) {
			return false;
		}
		// Allocate memory
		_buffer = (stack_buffer_t<T>*)calloc(getMaxByteSize(), sizeof(uint8_t));
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
			return false;
		}
		_buffer = (stack_buffer_t<T>*)buffer;
		//		LOGd("assign at %u array=%u", buffer, _buffer->array);
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

	stack_buffer_t<T>* getBuffer() { return _buffer; }

	void clear() { _buffer->length = 0; }

	uint16_t size() const { return _buffer->length; }

	uint16_t capacity() const { return _capacity; }

	bool empty() const { return (_buffer->length == 0); }

	bool full() const { return (size() >= _capacity); }

	bool push(T value) {
		if (full()) {
			return false;
		}
		//		LOGd("push %u at %u buffer=%u array=%u", value, _buffer->length, _buffer, _buffer->array);
		_buffer->array[_buffer->length++] = value;
		return true;
	}

	T pop() {
		assert(!empty(), "Buffer is empty");
		return _buffer->array[--(_buffer->length)];
	}

	T operator[](uint16_t idx) const {
		assert(idx < size(), "Index too large");
		//		LOGd("get %u buffer=%u array=%u", idx, _buffer, _buffer->array);
		return _buffer->array[idx];
	}

private:
	stack_buffer_t<T>* _buffer;
	uint16_t _capacity;
	bool _allocatedSelf;
};
