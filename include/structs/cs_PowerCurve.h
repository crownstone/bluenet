/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct. 02, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <common/cs_Types.h>
#include "structs/cs_BufferAccessor.h"
#include "drivers/cs_RTC.h"

using namespace BLEpp;

#define POWER_CURVE_HEADER_SIZE                (sizeof(uint16_t) + 4*sizeof(T) + 2*sizeof(uint32_t))

// Make sure that max_samples is divisible by 2
//#define POWER_CURVE_MAX_SAMPLES                ((MASTER_BUFFER_SIZE - POWER_CURVE_HEADER_SIZE + 3) / 4 * 2)
#define POWER_CURVE_MAX_BUF_SIZE               (MASTER_BUFFER_SIZE > BLE_GATTS_VAR_ATTR_LEN_MAX ? BLE_GATTS_FIX_ATTR_LEN_MAX : MASTER_BUFFER_SIZE)
#define POWER_CURVE_MAX_SAMPLES                ((POWER_CURVE_MAX_BUF_SIZE - POWER_CURVE_HEADER_SIZE + 3) / 4 * 2)

#define PC_SUCCESS                               0
#define PC_BUFFER_NOT_INITIALIZED                1
#define PC_BUFFER_NOT_LARGE_ENOUGH               2
#define PC_SAMPLE_DIFFERENCE_TOO_LARGE           3
#define PC_TIME_DIFFERENCE_TOO_LARGE             4
#define PC_WRONG_SAMPLE_TYPE                     5

typedef uint8_t PC_ERR_CODE;

/* Structure for the Power Curve
 *
 * Requires POWER_CURVE_SAMPLES to be set.
 */
template <typename T>
struct __attribute__((__packed__)) power_curve_t {
	uint16_t length;
	T firstCurrent;
	T lastCurrent;
	T firstVoltage;
	T lastVoltage;
	uint32_t firstTimeStamp;
	uint32_t lastTimeStamp;
	int8_t currentDiffs[POWER_CURVE_MAX_SAMPLES/2-1];
	int8_t voltageDiffs[POWER_CURVE_MAX_SAMPLES/2-1];
//	int8_t sampleDiffs[POWER_CURVE_MAX_SAMPLES-2];
	int8_t timeDiffs[POWER_CURVE_MAX_SAMPLES-1];
};

/* Power Curve
 *
 * Class used to send the current and voltage samples over Bluetooth.
 * In order to save space, only differences of the samples are stored.
 * Sampling could have been interrupted by the radio, so we also store timestamps
 */
template <typename T>
class PowerCurve : public BufferAccessor {
private:
	power_curve_t<T>* _buffer;
//	const size_t _item_size = sizeof(T);
//	const size_t _max_buf_size = MASTER_BUFFER_SIZE;
	const size_t _max_buf_size = POWER_CURVE_MAX_BUF_SIZE;

public:
	/* Default constructor
	 *
	 * Constructor does not initialize the buffer.
	 */
	PowerCurve(): _buffer(NULL) {
	};

	/* Release the buffer
	 *
	 * Sets pointer to zero, does not deallocate memory.
	 */
	void release() {
		LOGd("Release power curve. This will screw up SoftDevice if characteristic is not deleted.");
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
		if (_buffer->length/2 >= POWER_CURVE_MAX_SAMPLES/2) {
			return true;
		}
		return false;
	}

