/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 1, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdlib>
//#include <nrf.h>
#include <cfg/cs_Config.h>
#include <util/cs_Error.h>
#include <drivers/cs_Serial.h>

// The index type for a buffer is just a minimal-sized unsigned char as offset into an array.
typedef uint8_t buffer_id_t;

// The index type for a channel (a buffer within one of the buffers), organized like [A B A B A B]
typedef uint8_t channel_id_t;

// The index type for a value is positive
typedef uint16_t sample_value_id_t;

// The sample value type.
typedef int16_t sample_value_t;

// The number of buffers is to be defined statically
static const uint8_t INTERLEAVED_BUFFER_COUNT = CS_ADC_NUM_BUFFERS;

// The length of the complete buffer (including both channels) is also defined statically
static const sample_value_id_t INTERLEAVED_BUFFER_LENGTH = CS_ADC_BUF_SIZE;

#if CS_ADC_BUF_SIZE % 2 == 1
#error "Buffer size needs to be even with current channel count"
#endif

// The number of channels
static const channel_id_t INTERLEAVED_CHANNEL_COUNT = 2;

// The length of an individual channel within a buffer
static const sample_value_id_t INTERLEAVED_CHANNEL_LENGTH = INTERLEAVED_BUFFER_LENGTH / INTERLEAVED_CHANNEL_COUNT;

/** Interleaved Buffer implementation
 *
 */
class InterleavedBuffer {
private:

	sample_value_t* _buf[INTERLEAVED_BUFFER_COUNT];

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
	 * From the _buf array pick the one with the given buffer_id.
	 */
	sample_value_t* getBuffer(buffer_id_t buffer_id) {
//		assert(buffer_id < getBufferCount(), "ADC has fewer buffers allocated");
		return _buf[buffer_id];
	}

	/**
	 * Set the buffer at the particular buffer_id by just writing a pointer to it. The code calling this function is
	 * responsible for setting the right pointer. No checks w.r.t. this pointer are performed.
	 */
	void setBuffer(buffer_id_t buffer_id, sample_value_t* ptr) {
//		assert(buffer_id < getBufferCount(), "ADC has fewer buffers allocated");
		_buf[buffer_id] = ptr;
	}

	/**
	 * Expose buffer lengths to accessors.
	 */
	static inline constexpr sample_value_id_t getBufferLength() {
		return INTERLEAVED_BUFFER_LENGTH;
	}

	/**
	 * Get number of buffers.
	 */
	static inline constexpr buffer_id_t getBufferCount() {
		return INTERLEAVED_BUFFER_COUNT;
	}

	/**
	 * Expose channel lengths to accessors.
	 */
	static inline constexpr sample_value_id_t getChannelLength() {
		return INTERLEAVED_CHANNEL_LENGTH;
	}

	/**
	 * Get number of channels.
	 */
	static inline constexpr channel_id_t getChannelCount() {
		return INTERLEAVED_CHANNEL_COUNT;
	}

	/**
	 * Given a pointer to a buffer return the buffer_id.
	 */
	buffer_id_t getIndex(sample_value_t *buffer) {
		for (buffer_id_t i = 0; i < getBufferCount(); ++i) {
			if (buffer == _buf[i])
				return i;
		}
		return getBufferCount();
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
	bool exists(sample_value_t *buffer) {
		return (getIndex(buffer) != getBufferCount());
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
	 * @param[in] buffer_id                      Index to the buffer (0 up to getBufferCount() - 1)
	 * @return                                   Index to the previous buffer
	 */
	inline buffer_id_t getPrevious(buffer_id_t buffer_id, const uint8_t shift = 1) {
		return (buffer_id + getBufferCount() - shift) % getBufferCount();
	}

	/**
	 * Get the next buffer index. This will just use the total static buffer count and use a modulus operation to
	 * wrap around the first buffer.
	 *
	 * @param[in] buffer_id                      Index to the buffer (0 up to getBufferCount() - 1)
	 * @return                                   Index to the next buffer
	 */
	inline buffer_id_t getNext(buffer_id_t buffer_id, const uint8_t shift = 1) {
		return (buffer_id + shift) % getBufferCount();
	}

	/**
	 * Get a particular value from a buffer.
	 *
	 * The value_id is a reference to the index of a value within a channel (half of the buffer length).
	 *
	 * @param[in] buffer_id                      Index to the buffer (0 up to getBufferCount() - 1)
	 * @param[in] channel_id                     Particular channel within this buffer (0 or 1)
	 * @param[in] value_id                       Index to the value in the buffer (0 up to getChannelLength() - 1)
	 * @return                                   Particular value
	 */
	sample_value_t getValue(buffer_id_t buffer_id, channel_id_t channel_id, sample_value_id_t value_id) {
		assert(value_id >= 0 && value_id < getChannelLength(), "Invalid value id");
		sample_value_t value;
		sample_value_t* buf;

		sample_value_id_t value_id_in_channel = value_id * getChannelCount() + channel_id;
		buf = getBuffer(buffer_id);
		value = buf[value_id_in_channel];

		return value;
	}

	/**
	 * For in-place filtering it is necessary to write a particular value into the buffer.
	 *
	 * @param[in] buffer_id                      Index to the buffer (0 up to getBufferCount() - 1)
	 * @param[in] channel_id                     Particular channel within this buffer (0 or 1)
	 * @param[in] value_id                       Index to the value in the buffer
	 * @param[in] value                          Value to be written to the buffer
	 */
	void setValue(buffer_id_t buffer_id, channel_id_t channel_id, sample_value_id_t value_id, sample_value_t value) {
		assert(value_id >= 0, "value id should be positive");
		assert(value_id < getChannelLength(), "value id should be smaller than channel size");
		sample_value_t* buf;
		sample_value_id_t value_id_in_channel = value_id * getChannelCount() + channel_id;
		buf = getBuffer(buffer_id);
		buf[value_id_in_channel] = value;
	}

};
