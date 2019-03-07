/** StreamBuffer wraps an array of U elements of type T.
 *
 * The number of elements in the StreamBuffer is defined at compile time to be U. The type of the elements is T.
 * The StreamBuffer also contains a so-called header that defines a type, opcode and length. 
 *   The type typically is set to e.g. 'STATE_SWITCH_STATE', 'STATE_TEMPERATURE', etc.
 *   The opcode is meant for advertising, e.g. 'READ', 'WRITE', 'NOTIFY'.
 *   The memory allows U chunks, however the length parameter allows the use of fewer than U elements.
 *
 * The StreamBuffer is typically used in the CrownstoneService class.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Strings.h>
#include <common/cs_Types.h>
#include <structs/cs_BufferAccessor.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

#define SB_SUCCESS                               0
#define SB_BUFFER_NOT_INITIALIZED                1
#define SB_BUFFER_NOT_LARGE_ENOUGH               2

enum OpCode {
	OPCODE_READ_VALUE       = 0,
	OPCODE_WRITE_VALUE      = 1,
//	OPCODE_NOTIFY_VALUE     = 2, // Deprecated
	OPCODE_ERR_VALUE        = 3
};

#define SB_HEADER_SIZE                           sizeof(stream_buffer_header_t)

/** Structure for a StreamBuffer
 *
 * typename T defines the type of the payload elements
 * typename U defines the number of elements in the payload
 */
template <typename T, int U>
struct __attribute__((__packed__)) stream_buffer_t {
	stream_buffer_header_t header;
	T payload[U];
};

//! default payload length for a stream buffer. needs <MASTER_BUFFER_SIZE> to be defined
#define DEFAULT_PAYLOAD_LENGTH ((MASTER_BUFFER_SIZE-SB_HEADER_SIZE)/sizeof(T))

/** General StreamBuffer with type, opcode, length, and payload
 *
 * General class that can be used to send arrays of values over Bluetooth. For the structure, see the
 * <stream_t> struct.
 *
 * typename T defines the type of the payload elements
 * typename U defines the number of elements in the payload. by default, the number of elements is defined
 *            as (<MASTER_BUFFER_SIZE> - <SB_HEADER_SIZE>) / sizeof(T)
 *            but it can be overwritten if a smaller payload array should be used.
 */
template <typename T, int U = DEFAULT_PAYLOAD_LENGTH>
class StreamBuffer : public BufferAccessor {
public:
	/** Default constructor
	 *
	 * Constructor does not initialize the payload.
	 */
	StreamBuffer(): _buffer(NULL), _maxLength(0) {
	};

	/** @inherit */
	int assign(uint8_t *buffer, uint16_t size) {
		assert(SB_HEADER_SIZE + _max_items*_item_size <= size, STR_ERR_BUFFER_NOT_LARGE_ENOUGH);

#ifdef PRINT_STREAMBUFFER_VERBOSE
		LOGd(FMT_ASSIGN_BUFFER_LEN, buffer, size);
#endif

		_buffer = (stream_buffer_t<T, U>*)buffer;
		_maxLength = size;
		return 0;
	}

	/** Create a string from payload.
	 *
	 * This creates a C++ string from the uint8_t payload array. Note that this not always makes sense! It will
	 * use the length field to establish the length of the array to be used. So, the array does not have to
	 * have a null-terminated byte.
	 */
	cs_ret_code_t toString(std::string &str) {
		if (!_buffer) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
			return SB_BUFFER_NOT_INITIALIZED;
		}
		str = std::string((char*)_buffer->payload, length());
		return SB_SUCCESS;
	}

	/** Fill the StreamBuffer object from the string
	 *
	 * @str the string with which the buffer should be filled
	 *
	 *
	 */
	cs_ret_code_t fromString(std::string& str) {
		if (str.length() > (_max_items * _item_size)) {
			LOGe(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
			return SB_BUFFER_NOT_LARGE_ENOUGH;
		}
		memcpy(_buffer->payload, str.c_str(), str.length());
		_buffer->header.length = str.length();
		return SB_SUCCESS;
	}

	/** Add a value to the stream buffer
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
	cs_ret_code_t add(T value) {
		if (!_buffer) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
			return SB_BUFFER_NOT_INITIALIZED;
		}
		if (_buffer->header.length >= _max_items) {
			LOGe(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
			return SB_BUFFER_NOT_LARGE_ENOUGH;
		}
		_buffer->payload[_buffer->header.length++] = value;
		return SB_SUCCESS;
	}

	/** Clear the buffer
	 *
	 * Reset the payload array and set "length" to 0
	 */
	cs_ret_code_t clear() {
		if (!_buffer) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
			return SB_BUFFER_NOT_INITIALIZED;
		}
		memset(_buffer->payload, 0, _max_items * _item_size);
		_buffer->header.length = 0;
		return SB_SUCCESS;
	}

	/** Return the type assigned to the SreamBuffer
	 *
	 * @return the type, see <ConfigurationTypes>
	 */
	inline uint8_t type() const { 
		if (!_buffer) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
		}
		return _buffer->header.type; 
	}

	/** Get the length/size of the payload in number of elements
	 *
	 * @return number of elements stored
	 */
	inline uint16_t length() const { 
		if (!_buffer) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
		}
		return std::min(_buffer->header.length, _maxLength);
	}

	/** Get a pointer to the payload array
	 *
	 * @return pointer to the array used to store
	 *   the elements of the stream buffer
	 */
	inline T* payload() const { 
		if (!_buffer) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
		}
		return _buffer->payload; 
	}

	/** Set the type for this stream buffer
	 *
	 * @type the type, see <ConfigurationTypes>
	 */
	inline void setType(uint8_t type) { 
		if (!_buffer) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
		}
		_buffer->header.type = type; 
	}

	/** Return the opcode assigned to the SreamBuffer
	 *
	 * @return the opcode, see <OpCode>
	 */
	inline uint8_t opCode() const { 
		if (!_buffer) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
		}
		return _buffer->header.opCode; 
	}

	/** Set the opcode for this stream buffer
	 *
	 * @opcode the opcode, see <OpCode>
	 */
	inline void setOpCode(uint8_t opCode) { 
		if (!_buffer) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
		}
		_buffer->header.opCode = opCode; 
	}

	/** Set payload of the buffer.
	 *
	 * @payload pointer to the buffer containing the
	 *   elements which should be copied to this stream
	 *   buffer
	 *
	 * @length number of elements in the payload
	 *
	 * If length is larger than the capacity, the function returns an error.
	 */
	cs_ret_code_t setPayload(T *payload, uint16_t length) {
		if (!_buffer || !_buffer->payload) {
			LOGe(FMT_NOT_INITIALIZED, "Buffer");
			return SB_BUFFER_NOT_INITIALIZED;
		}
		if (length > _max_items) {
			LOGe(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
			return SB_BUFFER_NOT_LARGE_ENOUGH;
		}
		_buffer->header.length = length;
		memcpy(_buffer->payload, payload, length * _item_size);
		return SB_SUCCESS;
	}

	void print() {
		BLEutil::printArray((uint8_t*)_buffer, getDataLength());
	}

	///////////! Bufferaccessor ////////////////////////////

	/** @inherit */
	uint16_t getDataLength() const {
		return SB_HEADER_SIZE + _buffer->header.length;
	}

	/** @inherit */
	uint16_t getMaxLength() const {
		return _maxLength;
	}

	/** @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

protected:
	/** Pointer to the data to be sent
	 */
	stream_buffer_t<T, U>* _buffer;

private:
	uint16_t _maxLength;

	const size16_t _item_size = sizeof(T);
	const size16_t _max_items = U;

};
