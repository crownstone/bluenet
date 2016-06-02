/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 30, 2016
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "common/cs_Types.h"
#include "cfg/cs_Config.h"
#include "structs/buffer/cs_StackBuffer.h"
#include "structs/buffer/cs_DifferentialBuffer.h"
#include "structs/cs_BufferAccessor.h"

struct __attribute__((__packed__)) power_samples_t {
	stack_buffer_fixed_t<uint16_t, POWER_SAMPLE_BURST_NUM_SAMPLES> _currentSamples;
	stack_buffer_fixed_t<uint16_t, POWER_SAMPLE_BURST_NUM_SAMPLES> _voltageSamples;
	differential_buffer_fixed_t<uint32_t, POWER_SAMPLE_BURST_NUM_SAMPLES> _currentTimestamps;
	differential_buffer_fixed_t<uint32_t, POWER_SAMPLE_BURST_NUM_SAMPLES> _voltageTimestamps;
};

class PowerSamples : public BufferAccessor {
private:
	power_samples_t* _buffer;
	StackBuffer<uint16_t> _currentBuffer;
	StackBuffer<uint16_t> _voltageBuffer;
	DifferentialBuffer<uint32_t> _currentTimestampsBuffer;
	DifferentialBuffer<uint32_t> _voltageTimestampsBuffer;
	bool _allocatedSelf;

public:
	PowerSamples();

	bool init();

	bool deinit();

	void clear();

	uint16_t size();

	bool full();

	StackBuffer<uint16_t>* getCurrentSamplesBuffer() {
		return &_currentBuffer;
	}

	StackBuffer<uint16_t>* getVoltageSamplesBuffer() {
		return &_voltageBuffer;
	}

	DifferentialBuffer<uint32_t>* getCurrentTimestampsBuffer() {
		return &_currentTimestampsBuffer;
	}

	DifferentialBuffer<uint32_t>* getVoltageTimestampsBuffer() {
		return &_voltageTimestampsBuffer;
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
