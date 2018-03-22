/*
 * Author: Anne van Rossum
 * Copyright: Crownstone B.V.
 * Date: Mar 1, 2018
 * License: LGPLv3+, MIT, Apache
 */
#pragma once

#include <cstdlib>

#include <cfg/cs_Config.h>
#include <nrf.h>  

// The index type for a buffer is just a minimal-sized unsigned char as offset into an array.
typedef uint8_t buffer_id_t;

// The index type for a channel (a buffer within one of the buffers), organized like [A B A B A B]
typedef uint8_t channel_id_t;

// The index type for a value can be negative to support padding.
typedef int8_t value_id_t;

// The value type
typedef nrf_saadc_value_t value_t;

// The number of buffers is to be defined statically
static const uint8_t INTERLEAVED_BUFFER_COUNT = CS_ADC_NUM_BUFFERS;

// The length of each individual buffer is also defined statically
static const value_t INTERLEAVED_BUFFER_LENGTH = CS_ADC_BUF_SIZE;

// The number of channels
static const uint8_t CHANNEL_COUNT = 2;

/** Interleaved Buffer implementation
 *
 */
class InterleavedBuffer {
private:

	nrf_saadc_value_t* _buf[INTERLEAVED_BUFFER_COUNT];

public:
	/**
	 * Singleton implementation. There is no foreseen need to have multiple InterleavedBuffer objects instantiated.
	 * Note that this singleton implementation does not have dynamic memory allocation.
	 */
	static InterleavedBuffer& getInstance() {
		static InterleavedBuffer instance;
		return instance;
	}

	/**
	 * From the INTERLEAVED_BUFFER_COUNT pick the one with the given buffer_id.
	 */
	nrf_saadc_value_t* getBuffer(buffer_id_t buffer_id) {
		assert (buffer_id < INTERLEAVED_BUFFER_COUNT, "ADC has fewer buffers allocated");
		return _buf[buffer_id];
	}

	/**
	 * Set the buffer at the particular buffer_id by just writing a pointer to it. The code calling this function is
	 * responsible for setting the right pointer. No checks w.r.t. this pointer are performed.
	 */
	void setBuffer(buffer_id_t buffer_id, nrf_saadc_value_t* ptr) {
		assert (buffer_id < INTERLEAVED_BUFFER_COUNT, "ADC has fewer buffers allocated");
		_buf[buffer_id] = ptr;
	}

	/**
	 * Expose buffer lengths to accessors.
	 */
	inline const value_t getBufferLength() {
		return INTERLEAVED_BUFFER_LENGTH;
	}

	/**
	 * Given a pointer to a buffer return the buffer_id.
	 */
	cs_adc_buffer_id_t getIndex(nrf_saadc_value_t *buffer) {
		for (cs_adc_buffer_id_t i = 0; i < INTERLEAVED_BUFFER_COUNT; ++i) {
			if (buffer == _buf[i]) 
				return i;
		}
		return INTERLEAVED_BUFFER_COUNT;
	}

	/**
	 * Clear the pointer to a buffer.
	 */
	void clearBuffer(buffer_id_t buffer_id) {
		setBuffer(buffer_id, NULL);
	}

	/**
	 * Uses getIndex to find out if this pointer exists. 
	 */
	bool exists(nrf_saadc_value_t *buffer) {
		return (getIndex(buffer) != INTERLEAVED_BUFFER_COUNT);
	}

	/**
	 * Checks if the pointer is set.
	 */
	bool exists(buffer_id_t buffer_id) {
		return (_buf[buffer_id] != NULL);
	}

	/**
	 * Get the previous buffer index. This will just use the total static buffer count and use a modulus operation to 
	 * wrap around the first buffer.
	 *
	 * @param[in] buffer_id                      Index to the buffer (0 up to INTERLEAVED_BUFFER_COUNT - 1)
	 * @return                                   Index to the previous buffer
	 */
	inline buffer_id_t getPrevious(buffer_id_t buffer_id) {
		return (buffer_id + INTERLEAVED_BUFFER_COUNT - 1) % INTERLEAVED_BUFFER_COUNT;
	}

	/**
	 * Get the next buffer index. This will just use the total static buffer count and use a modulus operation to 
	 * wrap around the first buffer.
	 *
	 * @param[in] buffer_id                      Index to the buffer (0 up to INTERLEAVED_BUFFER_COUNT - 1)
	 * @return                                   Index to the next buffer
	 */
	inline buffer_id_t getNext(buffer_id_t buffer_id) {
		return (buffer_id + 1) % INTERLEAVED_BUFFER_COUNT;
	}

	/**
	 * Get a particular value from a buffer. We allow some "magic" behavior here. The value_id is a signed value. If it
	 * is negative it retrieves a value from the previous buffer. If it is larger than INTERLEAVED_BUFFER_LENGTH - 1 it
	 * retrieves a value from the next buffer.
	 *
	 * @param[in] buffer_id                      Index to the buffer (0 up to INTERLEAVED_BUFFER_COUNT - 1)
	 * @param[in] channel_id                     Particular channel within this buffer (0 or 1)
	 * @param[in] value_id                       Index to the value in the buffer
	 * @return                                   Particular value
	 */
	nrf_saadc_value_t getValue(buffer_id_t buffer_id, channel_id_t channel_id, value_id_t value_id) {
		value_t value;
		nrf_saadc_value_t* buf;
		if (value_id < 0) {
			// use previous buffer, e.g. for padding
			
			buffer_id_t prev_buffer_id = getPrevious(buffer_id);
			bool buffer_exists = exists(prev_buffer_id);
			assert (buffer_exists, "Previous buffer does not exist!");

			// a value such as -1 will be multiplied by 2, hence -2 and then used to count from the back of the 
			// previous buffer, say of length 100, value_id = -1 will hence retrieve the last item from the
			// previous buffer, prev_buffer[98] or prev_buffer[99] depending on channel_id
			value_id_t prev_value_id = INTERLEAVED_BUFFER_LENGTH + (value_id * CHANNEL_COUNT) + channel_id;
			buf = getBuffer(prev_buffer_id);
			value = buf[prev_value_id];

		} else if (value_id >= INTERLEAVED_BUFFER_LENGTH) {
			// use next buffer, e.g. for padding
			
			buffer_id_t next_buffer_id = getNext(buffer_id);
			bool buffer_exists = exists(next_buffer_id);
			assert (buffer_exists, "Next buffer does not exist!");

			// a value such as 101 will be subtracted by the buffer length, say 100, hence result is 1, then
			// the next buffer will be queried at next_buffer[2] or next_buffer[3] dependin on channel_id
			value_id_t next_value_id = (value_id - INTERLEAVED_BUFFER_LENGTH) * CHANNEL_COUNT + channel_id;
			buf = getBuffer(next_buffer_id);
			value = buf[next_value_id];

		} else {
			// return value in given buffer
			value_id_t value_id = value_id * CHANNEL_COUNT + channel_id;
			buf = getBuffer(buffer_id);
			value = buf[value_id];
		}

		return value;
	}

};
