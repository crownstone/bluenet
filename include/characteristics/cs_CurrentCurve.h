/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Mar. 02, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "ble_gap.h"
#include "ble_gatts.h"

#include "cs_BluetoothLE.h"
#include "characteristics/cs_Serializable.h"
#include "common/cs_Config.h"
#include "cs_BufferAccessor.h"
//#include "drivers/cs_ADC.h"

using namespace BLEpp;

#define CURRENT_CURVE_HEADER_SIZE                (sizeof(uint16_t) + 2*sizeof(T) + 2*sizeof(uint32_t))
//#define CURRENT_CURVE_MAX_SAMPLES                90
#define CURRENT_CURVE_MAX_SAMPLES                (GENERAL_BUFFER_SIZE - CURRENT_CURVE_HEADER_SIZE + 1)

#define CC_SUCCESS                               0
#define CC_BUFFER_NOT_INITIALIZED                1
#define CC_BUFFER_NOT_LARGE_ENOUGH               2
#define CC_DIFFERENCE_TOO_LARGE                  3

typedef int CC_ERR_CODE;

/* Structure for the Current Curve
 *
 * Requires CURRENT_CURVE_SAMPLES to be set.
 */
template <typename T>
struct __attribute__((__packed__)) current_curve_t {
	uint16_t length;
	T firstValue;
	T lastValue;
	uint32_t firstTimeStamp;
	uint32_t lastTimeStamp;
	int8_t differences[CURRENT_CURVE_MAX_SAMPLES-1];
};

/* General StreamBuffer with type, length, and payload
 *
 * General class that can be used to send arrays of values over Bluetooth, of which the first byte is a type
 * or identifier byte, the second one the length of the payload (so, minus identifier and length byte itself)
 * and the next bytes are the payload itself.
 */
template <typename T>
class CurrentCurve : public BufferAccessor {
private:
	/* Pointer to the data to be sent
	 */
	current_curve_t<T>* _buffer;

//	const size_t _item_size = sizeof(T);
	const size_t _max_buf_size = CURRENT_CURVE_HEADER_SIZE + CURRENT_CURVE_MAX_SAMPLES-1;
public:
	/* Default constructor
	 *
	 * Constructor does not initialize the buffer.
	 */
	CurrentCurve(): _buffer(NULL) {
	};

	/* Initialize the buffer
	 *
	 * @param buffer                the buffer to be used
	 * @param size                  size of buffer
	 *
	 * Size of the assigned buffer (should be equal or larger than capacity).
	 *
	 * @return 0 on SUCCESS, 1 on FAILURE (buffer required too large)
	 */
	CC_ERR_CODE assign(buffer_ptr_t buffer, uint16_t size) {
		LOGd("assign, this: %p, buff: %p, len: %d", this, buffer, size);
		if (_max_buf_size > size) {
			LOGe("Assigned buffer is not large enough");
			return CC_BUFFER_NOT_LARGE_ENOUGH;
		}
		_buffer = (current_curve_t<T>*)buffer;
		return CC_SUCCESS;
	}

	/* Release the buffer
	 *
	 * Sets pointer to zero, does not deallocate memory.
	 */
	void release() {
		LOGd("Release current curve. This will screw up SoftDevice if characteristic is not deleted.");
		_buffer = NULL;
	}

	bool isAssigned() const {
		if (_buffer == NULL) return false;
		return true;
	}

//	/* @inherit */
//	bool operator!=(const CurrentCurve &other) {
//		if (_buffer != other._buffer) return true;
//		return false;
//	}

	bool isFull() {
		if (_buffer->length >= CURRENT_CURVE_MAX_SAMPLES) {
			return true;
		}
		return false;
	}

	CC_ERR_CODE getValue(const uint16_t index, T& voltage) const {
		if (index >= _buffer->length) {
			return CC_BUFFER_NOT_LARGE_ENOUGH;
		}
		if (index == 0) {
			voltage = _buffer->firstValue;
			return CC_SUCCESS;
		}
		voltage += _buffer->differences[index-1];
		return CC_SUCCESS;
	}

	uint32_t getTimeStart() const {
		return _buffer->firstTimeStamp;
	}

	uint32_t getTimeEnd() const {
		return _buffer->lastTimeStamp;
	}

	/* Add a value to the current curve
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
	CC_ERR_CODE add(const T value, const uint32_t timeStamp=0) {
		if (!_buffer) {
			LOGe("Buffer not initialized!");
			return CC_BUFFER_NOT_INITIALIZED;
		}
		if (_buffer->length >= CURRENT_CURVE_MAX_SAMPLES) {
//			LOGe("Buffer is not large enough");
			return CC_BUFFER_NOT_LARGE_ENOUGH;
		}

		if (!_buffer->length) {
			_buffer->length++;
			_buffer->firstValue = value;
			_buffer->lastValue = value;
			_buffer->firstTimeStamp = timeStamp;
			_buffer->lastTimeStamp = timeStamp;
		}
		else {
			int32_t diff = (int32_t)value - _buffer->lastValue;
			if (diff > 127 || diff < -127) {
				clear();
				return CC_DIFFERENCE_TOO_LARGE;
			}
			_buffer->differences[_buffer->length++] = diff;
			_buffer->lastValue = value;
			_buffer->lastTimeStamp = timeStamp;
		}
		return CC_SUCCESS;
	}

	/* Clear the buffer
	 *
	 * Set "length" to 0
	 */
	CC_ERR_CODE clear() {
		if (!_buffer) {
			LOGe("Buffer not initialized!");
			return CC_BUFFER_NOT_INITIALIZED;
		}
		_buffer->firstValue = 0;
		_buffer->lastValue = 0;
		_buffer->firstTimeStamp = 0;
		_buffer->lastTimeStamp = 0;
//		memset(_buffer->differences, 0, _capacity * _item_size);
		_buffer->length = 0;
		return CC_SUCCESS;
	}

	/* Get the length/size of the payload in number of elements
	 *
	 * @return number of elements stored
	 */
	inline uint16_t length() const { return _buffer->length; }


	/////////// Bufferaccessor ////////////////////////////

	uint16_t getDataLength() const {
		return CURRENT_CURVE_HEADER_SIZE + _buffer->length-1;
	}

	uint16_t getMaxLength() const {
		//return sizeof(uint16_t) + 2*sizeof(T) + CURRENT_CURVE_MAX_SAMPLES-1;
		return _max_buf_size;
	}

	// TODO: Why do we need this function!?
	// Why pointer of pointer?
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};
