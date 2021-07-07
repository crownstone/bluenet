/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdlib>

#include <common/cs_Types.h>
#include <logging/cs_Logger.h>

#include <cstring>

/** EncryptionBuffer is a byte array with header.
 *
 * The EncryptionBuffer is used to put in all kind of data. This data is unorganized. The EncryptionBuffer can also be
 * accessed through more dedicated structures. This allows to read/write from the buffer directly or other types of
 * sophisticated objects.
 *
 * The disadvantage is that the data will be overwritten by the different accessors. The advantage is that the data
 * fits actually in the device RAM.
 */
class EncryptionBuffer {

private:
	buffer_ptr_t _buffer;
	uint16_t _size;

	bool _locked;

	EncryptionBuffer() :_buffer(NULL), _size(0), _locked(false) {};
	~EncryptionBuffer() {
		if (_buffer) {
			free(_buffer);
		}
	};

public:
	static EncryptionBuffer& getInstance() {
		static EncryptionBuffer instance;
		return instance;
	}

	void alloc(uint16_t size) {
		LOGd("Allocate buffer [%d]", size);
		_size = size;
		_buffer = (buffer_ptr_t)calloc(_size, sizeof(uint8_t));
//		LOGd("buffer: 0x%p", _buffer);
	}

	void clear() {
//		LOGd("clear");
		if (_buffer) {
			memset(_buffer, 0, _size);
		}
	}

	bool getBuffer(buffer_ptr_t& buffer, uint16_t& maxLength) {
//		LOGd("getBuffer");
		if (_buffer) {
			buffer = _buffer;
			maxLength = _size;
			return true;
		} else {
			return false;
		}
	}

//	buffer_ptr_t getBuffer() {
//		return _buffer;
//	}

	uint16_t size() { return _size; }

};
