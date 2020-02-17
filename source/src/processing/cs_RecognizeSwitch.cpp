/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 27, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_RecognizeSwitch.h"
#include "cfg/cs_Config.h"

RecognizeSwitch::RecognizeSwitch()
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
bool RecognizeSwitch::detect(buffer_id_t currentBufIndex, channel_id_t voltageChannelId) {
	bool result = false;
	if (!_running) {
		return result;
	}
	if (_skipSwitchDetectionTriggers > 0) {
		_skipSwitchDetectionTriggers--;
		return result;
	}

	InterleavedBuffer & ib = InterleavedBuffer::getInstance();

	// Check only part of the buffer length (half buffer length).
	// Then repeat that at different parts of the buffer (start, mid, end).
	// Example: if channel length = 100, then check 0-49, 25-74, and 50-99.
	ext_value_id_t checkLength = ib.getChannelLength() / 2;
	ext_value_id_t shift = checkLength / 2;
	ext_value_id_t startInd;
	ext_value_id_t endInd;

	buffer_id_t bufIndex0 = ib.getPrevious(currentBufIndex, 2);;
	buffer_id_t bufIndex1 = ib.getPrevious(currentBufIndex, 1);
	buffer_id_t bufIndex2 = currentBufIndex;
	LOGnone("buf ind=%u %u %u", bufIndex0, bufIndex1, bufIndex2);

	float value0, value1, value2; // Value of buffer0, buffer1, buffer2
	float diff01, diff12, diff02;
	float diffSum01, diffSum12, diffSum02; // Difference between buf0 and buf1, between buf1 and buf2, between buf0 and buf2
	for (startInd = 0; startInd < (ib.getChannelLength() - shift); startInd += shift) {
		diffSum01 = 0;
		diffSum12 = 0;
		diffSum02 = 0;
		endInd = startInd + checkLength;
		LOGnone("start=%i end=%i", startInd, endInd);
		for (int i = startInd; i < endInd; ++i) {
			value0 = ib.getValue(bufIndex0, voltageChannelId, i);
			value1 = ib.getValue(bufIndex1, voltageChannelId, i);
			value2 = ib.getValue(bufIndex2, voltageChannelId, i);
			diff01 = (value0 - value1) * (value0 - value1);
			diff12 = (value1 - value2) * (value1 - value2);
			diff02 = (value0 - value2) * (value0 - value2);
			diffSum01 += diff01;
			diffSum12 += diff12;
			diffSum02 += diff02;
		}
		LOGnone("%f %f %f", diffSum01, diffSum12, diffSum02);
		if (diffSum01 > _thresholdDifferent && diffSum12 > _thresholdDifferent) {
			float minDiffSum = diffSum01 < diffSum12 ? diffSum01 : diffSum12;
			if (diffSum02 < _thresholdSimilar || minDiffSum / diffSum02 > _thresholdRatio) {
				result = true;
				LOGd("Found switch: %i %i %i %i", (int)diffSum01, (int)diffSum12, (int)diffSum02, (int)(minDiffSum / diffSum02));
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
