/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 27, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/cs_RecognizeSwitch.h>

RecognizeSwitch::RecognizeSwitch() {
	int consecutive_samples = 20; // 20 and 450 for threshold works a bit
	_circBuffer = new CircularBuffer<int16_t>(consecutive_samples);
	_threshold = 200;
}

void RecognizeSwitch::init() {
	// Skip the first N cycles
	_skipSwitchDetectionTriggers = 200;
	_circBuffer->init();
}

void RecognizeSwitch::deinit() {
	_circBuffer->deinit();
}

//#define VERBOSE_SWITCH

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
	int32_t vdifftot = 0;
	int16_t last = 0, vdiff = 0, value0 = 0, value1 = 0;
	bool full = false;
	int j = 0; 
	//int k = -ib.getChannelLength() - 10;
	for (int i = -ib.getChannelLength() + 1; i < ib.getChannelLength(); ++i) {
		value0 = ib.getValue(bufIndex, voltageChannelId, i);
		value1 = ib.getValue(bufIndex, voltageChannelId, i - 1);
		vdiff = abs(value0 - value1);

		// it would be convenient to remove only the maximum (extremum)
		// or even better, to have a single spike as well would be a good indication of a switch event
		// skip extremum for now...
		if (vdiff > 150) {
	//		k = i;
			// something is wrong... vdiff should be > 150 anytime, must be error in getValue(...)
		//	LOGd("cnt %i, value0 %i, value1 %i | new %i, old %i", i, value0, value1, vdiff, last);	
			continue;
		}

		if (_circBuffer->full()) {
			full = true;
		}

		if (full) {
			last = _circBuffer->pop();
			vdifftot -= last;
		} 
		vdifftot += vdiff;
		_circBuffer->push(vdiff);
		
		if (full) {
			// now also use some spike condition
			if (vdifftot < _threshold) {
				if (!condition0) {
					j = i;
					condition0 = true;
				}
			}
		}
#ifdef VERBOSE_SWITCH
		if (condition0) {
			LOGd("cnt %i, value0 %i, value1 %i | new %i, old %i | vdifftot %i", i, value0, value1, vdiff, last, vdifftot);	
		}
#endif 
		if (condition0) {
			if (i> j + 10) break;
		}
	}

	if (condition0) {
#ifdef VERBOSE_SWITCH
		/*
		_circBuffer->clear();
		vdifftot = 0; last = 0; vdiff = 0; value0 = 0; value1 = 0; full = false;
		for (int i = -ib.getChannelLength() + 1; i < ib.getChannelLength(); ++i) {
			value0 = ib.getValue(bufIndex, voltageChannelId, i);
			value1 = ib.getValue(bufIndex, voltageChannelId, i - 1);
			vdiff = abs(value0 - value1);
			if (vdiff > 500) continue;
			if (_circBuffer->full()) {
				full = true;
			}

			if (full) {
				last = _circBuffer->pop();
				vdifftot -= last;
			} 
			vdifftot += vdiff;
			_circBuffer->push(vdiff);
			
			LOGd("cnt %i, value0 %i, value1 %i | new %i, old %i | vdifftot %i", i, value0, value1, vdiff, last, vdifftot);	
		}*/
		LOGd("State switch recognized at step %i", j);
#endif

		_skipSwitchDetectionTriggers = 25;
		result = true;
	}

	_circBuffer->clear();
	
	// second attempt, just comparing curves might be better after all...
//	int check_point = -ib.getChannelLength();
	int16_t vdiff01 = 0, vdiff02 = 0, vdiff12 = 0, value2 = 0;
	int32_t vdiff01tot = 0, vdiff02tot = 0, vdiff12tot = 0;
	// We do not yet obtain the values in the right way...
	for (int i = -ib.getChannelLength(); i < 0; ++i) {
		value0 = ib.getValue(bufIndex, voltageChannelId, i);
		value1 = ib.getValue(bufIndex, voltageChannelId, i + ib.getChannelLength());
		value2 = ib.getValue(bufIndex, voltageChannelId, i + 2 * ib.getChannelLength());

//		if (i == check_point) {
//			LOGd("01: %i", value0);
//		}

		vdiff01 = std::abs(value0 - value1);
		vdiff02 = std::abs(value0 - value2);
		vdiff12 = std::abs(value1 - value2);
		vdiff01tot += vdiff01;
		vdiff02tot += vdiff02;
		vdiff12tot += vdiff12;
//		if (i == check_point) {
//			LOGd("Diff 01: %i", vdiff01);
//			LOGd("Diff 02: %i", vdiff02);
//		}
	}
//	LOGd("Tot 01: %li", vdiff01tot);
//	LOGd("Tot 02: %li", vdiff02tot);
//	LOGd("Tot 12: %li", vdiff12tot);

	bool condition1 = false;
	if (vdiff01tot > 1000 && vdiff12tot > 1000 && vdiff02tot < 1000) {
		condition1 = true;
	}
	if (vdiff01tot > 10000 && vdiff12tot > 10000) {
		condition1 = true;
	}

	if (condition1) {
		LOGd("State switch recognized at step %i", j);

		_skipSwitchDetectionTriggers = 25;
		result = true;
	}

	return result;
}



