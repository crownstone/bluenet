/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "structs/buffer/cs_CharacteristicBuffer.h"

#include <logging/cs_Logger.h>

#include <cstdlib>
#include <cstring>

#include "util/cs_Error.h"

CharacteristicBuffer::CharacteristicBuffer() {}

CharacteristicBuffer::~CharacteristicBuffer() {
	if (_buffer) {
		free(_buffer);
	}
}

void CharacteristicBuffer::alloc(cs_buffer_size_t size) {
	LOGd("Allocate buffer [%d]", size);
	_size   = size + CS_CHAR_BUFFER_DEFAULT_OFFSET;
	_buffer = (buffer_ptr_t)calloc(_size, sizeof(uint8_t));
	LOGd("buffer: %p", _buffer);
}

void CharacteristicBuffer::clear() {
	//		LOGd("clear");
	if (_buffer) {
		memset(_buffer, 0, _size);
	}
}

bool CharacteristicBuffer::lock() {
	LOGd("lock");
	if (!_locked) {
		_locked = true;
		return true;
	}
	else {
		return false;
	}
}

bool CharacteristicBuffer::unlock() {
	LOGd("unlock");
	if (_locked) {
		_locked = false;
		return true;
	}
	else {
		return false;
	}
}

bool CharacteristicBuffer::isLocked() {
	return _locked;
}

cs_data_t CharacteristicBuffer::getBuffer(cs_buffer_size_t offset) {
	assert(_buffer != NULL, "Buffer not initialized");
	assert(offset <= _size, "Invalid arguments");
	cs_data_t data;
	data.data = _buffer + offset;
	data.len  = _size - offset;
	return data;
}

cs_buffer_size_t CharacteristicBuffer::size(cs_buffer_size_t offset) {
	return _size - offset;
}
