/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "characteristics/cs_StreamBuffer.h"


StreamBuffer::StreamBuffer() : _type(0), _length(0) {
	_payload = (uint8_t*)calloc(MAX_BUFFER_SERIALIZED_SIZE, sizeof(uint8_t));
}

bool StreamBuffer::operator!=(const StreamBuffer &other) {
	if (_type != other._type) return true;
	if (_length != other._length) return true;
	if (memcmp(_payload, other._payload, MAX_BUFFER_SERIALIZED_SIZE) != 0) return true;
	return false;
}

uint32_t StreamBuffer::getSerializedLength() const {
	return MAX_BUFFER_SERIALIZED_SIZE;
}

uint8_t StreamBuffer::add(uint8_t value) {
	if (_length >= MAX_BUFFER_SERIALIZED_SIZE - 1) return 1;
	_length++;
	_payload[_length] = value;
	return 0;
}

void StreamBuffer::serialize(uint8_t* buffer, uint16_t length) const {
	if (length < 3) return; // throw error
	
	uint8_t *ptr = buffer;
	*ptr++ = _type;
	*ptr++ = _length;
	memcpy(ptr, _payload, _length*sizeof(uint8_t));
}

void StreamBuffer::deserialize(uint8_t* buffer, uint16_t length) {
	if (length < 3) return;

	_type = buffer[0];
	_length = buffer[1];
	memset(&_payload, 0, MAX_BUFFER_SERIALIZED_SIZE);
	uint16_t corr_length = 
		((_length > MAX_BUFFER_SERIALIZED_SIZE) ? MAX_BUFFER_SERIALIZED_SIZE : _length); 
	memcpy(&_payload, &buffer[2], corr_length*sizeof(uint8_t));
}

