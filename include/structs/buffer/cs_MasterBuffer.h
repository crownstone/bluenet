/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 2, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstdlib>

#include <common/cs_Types.h>
#include "drivers/cs_Serial.h"

/* size of the header used for long write */
#define DEFAULT_OFFSET 6

class MasterBuffer {

private:
	buffer_ptr_t _buffer;
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
		_size = size + DEFAULT_OFFSET;
		_buffer = (buffer_ptr_t)calloc(_size, sizeof(uint8_t));
		LOGd("buffer: %p", _buffer);
	}

	void clear() {
//		LOGd("clear");
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

	bool getBuffer(buffer_ptr_t& buffer, uint16_t& maxLength, uint16_t offset = DEFAULT_OFFSET) {
//		LOGd("getBuffer");
		if (_buffer) {
			buffer = _buffer + offset;
			maxLength = _size - offset;
			return true;
		} else {
			return false;
		}
	}

	buffer_ptr_t getBuffer() {
		return _buffer;
	}

	uint16_t size(uint16_t offset = DEFAULT_OFFSET) { return _size - offset; }

};
