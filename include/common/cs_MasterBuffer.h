/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 2, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstdlib>

#include "drivers/cs_Serial.h"

class MasterBuffer {

	typedef uint8_t* buffer_ptr_t;

private:
	uint8_t* _buffer;
	uint16_t _size;

	bool _locked;

	MasterBuffer() :_buffer(NULL), _size(0), _locked(false) {};
	~MasterBuffer() {
		if (_buffer) {
			free(_buffer);
		}
	};

public:
	static MasterBuffer& getInstance() {
		static MasterBuffer instance;
		return instance;
	}

	void alloc(uint16_t size) {
		LOGd("alloc %d", size);
		_size = size;
		_buffer = (uint8_t*)calloc(_size, sizeof(uint8_t));
		LOGd("buffer: %p", _buffer);
	}

	void clear() {
		LOGd("clear");
		if (_buffer) {
			memset(_buffer, 0, _size);
		}
	}

	bool lock() {
		LOGd("lock");
		if (!_locked) {
			_locked = true;
			return true;
		} else {
			return false;
		}
	}

	bool unlock() {
		LOGd("unlock");
		if (_locked) {
			_locked = false;
			return true;
		} else {
			return false;
		}
	}

	bool isLocked() { return _locked; }

	bool getBuffer(buffer_ptr_t& buffer, uint16_t& maxLength) {
		LOGd("getBuffer");
		if (_buffer) {
			buffer = _buffer;
			maxLength = _size;
			return true;
		} else {
			return false;
		}
	}

	buffer_ptr_t getBuffer() {
		return _buffer;
	}

	uint16_t size() { return _size; }

};
