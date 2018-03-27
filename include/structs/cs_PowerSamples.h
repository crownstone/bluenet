/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 30, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "common/cs_Types.h"
#include "cfg/cs_Config.h"
#include "structs/buffer/cs_StackBuffer.h"
#include "structs/buffer/cs_DifferentialBuffer.h"
#include "structs/cs_BufferAccessor.h"

struct __attribute__((__packed__)) power_samples_t {
	stack_buffer_fixed_t<int16_t, POWER_SAMPLE_BURST_NUM_SAMPLES> _currentSamples;
	stack_buffer_fixed_t<int16_t, POWER_SAMPLE_BURST_NUM_SAMPLES> _voltageSamples;
	uint32_t _timestamp; //! Timestamp of first sample.
};

class PowerSamples : public BufferAccessor {
private:
	power_samples_t* _buffer;
	StackBuffer<int16_t> _currentBuffer;
	StackBuffer<int16_t> _voltageBuffer;
	bool _allocatedSelf;

public:
	PowerSamples();

	bool init();

	bool deinit();

	void clear();

	uint16_t size();

	bool full();

	StackBuffer<int16_t>* getCurrentSamplesBuffer() {
		return &_currentBuffer;
	}

	StackBuffer<int16_t>* getVoltageSamplesBuffer() {
		return &_voltageBuffer;
	}

	void setTimestamp(uint32_t timestamp) {
		_buffer->_timestamp = timestamp;
	}

	uint32_t getTimestamp() {
		return _buffer->_timestamp;
	}

	///////////! Bufferaccessor ////////////////////////////
	/** @inherit */
	int assign(buffer_ptr_t buffer, uint16_t size);

	/** @inherit */
	uint16_t getDataLength() const {
		return getMaxLength();
	}

	/** @inherit */
	uint16_t getMaxLength() const {
		return sizeof(power_samples_t);
	}

	/** @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}
};
