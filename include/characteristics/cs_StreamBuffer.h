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

using namespace BLEpp;

#define MAX_CAPACITY 100

/**
 * General class that can be used to send arrays of values over Bluetooth, of which the first byte is a type
 * or identifier byte, the second one the length of the payload (so, minus identifier and length byte itself)
 * and the next bytes are the payload itself.
 */
template <typename T, typename U = uint8_t>
class StreamBuffer : public Serializable {

	/* Define the size of the header in bytes:
	 *
	 * 1 Byte for the type
	 * x Byte for the length, where x is defined by type U
	 */
	#define SB_HEADER_SIZE     sizeof(uint8_t) + sizeof(U)

private:
	/* type enum, defines the type of data in the payload, see <ConfigurationTypes> */
	uint8_t _type;

	/* length of the payload (in number of elements) */
	U _plength;

	/* pointer to the array storing the payload */
	T* _payload;

	/* capacity, maximum length of the payload. array is
	 * initialized with capacity * sizeof(T) bytes
	 */
	U _capacity;

public:
	/* Default constructor
	 *
	 * Create and initialize a stream buffer. The payload array is NOT initialized
	 * for this constructor. In order to use the stream buffer, <StreamBuffer::init()>
	 * has to be called first. If you know the capacity at creation, you can use
	 * the constructor <StreamBuffer(U capacity)
	 */
	StreamBuffer() :_type(0), _plength(0), _payload(NULL), _capacity(0) {};

	/* Constructor initializes also the payload array
	 *
	 * @capacity the capacity with which the payload array should be initialized
	 *
	 * For this constructor the payload array is initialized with size
	 * capacity * sizeof(T)
	 */
	StreamBuffer(U capacity) : StreamBuffer() {
		init(capacity);
	}

	/* Initialize the payload array
	 *
	 * @param capacity the capacity with which the payload should be initialized
	 *
	 * If the object was initialized beforehand already, the old payload array
	 * is freed prior to the new allocation.
	 */
	void init(U capacity) {
		if (capacity > MAX_CAPACITY) {
			LOGe("requested capacity: %d, max capacity allowed: %d", capacity, MAX_CAPACITY);
			return;
		}
		if (_payload) {
			free(_payload);
		}
		_capacity = capacity;
		_payload = (T*)calloc(capacity, sizeof(T));
	}

	/* @inherit */
	bool operator!=(const StreamBuffer &other) {
		if (_type != other._type) return true;
		if (_plength != other._plength) return true;
		if (memcmp(_payload, other._payload, _capacity * sizeof(T)) != 0) return true;
		return false;
	}

	/* Create a string from payload. 
	 *
	 * This creates a C++ string from the uint8_t payload array. Note that this not always makes sense! It will
	 * use the _plength field to establish the length of the array to be used. So, the array does not have to 
	 * have a null-terminated byte.
	 */
	bool toString(std::string &str) {
		if (!_plength) return false;
		str = std::string((char*)_payload, _plength);
		return true;
	}

	/* Create a StreamBuffer object from the string
	 *
	 * @str the string with which the buffer should be initialized
	 *
	 * The stream buffer is created with a capacity defined by the
	 * size of the string. then the string content is copied to the
	 * payload array
	 *
	 * *Note* needs specialization, for types other than uint8_t
	 *
	 * @return the created StreamBuffer object
	 */
	static StreamBuffer<T>* fromString(std::string& str);

	/* Create a StreamBuffer object from the payload
	 *
	 * @payload pointer to the payload with which the stream buffer
	 *   should be filled
	 *
	 * @length length (number of bytes) of the array pointed to by
	 *   @payload
	 *
	 * The stream buffer is created with the capacity defined by
	 * @length. then the payload from @payload is copied to the
	 * buffer's payload array
	 *
	 * @return the created StreamBuffer object
	 */
	static StreamBuffer<T>* fromPayload(T *payload, U plength) {
		StreamBuffer<T>* buffer = new StreamBuffer<uint8_t>(plength);
		buffer->_plength = plength;
		memcpy(buffer->_payload, payload, buffer->_plength * sizeof(T));
		return buffer;
	}

	/* Fill payload with string.
	 *
	 * @str the string to be copied to the payload.
	 *
	 * If the length of the string is bigger than the capacity of
	 * the stream buffer, the string will be capped at the size of the
	 * stream buffer.
	 *
	 * @return true if successful, false otherwise
	 */
	bool setString(const std::string & str) {
		if (!_payload) {
			LOGe("buffer not initialized!");
			return false;
		}
		_plength = str.length();
		_plength =
			((_plength > _capacity) ? _capacity : _plength);
		memcpy(_payload, str.c_str(), _plength * sizeof(T));
		return true;
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
	uint8_t add(T value) {
		if (!_payload) {
			LOGe("buffer not initialized!");
			return 2;
		}
		if (_plength >= _capacity) return 1;
		_payload[_plength++] = value;
		return 0;
	}

	/* Clear the buffer
	 *
	 * Reset the payload array and set <_plength> to 0
	 */
	void clear() {
		if (!_payload) {
			LOGe("buffer not initialized!");
			return;
		}
		memset(_payload, 0, _capacity * sizeof(T));
		_plength = 0;
	}

	/* Return the type assigned to the SreamBuffer
	 *
	 * @return the type, see <ConfigurationTypes>
	 */
	inline uint8_t type() const { return _type; } 

	/* Get the length/size of the payload in number
	 * of elements
	 *
	 * @return number of elements stored
	 */
	inline U length() const { return _plength; }

	/* Get a pointer to the payload array
	 *
	 * @return pointer to the array used to store
	 *   the elements of the stream buffer
	 */
	inline T* payload() const { return _payload; }

	/* Set the type for this stream buffer
	 *
	 * @type the type, see <ConfigurationTypes>
	 */
	inline void setType(uint8_t type) { _type = type; }

	/* Set payload of the buffer.
	 *
	 * @payload pointer to the buffer containing the
	 *   elements which should be copied to this stream
	 *   buffer
	 *
	 * @length number of elements in the payload
	 *
	 * If plength is bigger than the capacity, only the
	 * first capacity elements will be copied
	 */
	void setPayload(T *payload, U plength) {
		if (!_payload) {
			LOGe("buffer not initialized!");
			return;
		}
		_plength =
			((plength > _capacity) ? _capacity : plength);
		memcpy(_payload, payload, _plength * sizeof(T));
	}

	//////////// serializable ////////////////////////////

	/* @inherit */
    uint16_t getSerializedLength() const {
    	return _plength * sizeof(T) + SB_HEADER_SIZE;
    }

	/* @inherit */
    uint16_t getMaxLength() const {
		return MAX_CAPACITY;
    }

	/* Serialize entire class.
	 *
	 * This is used to stream this class over BLE. Note that length here includes the field for type and length,
	 * and is hence larger than plength (which is just the length of the payload).
	 *
	 * Needs specialization
	 */
    void serialize(uint8_t* buffer, uint16_t length) const;

	/* Deserialize class
	 *
	 * Needs specialization
	 */
    void deserialize(uint8_t* buffer, uint16_t length);

};
