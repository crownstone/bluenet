/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 27, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdlib>
#include "common/cs_Types.h"
#include "drivers/cs_Serial.h"
#include <cfg/cs_Strings.h>

//#define PRINT_CIRCULARBUFFER_VERBOSE

/** Circular Buffer implementation
 * @param T Element type of elements within the buffer.
 *
 * Elements are added at the back and removed from the front. If the capacity
 * of the buffer is reached, the oldest element will be overwritten.
 */
template <class T>
class CircularBuffer {
public:
	/** Default constructor
	 */
	CircularBuffer(uint16_t capacity): _array(NULL), _capacity(capacity), _head(0), _tail(-1), _contentsSize(0), _allocatedSelf(false)
	{
	}

	/** Default destructor
	 */
	virtual ~CircularBuffer() {
		deinit();
	}

	uint16_t getMaxByteSize(uint16_t capacity) { return capacity * sizeof(T); }
	uint16_t getMaxByteSize() { return getMaxByteSize(_capacity); }
	uint16_t getMaxSize(uint16_t byteSize) { return byteSize / sizeof(T); }

	/** Initializes and allocates memory for the buffer based on the capacity.
	 *
	 * @capacity the number of elements that should be stored in this buffer, before overwriting the oldest element.
	 *
	 * @return true if memory allocation was successful, false otherwise
	 */
	bool init() {
		if (_array != NULL) {
			return false;
		}
		// Allocate memory
		_array = (T*)calloc(_capacity, sizeof(T));
		if (_array == NULL) {
			LOGw(STR_ERR_ALLOCATE_MEMORY);
			return false;
		}

#ifdef PRINT_CIRCULARBUFFER_VERBOSE
		LOGd(FMT_ALLOCATE_MEMORY, _array);
#endif

		_allocatedSelf = true;
		// Also call clear to make sure we start with a clean buffer
		clear();
		return true;
	}

	/**
	 *
	 */
	bool deinit() {
		if (_array != NULL && _allocatedSelf) {
			free(_array);
		}
		_allocatedSelf = false;
		_array = NULL;
		return true;
	}

	/** Assign the buffer used to store the data, instead of allocating it via init().
	 * @buffer The buffer to be used.
	 * @bufferSize Size of the buffer.
	 *
	 * @return true on success.
	 */
	bool assign(buffer_ptr_t buffer, uint16_t bufferSize) {
		if (getMaxSize(bufferSize) < _capacity || _allocatedSelf) {
			LOGd(FMT_ERR_ASSIGN_BUFFER, buffer, bufferSize);
			return false;
		}
		_array = (T*) buffer;

#ifdef PRINT_CIRCULARBUFFER_VERBOSE
		LOGd(FMT_ASSIGN_BUFFER_LEN, buffer, bufferSize);
#endif

		// Also call clear to make sure we start with a clean buffer
		clear();
		return true;
	}

	/** Release the buffer that was assigned.
	 *
	 * @return true on success.
	 */
	bool release() {
		if (_allocatedSelf) {
			return false;
		}
		_array = NULL;
		return true;
	}

	T* getBuffer() {
		return _array;
	}

	/** Clears the buffer
	 *
	 * The buffer is cleared by setting head and tail
	 * to the beginning of the buffer. The array itself
	 * doesn't have to be cleared
	 */
	void clear() {
		_head = 0;
		_tail = -1;
		_contentsSize = 0;
	}

	/** Returns the number of elements stored
	 *
	 * @return the number of elements stored in the buffer
	 */
	uint16_t size() const {
		return _contentsSize;
	}

	/** Returns the capacity of the buffer
	 *
	 * The capacity is the maximum number of elements that can be stored
	 * in the buffer
	 *
	 * @return the capacity of the buffer
	 */
	uint16_t capacity() const {
		return _capacity;
	}

	/** Checks if the buffer is empty
	 *
	 * @return true if empty, false otherwise
	 */
	bool empty() const {
		return size() == 0;
	}

	/** Checks if the buffer is full
	 *
	 * @return true if full, false otherwise
	 */
	bool full() const {
		return size() == capacity();
	}

	/** Add an element to the end of the buffer
	 *
	 * @value the element to be added
	 *
	 * Elements are added at the end of the buffer and
	 * removed from the beginning. If the buffer is full
	 * the oldest element will be overwritten.
	 */
	void push(T& value) {
		incTail();
		if (_contentsSize > _capacity) {
			incHead();
		}
		_array[_tail] = value;
	}

	/** Get the oldest element
	 *
	 * This returns the value of the oldest element and
	 * removes it from the buffer
	 *
	 * @return the value of the oldest element
	 */
	T& pop() {
		T res = peek();
		incHead();
		return res;
	}

	/** Peek at the oldest element without removing it
	 *
	 * This returns the value of the oldest element without
	 * removing the element from the buffer. Use <CircularBuffer>>pop()>
	 * to get the value and remove it at the same time
	 *
	 * @return the value of the oldest element
	 */
	T& peek() const {
		return _array[_head];
	}

	/** Returns the Nth value, starting from oldest element
	 *
	 * Does NOT check if you reached the end, make sure you read no more than size().
	 */
	T& operator[](uint16_t idx) const {
		return _array[(_head+idx)%_capacity];
	}

private:
	/** Pointer to the array storing the elements */
	T *_array;

	/** The capacity of the buffer (maximum number of elements) */
	uint16_t _capacity;

	/** Index of the head (next element to be removed) */
	uint16_t _head;

	/** Index of the tail (where the next element will be inserted) */
	uint16_t _tail;

	/** Number of elements stored in the buffer */
	uint16_t _contentsSize;

	/** Whether the array was allocated by init() or not */
	bool _allocatedSelf;

	/** Increases the tail.
	 *
	 * Increases the contentsSize and the index of the tail. It also
	 * wraps the tail around if the end of the array is reached.
	 */
	void incTail() {
		++_tail;
		_tail %= _capacity;
		++_contentsSize;
	}

	/** Increases the head.
	 *
	 * Decreases the contentsSize and increases the index of the head.
	 * It also wraps around the head if the end of the array is reached.
	 */
	void incHead() {
		++_head;
		_head %= _capacity;
		--_contentsSize;
	}

};
