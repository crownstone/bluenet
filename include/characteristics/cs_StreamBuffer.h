/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "ble_gap.h"
#include "ble_gatts.h"

#include "cs_BluetoothLE.h"
#include "characteristics/cs_Serializable.h"
#include <string>
#include "util/cs_Utils.h"
#include "drivers/cs_Serial.h"
#include "common/cs_Config.h"
#include "cs_BufferAccessor.h"

using namespace BLEpp;

#define SB_HEADER_SIZE 3

#define SB_SUCCESS                               0
#define SB_BUFFER_NOT_INITIALIZED                1
#define SB_BUFFER_NOT_LARGE_ENOUGH               2

typedef uint8_t ERR_CODE;

/* Structure for a StreamBuffer
 *
 * Requires GENERAL_BUFFER_SIZE to be set.
 */
template <typename T>
struct __attribute__((__packed__)) stream_t {
	uint8_t type;
	uint16_t length;
	T payload[(GENERAL_BUFFER_SIZE-SB_HEADER_SIZE)/sizeof(T)];
};

/* General StreamBuffer with type, length, and payload
 *
 * General class that can be used to send arrays of values over Bluetooth, of which the first byte is a type
 * or identifier byte, the second one the length of the payload (so, minus identifier and length byte itself)
 * and the next bytes are the payload itself.
 */
template <typename T>
class StreamBuffer : public BufferAccessor {
private:
	/* Pointer to the data to be sent
	 */
	stream_t<T>* _buffer;

	const size_t _item_size = sizeof(T);
	const size_t _capacity = (GENERAL_BUFFER_SIZE-SB_HEADER_SIZE) / _item_size;
public:
	/* Default constructor
	 *
	 * Constructor does not initialize the payload.
	 */
	StreamBuffer(): _buffer(NULL) {
	};

	/* Initialize the buffer 
	 *
	 * @param buffer                the buffer to be used
	 * @param size                  size of buffer
	 *
	 * Size of the asigned buffer (should be equal or larger than capacity).
	 *
	 * @return 0 on SUCCESS, 1 on FAILURE (buffer required too large)
	 */
	int assign(uint8_t *buffer, uint16_t size) {
		LOGd("assign, this: %p, buff: %p, len: %d", this, buffer, size);
		if (_capacity > size) {
			LOGe("Assigned buffer is not large enough");
			return 1;
		}
		_buffer = (stream_t<T>*)buffer;
		return 0;
	}

	/* Release the buffer
	 *
	 * Sets pointer to zero, does not deallocate memory.
	 */
	void release() {
		LOGd("Release stream buffer. This will screw up SoftDevice if characteristic is not deleted.");
		_buffer = NULL;
	}

	/* Create a string from payload. 
	 *
	 * This creates a C++ string from the uint8_t payload array. Note that this not always makes sense! It will
	 * use the length field to establish the length of the array to be used. So, the array does not have to 
	 * have a null-terminated byte.
	 */
	ERR_CODE toString(std::string &str) {
		if (!_buffer) {
			LOGe("Buffer not initialized!");
			return SB_BUFFER_NOT_INITIALIZED;
		}
		str = std::string((char*)_buffer->payload, _buffer->length);
		return SB_SUCCESS;
	}

	/* Fill the StreamBuffer object from the string
	 *
	 * @str the string with which the buffer should be filled
	 *
	 * 
	 */
	ERR_CODE fromString(std::string& str) {
		if (str.length() > (_capacity * _item_size)) {
			LOGe("Buffer is not large enough");
			return SB_BUFFER_NOT_LARGE_ENOUGH;
		}
		memcpy(_buffer->payload, str.c_str(), str.length());
		_buffer->length = str.length();
		return SB_SUCCESS;
	}

	/* Add a value to the stream buffer
	 *
	 * @value the value to be added
	 *
	 * Adds the value to the buffer if it is initialized and
	 * not full
	 *
	 * @return 0 on SUCCESS,
	 *         1 if buffer is full,
	 *         2 if buffer is not initialized
	 */
	ERR_CODE add(T value) {
		if (!_buffer) {
			LOGe("Buffer not initialized!");
			return SB_BUFFER_NOT_INITIALIZED;
		}
		if (_buffer->length > _capacity) {
			LOGe("Buffer is not large enough");
			return SB_BUFFER_NOT_LARGE_ENOUGH;
		}
		_buffer->payload[_buffer->length++] = value;
		return SB_SUCCESS;
	}

	/* Clear the buffer
	 *
	 * Reset the payload array and set "length" to 0
	 */
	ERR_CODE clear() {
		if (!_buffer) {
			LOGe("Buffer not initialized!");
			return SB_BUFFER_NOT_INITIALIZED;
		}
		memset(_buffer->payload, 0, _capacity * _item_size);
		_buffer->length = 0;
		return SB_SUCCESS;
	}

	/* Return the type assigned to the SreamBuffer
	 *
	 * @return the type, see <ConfigurationTypes>
	 */
	inline uint8_t type() const { return _buffer->type; } 

	/* Get the length/size of the payload in number of elements
	 *
	 * @return number of elements stored
	 */
	inline uint16_t length() const { return _buffer->length; }

	/* Get a pointer to the payload array
	 *
	 * @return pointer to the array used to store
	 *   the elements of the stream buffer
	 */
	inline T* payload() const { return _buffer->payload; }

	/* Set the type for this stream buffer
	 *
	 * @type the type, see <ConfigurationTypes>
	 */
	inline void setType(uint8_t type) { _buffer->type = type; }

	/* Set payload of the buffer.
	 *
	 * @payload pointer to the buffer containing the
	 *   elements which should be copied to this stream
	 *   buffer
	 *
	 * @length number of elements in the payload
	 *
	 * If length is larger than the capacity, the function returns an error.
	 */
	ERR_CODE setPayload(T *payload, uint16_t length) {
		if (!_buffer->payload) {
			LOGe("Buffer not initialized!");
			return SB_BUFFER_NOT_INITIALIZED;
		}
		if (length > _capacity) { 
			LOGe("Buffer is not large enough");
			return SB_BUFFER_NOT_LARGE_ENOUGH;
		}
		_buffer->length = length;
		//_buffer->length = ((length > _capacity) ? _capacity : plength);
		memcpy(_buffer->payload, payload, length * _item_size);
		return SB_SUCCESS;
	}

	/////////// Bufferaccessor ////////////////////////////

	/* @inherit */
	uint16_t getDataLength() const {
		return SB_HEADER_SIZE + _buffer->length;
	}

	/* @inherit */
	uint16_t getMaxLength() const {
		return GENERAL_BUFFER_SIZE;
	}

	// TODO: Why do we need this function!?
	/* @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};
