/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 1, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <cstdlib>
#include <cfg/cs_Config.h>
#include <util/cs_Error.h>
#include <logging/cs_Logger.h>
#include <structs/cs_PacketsInternal.h>


// The number of channels per buffer.
static const adc_channel_id_t ADC_CHANNEL_COUNT = CS_ADC_NUM_CHANNELS;

// The number of samples for each channel in a buffer.
static const adc_sample_value_id_t ADC_CHANNEL_SAMPLE_COUNT = CS_ADC_NUM_SAMPLES_PER_CHANNEL;

// Total number of samples in a buffer.
static const uint32_t ADC_BUFFER_SAMPLE_COUNT = ADC_CHANNEL_COUNT * ADC_CHANNEL_SAMPLE_COUNT;

// The number of buffers.
static const adc_buffer_id_t ADC_BUFFER_COUNT = CS_ADC_NUM_BUFFERS;

/**
 * Class that keeps up the buffers used for ADC.
 *
 * - Allocates the buffers.
 * - Abstracts away the interleaved nature of the ADC samples.
 */
class AdcBuffer {
private:

	adc_buffer_t* _buf[ADC_BUFFER_COUNT] = { nullptr };
	bool _allocated = false;

	AdcBuffer() {};
	AdcBuffer(AdcBuffer const&) {};

public:
	/**
	 * Singleton implementation. There is no foreseen need to have multiple objects instantiated.
	 * Note that this singleton implementation does not have dynamic memory allocation.
	 */
	static AdcBuffer& getInstance() {
		static AdcBuffer instance;
		return instance;
	}

	/**
	 * Allocate the buffers.
	 */
	cs_ret_code_t init() {
		if (_allocated) {
			return ERR_SUCCESS;
		}
		for (adc_buffer_id_t i = 0; i < ADC_BUFFER_COUNT; ++i) {
			adc_buffer_t* buf = (adc_buffer_t*) malloc(sizeof(adc_buffer_t));

//			adc_sample_value_t* buf = (adc_sample_value_t*) malloc(sizeof(adc_sample_value_t) * ADC_BUFFER_SAMPLE_COUNT);
			if (buf == nullptr) {
				LOGw("No space to allocate buf");
				// TODO: free all buffers?
				return ERR_NO_SPACE;
			}
			buf->samples = (adc_sample_value_t*) malloc(sizeof(adc_sample_value_t) * ADC_BUFFER_SAMPLE_COUNT);
			if (buf->samples == nullptr) {
				LOGw("No space to allocate samples buf");
				// TODO: free all buffers?
				return ERR_NO_SPACE;
			}
			LOGd("Allocate buffer %i = %p", i, buf->samples);
			_buf[i] = buf;
		}
		_allocated = true;
		return ERR_SUCCESS;
	}

	/**
	 * Get buffer with given id.
	 */
	adc_buffer_t* getBuffer(adc_buffer_id_t buffer_id) {
//		assert(buffer_id < getBufferCount(), "ADC has fewer buffers allocated");
		return _buf[buffer_id];
	}

#ifdef HOST_TARGET
	/**
	 * Set the buffer at the particular buffer_id by just writing a pointer to it. The code calling
	 * this function is responsible for setting the right pointer. No checks w.r.t. this pointer are
	 * performed.
	 */
	void setBuffer(adc_buffer_id_t buffer_id, adc_buffer_t* ptr) {
		_buf[buffer_id] = ptr;
	}
#endif

	/**
	 * Get total number of samples in a buffer.
	 */
	static inline constexpr adc_sample_value_id_t getBufferLength() {
		return ADC_BUFFER_SAMPLE_COUNT;
	}

	/**
	 * Get number of samples for each channel in a buffer.
	 */
	static inline constexpr adc_sample_value_id_t getChannelLength() {
		return ADC_CHANNEL_SAMPLE_COUNT;
	}

	/**
	 * Get number of channels per buffer.
	 */
	static inline constexpr adc_channel_id_t getChannelCount() {
		return ADC_CHANNEL_COUNT;
	}

	/**
	 * Get number of buffers.
	 */
	static inline constexpr adc_buffer_id_t getBufferCount() {
		return ADC_BUFFER_COUNT;
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
	adc_sample_value_t getValue(adc_buffer_id_t buffer_id, adc_channel_id_t channel_id, adc_sample_value_id_t value_id) {
		assert(value_id >= 0 && value_id < getChannelLength(), "Invalid value id");

		adc_sample_value_id_t value_id_in_channel = value_id * getChannelCount() + channel_id;
		adc_sample_value_t* buf = getBuffer(buffer_id)->samples;
		adc_sample_value_t value = buf[value_id_in_channel];
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
	void setValue(adc_buffer_id_t buffer_id, adc_channel_id_t channel_id, adc_sample_value_id_t value_id, adc_sample_value_t value) {
		assert(value_id >= 0, "value id should be positive");
		assert(value_id < getChannelLength(), "value id should be smaller than channel size");

		adc_sample_value_id_t value_id_in_channel = value_id * getChannelCount() + channel_id;
		adc_sample_value_t* buf = getBuffer(buffer_id)->samples;
		buf[value_id_in_channel] = value;
	}

};
