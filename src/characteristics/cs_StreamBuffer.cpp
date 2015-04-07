/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "characteristics/cs_StreamBuffer.h"

/////////////////////////////////////////////////////////////////////////////////
// uint8_t
/////////////////////////////////////////////////////////////////////////////////

template<>
StreamBuffer<uint8_t>* StreamBuffer<uint8_t>::fromString(std::string& str) {
	StreamBuffer<uint8_t>* buffer = new StreamBuffer<uint8_t>(str.length());
	buffer->_plength = str.length();
	memcpy(buffer->_payload, str.c_str(), buffer->_plength);
	return buffer;
}

template<>
void StreamBuffer<uint8_t>::serialize(uint8_t* buffer, uint16_t length) const {
	if (length < 3) return; // throw error

	uint8_t *ptr = buffer;
	*ptr++ = _type;
	*ptr++ = _plength;

	if (_plength) memcpy(ptr, _payload, _plength * sizeof(uint8_t));

//	LOGd("serialize...");
//	BLEutil::printArray(buffer, length);
}

template<>
void StreamBuffer<uint8_t>::deserialize(uint8_t* buffer, uint16_t length) {
	if (length < 3) return;

	uint8_t *ptr = buffer;
	_type = *ptr++;
	_plength = *ptr++;

	init(_plength);
	memcpy(_payload, ptr, _plength * sizeof(uint8_t));

//	LOGd("deserialize...");
//	BLEutil::printArray(_payload, _plength);
}

/////////////////////////////////////////////////////////////////////////////////
// uint16_t
/////////////////////////////////////////////////////////////////////////////////

template<>
void StreamBuffer<uint16_t>::serialize(uint8_t* buffer, uint16_t length) const {
	if (length < 3) return; // throw error

	uint8_t *ptr = buffer;
	*ptr++ = _type;
	*ptr++ = _plength;

	for (int i = 0; i < _plength; ++i) {
		*ptr++ = (_payload[i] >> 8) & 0xFF;
		*ptr++ = _payload[i] & 0xFF;
	}
}

template<>
void StreamBuffer<uint16_t>::deserialize(uint8_t* buffer, uint16_t length) {
	if (length < 3) return;

	uint8_t *ptr = buffer;
	_type = *ptr++;
	_plength = *ptr++;

	init(_plength);

	for (int i = 0; i < _plength; ++i) {
		_payload[i] = (*ptr++ << 8) & 0xFF00;
		_payload[i] |= *ptr++;
	}
}

/////////////////////////////////////////////////////////////////////////////////
// uint32_t
/////////////////////////////////////////////////////////////////////////////////

template<>
void StreamBuffer<uint32_t>::serialize(uint8_t* buffer, uint16_t length) const {
	if (length < 3) return; // throw error

	uint8_t *ptr = buffer;
	*ptr++ = _type;
	*ptr++ = _plength;

	for (int i = 0; i < _plength; ++i) {
		*ptr++ = (_payload[i] >> 24) & 0xFF;
		*ptr++ = (_payload[i] >> 16) & 0xFF;
		*ptr++ = (_payload[i] >> 8) & 0xFF;
		*ptr++ = _payload[i] & 0xFF;
	}
}

template<>
void StreamBuffer<uint32_t>::deserialize(uint8_t* buffer, uint16_t length) {
	if (length < 3) return;

	uint8_t *ptr = buffer;
	_type = *ptr++;
	_plength = *ptr++;

	init(_plength);

	for (int i = 0; i < _plength; ++i) {
		_payload[i] = (*ptr++ << 24) & 0xFF000000;
		_payload[i] |= (*ptr++ << 16) & 0xFF0000;
		_payload[i] |= (*ptr++ << 8) & 0xFF00;
		_payload[i] |= *ptr++;
	}
}
