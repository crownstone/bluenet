/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdlib>

#include <common/cs_Types.h>
#include "drivers/cs_Serial.h"

#include <cstring>

/** size of the header used for long write
 * has to be a multiple of 4 (word alignment), even though only 6 are needed
 */
#define DEFAULT_OFFSET 8

/** MasterBuffer is a byte array with header.
 *
 * The MasterBuffer is used to put in all kind of data. This data is unorganized. The MasterBuffer can also be
 * accessed through more dedicated structures. This allows to read/write from the buffer directly ScanResults or
 * other types of sophisticated objects.
 *
 * The disadvantage is that the data will be overwritten by the different accessors. The advantage is that the data
 * fits actually in the device RAM.
 */
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
		LOGd("Allocate buffer [%d]", size);
		_size = size + DEFAULT_OFFSET;
		_buffer = (buffer_ptr_t)calloc(_size, sizeof(uint8_t));
		//LOGd("buffer: %p", _buffer);
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

//	buffer_ptr_t getBuffer() {
//		return _buffer;
//	}

	uint16_t size(uint16_t offset = DEFAULT_OFFSET) { return _size - offset; }

};
