/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Feb 27, 2015
 * License: LGPLv3+
 */
#pragma once

//#include <common/cs_Types.h>

#include <cstddef>
//#include <cstdint>

//#include <stdlib.h> // malloc, free
//#include <stdint.h> // uint32_t
//
//#include "drivers/cs_Serial.h"

/* Circular Buffer implementation
 *
 * Elements are added at the back and removed from the front. If the capacity
 * of the buffer is reached, the oldest element will be overwritten.
 */
template <class T>
class CircularBuffer {
public:
	/* Default constructor
	 *
	 * @capacity the size with which the buffer should be initialized. A maximum
	 *   of capacity elements can be stored in the buffer before the oldest element
	 *   will be overwritten
	 *
	 * @initialize if true, the array will be directly initialized, otherwise
	 *   <CircularBuffer::init()> has to be called before accessing the buffer
	 *
	 * If <initialize> is set to true, the buffer will be directly initialized. To avoid
	 * unnecessary allocation of memory, use initialize = false and call
	 * <CircularBuffer::init()> only once the buffer is used.
	 */
	CircularBuffer(uint16_t capacity = 32, bool initialize = false) :
		_array(NULL), _capacity(capacity), _head(0), _tail(-1), _contentsSize(0)
	{
		if (initialize) {
			init();
		}
	}

	/* Default destructor
	 *
	 * Free memory allocated for the buffer
	 */
	virtual ~CircularBuffer() {
		if (_array != NULL) {
			free(_array);
		}
	}

	/* Initializes and allocates memory for the buffer based on the capacity
	 * used with the constructor
	 *
	 * If the constructor was called with initialize = false, then this function
	 * has to be called before the buffer can be accessed!
	 *
	 * @return true if memory allocation was successful, false otherwise
	 */
	bool init() {
		if (!_array) {
			_array = (T*)calloc(_capacity, sizeof(T));
		}
		// also call clear to make sure the head and tail are reset and we
		// start with a clean buffer
		clear();
		return _array != NULL;
	}

	/* Returns the number of elements stored
	 *
	 * @return the number of elements stored in the buffer
	 */
	uint16_t size() const {
		return _contentsSize;
	}

	/* Returns the capacity of the buffer
	 *
	 * The capacity is the maximum number of elements that can be stored
	 * in the buffer
	 *
	 * @return the capacity of the buffer
	 */
	uint16_t capacity() const {
		return _capacity;
	}

	/* Checks if the buffer is empty
	 *
	 * @return true if empty, false otherwise
	 */
	bool empty() const {
		return size() == 0;
	}

	/* Checks if the buffer is full
	 *
	 * @return true if full, false otherwise
	 */
	bool full() const {
		return size() == capacity();
	}

	/* Peek at the oldest element without removing it
	 *
	 * This returns the value of the oldest element without
	 * removing the element from the buffer. Use <CircularBuffer>>pop()>
	 * to get the value and remove it at the same time
	 *
	 * @return the value of the oldest element
	 */
	T peek() const {
		return _array[_head];
	}

	/* Get the oldest element
	 *
	 * This returns the value of the oldest element and
	 * removes it from the buffer
	 *
	 * @return the value of the oldest element
	 */
	T pop() {
		T res = peek();
		incHead();
		return res;
	}

	/* Add an element to the end of the buffer
	 *
	 * @value the element to be added
	 *
	 * Elements are added at the end of the buffer and
	 * removed from the beginning. If the buffer is full
	 * the oldest element will be overwritten.
	 */
	void push(T value) {
		incTail();
		if (_contentsSize > _capacity) {
			incHead();
		}
		_array[_tail] = value;
	}

	/* Clears the buffer
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

	/* Returns the Nth value, starting from oldest element
	 *
	 * Does NOT check if you reached the end, make sure you read no more than size().
	 */
	T operator[](uint16_t idx) const {
		return _array[(_head+idx)%_capacity];
	}

private:
	/* Pointer to the array storing the elements */
	T *_array;

	/* The capacity of the buffer (maximum number of elements) */
	uint16_t _capacity;

	/* Index of the head (next element to be removed) */
	uint16_t _head;

	/* Index of the tail (where the next element will be inserted) */
	uint16_t _tail;

	/* Number of elements stored in the buffer */
	uint16_t _contentsSize;

	/* Increases the tail.
	 *
	 * Increases the contentsSize and the index of the tail. It also
	 * wraps the tail around if the end of the array is reached.
	 */
	void incTail() {
		++_tail;
		_tail %= _capacity;
		++_contentsSize;
	}

	/* Increases the head.
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