	/* Get i-th current and voltage sample, with timestamps
	 * @index Index of the sample you want to get.
	 * @current Value of previous current sample (index-1), this value will be set to the i-th sample.
	 * @voltage Value of previous voltage sample (index-1), this value will be set to the i-th sample.
	 * @currentTimeStamp TimeStamp of previous current sample (index-1), this value will be set to the i-th current time stamp.
	 * @voltageTimeStamp TimeStamp of previous voltage sample (index-1), this value will be set to the i-th voltage time stamp.
	 *
	 * @return PC_SUCCESS on success.
	 */
	PC_ERR_CODE getValue(const uint16_t index, T& current, T& voltage, uint32_t& currentTimeStamp, uint32_t& voltageTimeStamp) const {
		if (index >= length()) {
			return PC_BUFFER_NOT_LARGE_ENOUGH;
		}
		if (index == 0) {
			current = _buffer->firstCurrent;
			voltage = _buffer->firstVoltage;
			currentTimeStamp = _buffer->firstTimeStamp;
			voltageTimeStamp = currentTimeStamp + _buffer->timeDiffs[0];
			return PC_SUCCESS;
		}
//		current += _buffer->sampleDiffs[2*index-2];
//		voltage += _buffer->sampleDiffs[2*index-1];
		current += _buffer->currentDiffs[index-1];
		voltage += _buffer->voltageDiffs[index-1];
		currentTimeStamp = voltageTimeStamp + _buffer->timeDiffs[2*index-1];
		voltageTimeStamp = currentTimeStamp + _buffer->timeDiffs[2*index];
		return PC_SUCCESS;
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

	/* Add a current sample to the power curve
	 * @value the sample to be added
	 * @timeStamp the time stamp of the sample
	 *
	 * Always first add a current sample, then voltage, then current, etc!
	 * Adds the value to the buffer if it is assigned and not full.
	 * If the difference with the previous sample is too large, the buffer is cleared.
	 *
	 * @return PC_SUCCESS on success.
	 */
	PC_ERR_CODE addCurrent(const T value, const uint32_t timeStamp=0) {
		if (!_buffer) {
			LOGe("Buffer not assigned!");
			return PC_BUFFER_NOT_INITIALIZED;
		}
		if (_buffer->length >= POWER_CURVE_MAX_SAMPLES) {
//			LOGe("Buffer is not large enough");
			return PC_BUFFER_NOT_LARGE_ENOUGH;
		}

		if (_buffer->length % 2 == 1) {
			// TODO: Clear buffer?
			return PC_WRONG_SAMPLE_TYPE;
		}

		if (_buffer->length == 0) {
			_buffer->length++;
			_buffer->firstCurrent = value;
			_buffer->lastCurrent = value;
			_buffer->firstTimeStamp = timeStamp;
			_buffer->lastTimeStamp = timeStamp;
			return PC_SUCCESS;
		}
		else {
			uint32_t dt = RTC::difference(timeStamp, _buffer->lastTimeStamp);
//			int64_t dt = (int64_t)timeStamp - _buffer->lastTimeStamp;
//			if (dt > 127 || dt < -127) {
			if (dt > 127) {
				clear();
				return PC_TIME_DIFFERENCE_TOO_LARGE;
			}
			_buffer->timeDiffs[_buffer->length-1] = dt;

			int32_t diff = (int32_t)value - _buffer->lastCurrent;
			if (diff > 127 || diff < -127) {
				clear();
				return PC_SAMPLE_DIFFERENCE_TOO_LARGE;
			}
//			_buffer->sampleDiffs[_buffer->length++ -2] = diff;
			_buffer->currentDiffs[_buffer->length++/2 -1] = diff;
			_buffer->lastCurrent = value;
			_buffer->lastTimeStamp = timeStamp;
			return PC_SUCCESS;
		}
	}

	/* Add a voltage sample to the power curve
	 * @value the sample to be added
	 * @timeStamp the time stamp of the sample
	 *
	 * Always first add a current sample, then voltage, then current, etc!
	 * Adds the value to the buffer if it is assigned and not full.
	 * If the difference with the previous sample is too large, the buffer is cleared.
	 *
	 * @return PC_SUCCESS on success.
	 */
	PC_ERR_CODE addVoltage(const T value, const uint32_t timeStamp=0) {
		if (!_buffer) {
			LOGe("Buffer not assigned!");
			return PC_BUFFER_NOT_INITIALIZED;
		}
		if (_buffer->length >= POWER_CURVE_MAX_SAMPLES) {
//			LOGe("Buffer is not large enough");
			return PC_BUFFER_NOT_LARGE_ENOUGH;
		}

		if (_buffer->length % 2 == 0) {
			// TODO: Clear buffer?
			return PC_WRONG_SAMPLE_TYPE;
		}

		uint32_t dt = RTC::difference(timeStamp, _buffer->lastTimeStamp);
		if (dt > 127) {
			clear();
			return PC_TIME_DIFFERENCE_TOO_LARGE;
		}
		_buffer->timeDiffs[_buffer->length-1] = dt;

		if (_buffer->length == 1) {
			_buffer->length++;
			_buffer->firstVoltage = value;
			_buffer->lastVoltage = value;
			return PC_SUCCESS;
		}
		else {
			int32_t diff = (int32_t)value - _buffer->lastVoltage;
			if (diff > 127 || diff < -127) {
				clear();
				return PC_SAMPLE_DIFFERENCE_TOO_LARGE;
			}
//			_buffer->sampleDiffs[_buffer->length++ -2] = diff;
			_buffer->voltageDiffs[_buffer->length++/2 -1] = diff;
			_buffer->lastVoltage = value;
			_buffer->lastTimeStamp = timeStamp;
			return PC_SUCCESS;
		}
	}





	/* Clear the buffer
	 */
	PC_ERR_CODE clear() {
		if (!_buffer) {
			LOGe("Buffer not initialized!");
			return PC_BUFFER_NOT_INITIALIZED;
		}
		_buffer->firstCurrent = 0;
		_buffer->lastCurrent = 0;
		_buffer->firstVoltage = 0;
		_buffer->lastVoltage = 0;
		_buffer->firstTimeStamp = 0;
		_buffer->lastTimeStamp = 0;
//		memset(_buffer->differences, 0, _capacity * _item_size);
		_buffer->length = 0;
		return PC_SUCCESS;
	}

	/* Get number of added samples in the buffer
	 *
	 * @return number of elements stored
	 */
	inline uint16_t length() const { return _buffer->length/2; }


	/////////// Bufferaccessor ////////////////////////////

	/* @inherit */
	int assign(buffer_ptr_t buffer, uint16_t size) {
		LOGd("assign buff: %p, len: %d", buffer, size);
		if (_max_buf_size > size) {
			LOGe("Assigned buffer is not large enough");
			return 1;
		}
		_buffer = (power_curve_t<T>*)buffer;
		return 0;
	}

	/* @inherit */
	uint16_t getDataLength() const {
		return POWER_CURVE_HEADER_SIZE + 2*_buffer->length-3;
	}

	/* @inherit */
	uint16_t getMaxLength() const {
		return _max_buf_size;
	}

	/* @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};
