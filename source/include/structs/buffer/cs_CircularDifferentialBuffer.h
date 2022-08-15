/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 16, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

//#include <common/cs_Types.h>

#include <cstddef>
//#include <cstdint>

//#include <stdlib.h> // malloc, free
//#include <stdint.h> // uint32_t
//
//#include <logging/cs_Logger.h>

/** Differential Buffer implementation
 * @param T primitive type such as uint8_t
 *
 * Elements are added at the back and removed from the front. If the capacity
 * of the buffer is reached, the oldest element will be overwritten.
 *
 */
template <typename T>
class CircularDifferentialBuffer {
public:
	/** Default constructor
	 * @capacity the size with which the buffer should be initialized. A maximum
	 *   of capacity elements can be stored in the buffer before the oldest element
	 *   will be overwritten
	 * @initialize if true, the array will be directly initialized, otherwise
	 *   <CircularDifferentialBuffer::init()> has to be called before accessing the buffer
	 *
	 * If <initialize> is set to true, the buffer will be directly initialized. To avoid
	 * unnecessary allocation of memory, use initialize = false and call
	 * <CircularDifferentialBuffer::init()> only once the buffer is used.
	 */
	CircularDifferentialBuffer(uint16_t capacity = 32, bool initialize = false)
			: _array(NULL), _capacity(capacity), _head(0), _tail(-1), _contentSize(0) {
		if (initialize) {
			init();
		}
	}

	/** Default destructor
	 *
	 * Free memory allocated for the buffer
	 */
	virtual ~CircularDifferentialBuffer() { free(_array); }

	/** Initializes and allocates memory for the buffer based on the capacity
	 * used with the constructor
	 *
	 * If the constructor was called with initialize = false, then this function
	 * has to be called before the buffer can be accessed!
	 *
	 * @return true if memory allocation was successful, false otherwise
	 */
	bool init() {
		if (!_array) {
			_array = (int8_t*)calloc(_capacity - 1, sizeof(int8_t));
		}
		// also call clear to make sure the head and tail are reset and we
		// start with a clean buffer
		clear();
		return _array != NULL;
	}

	/** Clears the buffer
	 *
	 * The buffer is cleared by setting head and tail
	 * to the beginning of the buffer. The array itself
	 * doesn't have to be cleared
	 */
	void clear() {
		_head        = 0;
		_tail        = -1;
		_contentSize = 0;
	}

	/** Returns the number of elements stored
	 *
	 * @return the number of elements stored in the buffer
	 */
	uint16_t size() const { return _contentSize; }

	/** Returns the capacity of the buffer
	 *
	 * The capacity is the maximum number of elements that can be stored
	 * in the buffer
	 *
	 * @return the capacity of the buffer
	 */
	uint16_t capacity() const { return _capacity; }

	/** Checks if the buffer is empty
	 *
	 * @return true if empty, false otherwise
	 */
	bool empty() const { return size() == 0; }

	/** Checks if the buffer is full
	 *
	 * @return true if full, false otherwise
	 */
	bool full() const { return size() == capacity(); }

	/** Peek at the oldest element without removing it
	 *
	 * This returns the value of the oldest element without
	 * removing the element from the buffer. Use <CircularBuffer>>pop()>
	 * to get the value and remove it at the same time
	 *
	 * @return the value of the oldest element
	 */
	T peek() const { return _firstVal; }

	/** Get the first element of the buffer
	 * @val the value of the oldest element
	 *
	 * @return false when the buffer was empty
	 */
	bool getFirstElement(T& val) {
		if (!_contentSize) {
			return false;
		}
		//		_readIdx = _head;
		_readIdx = 0;
		val      = _firstVal;
		return true;
	}

	/** Get the next value of the buffer, after calling <CircularDifferentialBuffer>>getFirstElement()>
	 *
	 * @val the previous value, will be set to the value of the next element
	 *
	 * @return false when the last element has been reached (this read is invalid).
	 */
	bool getNextElement(T& val) {
		if (_readIdx >= (_contentSize - 1)) {
			//		if (_readIdx == _tail) {
			return false;
		}
		//		++_readIdx;
		//		_readIdx %= (_capacity-1);
		//		val += _array[_readIdx];
		val += _array[(_head + _readIdx++) % (_capacity - 1)];
		return true;
	}

	/** Get the first element of the buffer
	 * @val the value of the oldest element
	 *
	 * @return false when the buffer was empty
	 */
	bool getLastElement(T& val) {
		if (!_contentSize) {
			return false;
		}
		val = _lastVal;
		return true;
	}

	/** Get the serialized length
	 *
	 * @return the size to fit all elements in a uint8_t buffer
	 */
	uint16_t getSerializedLength() const {
		if (empty()) {
			return 0;
		}
		return sizeof(T) + size() - 1;
	}

	/** Write all elements to a buffer
	 * @buffer buffer to which to write the elements
	 *
	 * The buffer has to be of the correct size, use <CircularDifferentialBuffer>>getSerializedLength()>
	 * First sizeof(T) bytes is the first value, the bytes afterwards are increments (int8_t).
	 */
	void serialize(uint8_t* buffer);

	/** Get and remove the oldest element
	 *
	 * This returns the value of the oldest element and
	 * removes it from the buffer
	 *
	 * @return the value of the oldest element
	 */
	T pop() {
		T res = peek();
		_firstVal += _array[_head];
		incHead();
		return res;
	}

	/** Add an element to the end of the buffer
	 * @value the element to be added
	 *
	 * Elements are added at the end of the buffer and
	 * removed from the beginning. If the buffer is full
	 * the oldest element will be overwritten.
	 *
	 * @return false when the difference was too large, and thus when the buffer was cleared
	 */
	bool push(T value) {
		if (!_contentSize) {
			_firstVal    = value;
			_lastVal     = value;
			_contentSize = 1;
			return true;
		}

		int32_t diff = (int32_t)value - _lastVal;
		if (diff > 127 || diff < -127) {
			//			LOGd("difference too large! %i", diff);
			clear();
			return false;
		}

		incTail();
		if (_contentSize > _capacity) {
			_firstVal += _array[_head];
			incHead();
		}
		_array[_tail] = diff;
		_lastVal      = value;
		return true;
	}

private:
	/** Pointer to the array storing the difference of elements compared to the previous element */
	int8_t* _array;

	/** The capacity of the buffer (maximum number of elements) */
	uint16_t _capacity;

	/** Index of the head (next element to be removed) */
	uint16_t _head;

	/** Index of the tail (where the next element will be inserted) */
	uint16_t _tail;

	/** Number of elements stored in the buffer */
	uint16_t _contentSize;

	/** Value of the oldest element in the buffer */
	T _firstVal;

	/** Value of the newest element in the buffer */
	T _lastVal;

	/** Index of the last read value, used in getNextElement() */
	uint16_t _readIdx;

	/** Increases the tail.
	 *
	 * Increases the contentsSize and the index of the tail. It also
	 * wraps the tail around if the end of the array is reached.
	 */
	void incTail() {
		++_tail;
		_tail %= (_capacity - 1);
		++_contentSize;
	}

	/** Increases the head.
	 *
	 * Decreases the contentsSize and increases the index of the head.
	 * It also wraps around the head if the end of the array is reached.
	 */
	void incHead() {
		++_head;
		_head %= (_capacity - 1);
		--_contentSize;
	}
};
