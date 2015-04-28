/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Mar. 02, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

//#include "ble_gap.h"
//#include "ble_gatts.h"
//
//#include "cs_BluetoothLE.h"
//#include "common/cs_Config.h"
#include <common/cs_Types.h>
#include "structs/cs_BufferAccessor.h"

using namespace BLEpp;

#define CURRENT_CURVE_HEADER_SIZE                (sizeof(uint16_t) + 2*sizeof(T) + 2*sizeof(uint32_t))
//#define CURRENT_CURVE_MAX_SAMPLES                (MASTER_BUFFER_SIZE - CURRENT_CURVE_HEADER_SIZE + 1)
#define CURRENT_CURVE_MAX_SAMPLES                ((MASTER_BUFFER_SIZE - CURRENT_CURVE_HEADER_SIZE) / 2 + 1)

#define CC_SUCCESS                               0
#define CC_BUFFER_NOT_INITIALIZED                1
#define CC_BUFFER_NOT_LARGE_ENOUGH               2
#define CC_DIFFERENCE_TOO_LARGE                  3

typedef uint8_t CC_ERR_CODE;

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
	int8_t valueDiffs[CURRENT_CURVE_MAX_SAMPLES-1];
	int8_t timeDiffs[CURRENT_CURVE_MAX_SAMPLES-1];
};

/* Current Curve
 *
 * Class used to send the current samples over Bluetooth.
 * In order to save space, only differences of the samples are stored.
 * We only store the first and last time stamp, not of every sample.
 * You can assume the sampling is done at a gradual speed.
 */
template <typename T>
class CurrentCurve : public BufferAccessor {
private:
	current_curve_t<T>* _buffer;
//	const size_t _item_size = sizeof(T);
//	const size_t _max_buf_size = CURRENT_CURVE_HEADER_SIZE + CURRENT_CURVE_MAX_SAMPLES-1;
	const size_t _max_buf_size = CURRENT_CURVE_HEADER_SIZE + 2*CURRENT_CURVE_MAX_SAMPLES-2;
public:
	/* Default constructor
	 *
	 * Constructor does not initialize the buffer.
	 */
	CurrentCurve(): _buffer(NULL) {
	};

	/* Release the buffer
	 *
	 * Sets pointer to zero, does not deallocate memory.
	 */
	void release() {
		LOGd("Release current curve. This will screw up SoftDevice if characteristic is not deleted.");
		_buffer = NULL;
	}

	/* Check if a buffer is assigned
	 *
	 * @return true if a buffer is assigned
	 */
	bool isAssigned() const {
		if (_buffer == NULL) return false;
		return true;
	}

	/* Check if the buffer is full
	 *
	 * @return true when you cannot <add> any more samples.
	 */
	bool isFull() {
		if (_buffer->length >= CURRENT_CURVE_MAX_SAMPLES) {
			return true;
		}
		return false;
	}

	/* Get i-th sample.
	 * @index Index of the sample you want to get.
	 * @voltage Value of previous sample (index-1), this value will be set to the i-th sample.
	 *
	 * @return CC_SUCCESS on success.
	 */
	CC_ERR_CODE getValue(const uint16_t index, T& value, uint32_t& timeStamp) const {
		if (index >= _buffer->length) {
			return CC_BUFFER_NOT_LARGE_ENOUGH;
		}
		if (index == 0) {
			value = _buffer->firstValue;
			timeStamp = _buffer->firstTimeStamp;
			return CC_SUCCESS;
		}
		value += _buffer->valueDiffs[index-1];
		timeStamp += _buffer->timeDiffs[index-1];
		return CC_SUCCESS;
	}

	/* Get the time stamp of the first sample.
	 */
	uint32_t getTimeStart() const {
		return _buffer->firstTimeStamp;
	}

	/* Get the time stamp of the last sample.
	 */
	uint32_t getTimeEnd() const {
		return _buffer->lastTimeStamp;
	}

	/* Add a sample to the current curve
	 * @value the sample to be added
	 * @timeStamp the time stamp of the sample
	 *
	 * Adds the value to the buffer if it is assigned and not full.
	 * If the difference with the previous sample is too large, the buffer is cleared.
	 *
	 * @return CC_SUCCESS on success.
	 */
	CC_ERR_CODE add(const T value, const uint32_t timeStamp=0) {
		if (!_buffer) {
			LOGe("Buffer not assigned!");
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
			int64_t dt = (int64_t)timeStamp - _buffer->lastTimeStamp;
			if (dt > 127 || dt < -127) {
				clear();
				return CC_DIFFERENCE_TOO_LARGE;
			}
			_buffer->timeDiffs[_buffer->length] = dt;

			int32_t diff = (int32_t)value - _buffer->lastValue;
			if (diff > 127 || diff < -127) {
				clear();
				return CC_DIFFERENCE_TOO_LARGE;
			}
			_buffer->valueDiffs[_buffer->length++] = diff;
			_buffer->lastValue = value;
			_buffer->lastTimeStamp = timeStamp;
		}
		return CC_SUCCESS;
	}

	/* Clear the buffer
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

	/* Get number of added samples in the buffer
	 *
	 * @return number of elements stored
	 */
	inline uint16_t length() const { return _buffer->length; }


	/////////// Bufferaccessor ////////////////////////////

	/* @inherit */
	int assign(buffer_ptr_t buffer, uint16_t size) {
		LOGd("assign buff: %p, len: %d", buffer, size);
		if (_max_buf_size > size) {
			LOGe("Assigned buffer is not large enough");
			return 1;
		}
		_buffer = (current_curve_t<T>*)buffer;
		return 0;
	}

	/* @inherit */
	uint16_t getDataLength() const {
//		return CURRENT_CURVE_HEADER_SIZE + _buffer->length-1;
		return CURRENT_CURVE_HEADER_SIZE + 2*_buffer->length-2;
	}

	/* @inherit */
	uint16_t getMaxLength() const {
		//return sizeof(uint16_t) + 2*sizeof(T) + CURRENT_CURVE_MAX_SAMPLES-1;
		return _max_buf_size;
	}

	/* @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};
