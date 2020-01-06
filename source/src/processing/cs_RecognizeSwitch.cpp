/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 27, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_RecognizeSwitch.h"
#include "cfg/cs_Config.h"

// Define to print debug
#define PRINT_DEBUG

RecognizeSwitch::RecognizeSwitch() :
	_running(false),
	_thresholdDifferent(SWITCHCRAFT_THRESHOLD),
	_thresholdSimilar(SWITCHCRAFT_THRESHOLD),
	_thresholdRatio(100)
{

}

void RecognizeSwitch::init() {
}

void RecognizeSwitch::deinit() {
}

void RecognizeSwitch::configure(float threshold) {
	_thresholdDifferent = threshold;
	_thresholdSimilar = threshold;
	LOGd("config: diff=%i similar=%i ratio=%i", (int)_thresholdDifferent, (int)_thresholdSimilar, (int)_thresholdRatio);
}

void RecognizeSwitch::start() {
	_running = true;
	_skipSwitchDetectionTriggers = 200;
}

void RecognizeSwitch::stop() {
	_running = false;
}

void RecognizeSwitch::skip(uint16_t num) {
	_skipSwitchDetectionTriggers = num;
}


/**
 * The recognizeSwitch function goes through a sequence of buffers to detect if a switch event happened in the buffer
 * in the middle. It uses the InterleavedBuffer to get the previous buffers.
 */
bool RecognizeSwitch::detect(buffer_id_t bufIndex, channel_id_t voltageChannelId) {
	bool result = false;
	if (!_running) {
		return result;
	}
	if (_skipSwitchDetectionTriggers > 0) {
		_skipSwitchDetectionTriggers--;
		return result;
	}

	// The previous buffer, "current" and next (future) buffer are available.
	// buffer0 is the previous buffer
	// buffer1 is the current buffer (bufIndex)
	// buffer2 is the next buffer
	InterleavedBuffer & ib = InterleavedBuffer::getInstance();

	// Check only part of the buffer length (half buffer length).
	// Then repeat that at different parts of the buffer (start, mid, end).
	ext_value_id_t checkLength = ib.getChannelLength() / 2;
	ext_value_id_t startInd;
	ext_value_id_t endInd;

	float value0, value1, value2; // Value of buffer0, buffer1, buffer2
	float diff01, diff12, diff02;
	float diffSum01, diffSum12, diffSum02; // Difference between buf0 and buf1, between buf1 and buf2, between buf0 and buf2
	for (int shift = 0; shift < ib.getChannelLength(); shift += checkLength / 2) {
		diffSum01 = 0;
		diffSum12 = 0;
		diffSum02 = 0;
		startInd = -ib.getChannelLength() + shift;
		endInd = startInd + checkLength;
		for (int i = startInd; i < endInd; ++i) {
			value0 = ib.getValue(bufIndex, voltageChannelId, i);
			value1 = ib.getValue(bufIndex, voltageChannelId, i + ib.getChannelLength());
			value2 = ib.getValue(bufIndex, voltageChannelId, i + 2 * ib.getChannelLength());
			diff01 = (value0 - value1) * (value0 - value1);
			diff12 = (value1 - value2) * (value1 - value2);
			diff02 = (value0 - value2) * (value0 - value2);
			diffSum01 += diff01;
			diffSum12 += diff12;
			diffSum02 += diff02;
		}
//		LOGd("%f %f %f", diffSum01, diffSum12, diffSum02);
		if (diffSum01 > _thresholdDifferent && diffSum12 > _thresholdDifferent) {
			float minDiffSum = diffSum01 < diffSum12 ? diffSum01 : diffSum12;
			if (diffSum02 < _thresholdSimilar || minDiffSum / diffSum02 > _thresholdRatio) {
				result = true;
#ifdef PRINT_DEBUG
				LOGd("Found switch: %i %i %i %i", (int)diffSum01, (int)diffSum12, (int)diffSum02, (int)(minDiffSum / diffSum02));
#endif
				break;
			}
		}
//		else if (diffSum01 > diffSum02 && diffSum12 > diffSum02 && diffSum01 > _thresholdDifferent) {
//			LOGd("%f %f %f", diffSum01, diffSum12, diffSum02);
//		}
	}
	if (result) {
		_skipSwitchDetectionTriggers = 5;
	}

	return result;
}
