/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 27, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/cs_RecognizeSwitch.h>

RecognizeSwitch::RecognizeSwitch() {
	// Skip the first N cycles
	_skipSwitchDetectionTriggers = 200;

	_threshold = 15;

	_circBuffer = new CircularBuffer<int16_t>(15); 
}

#define VERBOSE_SWITCH

/**
 * The recognizeSwitch function goes through a sequence of buffers to detect if a switch event happened in the buffer
 * in the middle. It uses the InterleavedBuffer to get the previous buffers.
 */
bool RecognizeSwitch::detect(buffer_id_t bufIndex, channel_id_t voltageChannelId) {
	bool result = false;
	if (_skipSwitchDetectionTriggers > 0) {
		_skipSwitchDetectionTriggers--;
		return result;
	}

	InterleavedBuffer & ib = InterleavedBuffer::getInstance();

	bool condition0 = false;

	// calculate moving absolute average/sum, this should be close to zero, assume window size still keeps it
	// limited to a int16_t w.r.t. size
	int16_t vdifftot = 0;
	bool full = false;
	for (int i = -ib.getChannelLength() + 1; i < ib.getChannelLength(); ++i) {
		int16_t vdiff = abs(
				ib.getValue(bufIndex, voltageChannelId, i    ) - 
				ib.getValue(bufIndex, voltageChannelId, i - 1) );

		if (!full && (_circBuffer->size() == _circBuffer->capacity())) {
			full = true;
		}

		if (full) {
			int16_t last = _circBuffer->pop();
			vdifftot -= last;
		} 
		vdifftot += vdiff;
		_circBuffer->push(vdiff);

		if (full) {
			if (vdifftot < _threshold) {
#ifdef VERBOSE_SWITCH
				LOGd("Threshold: %i", _threshold);	
#endif 
				condition0 = true;
			}
		}
	}

	if (condition0) {
#ifdef VERBOSE_SWITCH
		LOGd("State switch recognized");
#endif
		_skipSwitchDetectionTriggers = 50;
		result = true;
	}
	return result;
}



