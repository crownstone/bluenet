/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "characteristics/cs_StreamBuffer.h"


StreamBuffer::StreamBuffer() : _type(0), _plength(0) {
	_payload = (uint8_t*)calloc(MAX_BUFFER_SERIALIZED_SIZE, sizeof(uint8_t));
}

bool StreamBuffer::operator!=(const StreamBuffer &other) {
	if (_type != other._type) return true;
	if (_plength != other._plength) return true;
	if (memcmp(_payload, other._payload, MAX_BUFFER_SERIALIZED_SIZE) != 0) return true;
	return false;
}

uint16_t StreamBuffer::getSerializedLength() const {
	return MAX_BUFFER_SERIALIZED_SIZE;
}

bool StreamBuffer::toString(std::string &str) {
	if (!_plength) return false;
	str = std::string((char*)_payload, _plength);
	return true;
}

bool StreamBuffer::fromString(const std::string &str) {
	_plength = str.length();
	_plength = 
		((_plength > MAX_BUFFER_SERIALIZED_SIZE) ? MAX_BUFFER_SERIALIZED_SIZE : _plength); 
	memcpy(_payload, str.c_str(), _plength*sizeof(uint8_t));
}
	
void StreamBuffer::setPayload(uint8_t *payload, uint8_t plength) {
	_plength = 
		((plength > MAX_BUFFER_SERIALIZED_SIZE) ? MAX_BUFFER_SERIALIZED_SIZE : plength); 
	memcpy(_payload, payload, _plength*sizeof(uint8_t));
}

uint8_t StreamBuffer::add(uint8_t value) {
	if (_plength >= MAX_BUFFER_SERIALIZED_SIZE - 1) return 1;
	_plength++;
	_payload[_plength] = value;
	return 0;
}

void StreamBuffer::serialize(uint8_t* buffer, uint16_t length) const {
	if (length < 3) return; // throw error
	
	uint8_t *ptr = buffer;
	*ptr++ = _type;
	*ptr++ = _plength;
	if (_plength) memcpy(ptr, _payload, _plength*sizeof(uint8_t));
}

void StreamBuffer::deserialize(uint8_t* buffer, uint16_t length) {
	if (length < 3) return;

	uint8_t *ptr = buffer;
	_type = *ptr++;
	_plength = *ptr++;
	memset(_payload, 0, MAX_BUFFER_SERIALIZED_SIZE);
	uint16_t corr_plength = 
		((_plength > MAX_BUFFER_SERIALIZED_SIZE) ? MAX_BUFFER_SERIALIZED_SIZE : _plength); 
	memcpy(_payload, ptr, corr_plength*sizeof(uint8_t));
}

