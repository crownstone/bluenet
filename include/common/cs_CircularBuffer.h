/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Feb 27, 2015
 * License: LGPLv3+
 */
#pragma once

#include <stdlib.h> // malloc, free
#include <stdint.h> // uint32_t

#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"

template <class T>
class CircularBuffer {
public:
	CircularBuffer(uint16_t capacity = 32, bool initialize = false) : _array(NULL), _capacity(capacity), _head(0), _tail(-1), _contentsSize(0) {
		if (initialize) {
			init();
		}
	}
	virtual ~CircularBuffer() {
		free(_array);
	}

	bool init() {
		if (_array) {
			free(_array);
		}
		_array = (T*)calloc(_capacity, sizeof(T));
		return _array != NULL;
	}

	uint16_t size() const {
		return _contentsSize;
	}

	uint16_t capacity() const {
		return _capacity;
	}

	bool empty() const {
		return size() == 0;
	}

	bool full() const {
		return size() == capacity();
	}

	T top() const {
		return _array[_head];
	}

	T pop() {
		T res = top();
		incHead();
		return res;
	}

	bool push(T value) {
		incTail();
		if (_contentsSize > _capacity) {
			incHead();
		}
		_array[_tail] = value;

		return true;
	}

	void clear() {
		_head = 0;
		_tail = -1;
	}

private:
	T *_array;
	uint16_t _capacity;
	uint16_t _head;
	uint16_t _tail;
	uint16_t _contentsSize;

	void incTail() {
		++_tail;
		++_contentsSize;
		if (_tail == _capacity) {
			_tail = 0;
		}
	}

	void incHead() {
		++_head;
		--_contentsSize;
		if (_head == _capacity) {
			_head = 0;
		}
	}

};
