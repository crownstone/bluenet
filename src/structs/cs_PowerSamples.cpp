/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jun 2, 2016
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <cstdlib>
#include "structs/cs_PowerSamples.h"
#include <cfg/cs_Strings.h>

//#define PRINT_VERBOSE

PowerSamples::PowerSamples():
	_buffer(NULL),
	_currentBuffer(POWER_SAMPLE_BURST_NUM_SAMPLES),
	_voltageBuffer(POWER_SAMPLE_BURST_NUM_SAMPLES),
	_currentTimestampsBuffer(POWER_SAMPLE_BURST_NUM_SAMPLES),
	_voltageTimestampsBuffer(POWER_SAMPLE_BURST_NUM_SAMPLES),
	_allocatedSelf(false)
{

};


bool PowerSamples::init() {

	//! Allocate memory
	_buffer = (power_samples_t*)calloc(1, sizeof(power_samples_t));
	if (_buffer == NULL) {
		LOGw(STR_ERR_ALLOCATE_MEMORY);
		return false;
	}

#ifdef PRINT_VERBOSE
	LOGd(FMT_ALLOCATE_MEMORY, _buffer);
#endif

	_allocatedSelf = true;

	_currentBuffer.assign((buffer_ptr_t)&_buffer->_currentSamples, sizeof(_buffer->_currentSamples));
	_voltageBuffer.assign((buffer_ptr_t)&_buffer->_voltageSamples, sizeof(_buffer->_voltageSamples));
	_currentTimestampsBuffer.assign((buffer_ptr_t)&_buffer->_currentTimestamps, sizeof(_buffer->_currentTimestamps));
	_voltageTimestampsBuffer.assign((buffer_ptr_t)&_buffer->_voltageTimestamps, sizeof(_buffer->_voltageTimestamps));

	//! Also call clear to make sure we start with a clean buffer
	clear();
	return true;
}


bool PowerSamples::deinit() {
	if (_buffer != NULL && _allocatedSelf) {
		free(_buffer);
	}
	_allocatedSelf = false;
	_buffer = NULL;

	_currentBuffer.release();
	_voltageBuffer.release();
	_currentTimestampsBuffer.release();
	_voltageTimestampsBuffer.release();
	return true;
}

void PowerSamples::clear() {
	_currentBuffer.clear();
	_voltageBuffer.clear();
	_currentTimestampsBuffer.clear();
	_voltageTimestampsBuffer.clear();
}

uint16_t PowerSamples::size() {
	if (_currentBuffer.size() > _voltageBuffer.size()) {
		return _voltageBuffer.size();
	}
	return _currentBuffer.size();
}

bool PowerSamples::full() {
	// TODO: should be OR or AND?
	return _currentBuffer.full() || \
			_voltageBuffer.full() || \
			_currentTimestampsBuffer.full() || \
			_voltageTimestampsBuffer.full();
}


///////////! Bufferaccessor ////////////////////////////
/** @inherit */
int PowerSamples::assign(buffer_ptr_t buffer, uint16_t size) {
	if (getMaxLength() > size) {
		LOGe(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		return 1;
	}

#ifdef PRINT_VERBOSE
	LOGd(FMT_ASSIGN_BUFFER_LEN, buffer, size);
#endif

	_buffer = (power_samples_t*)buffer;

	_currentBuffer.assign((buffer_ptr_t)&_buffer->_currentSamples, sizeof(_buffer->_currentSamples));
	_voltageBuffer.assign((buffer_ptr_t)&_buffer->_voltageSamples, sizeof(_buffer->_voltageSamples));
	_currentTimestampsBuffer.assign((buffer_ptr_t)&_buffer->_currentTimestamps, sizeof(_buffer->_currentTimestamps));
	_voltageTimestampsBuffer.assign((buffer_ptr_t)&_buffer->_voltageTimestamps, sizeof(_buffer->_voltageTimestamps));

	return 0;
}
