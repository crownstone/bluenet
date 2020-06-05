/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 27, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/cs_RecognizeSwitch.h>
#include <cfg/cs_Config.h>
#include <protocol/cs_Packets.h>
#include <structs/cs_PacketsInternal.h>
#include <time/cs_SystemTime.h>

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
	sample_value_id_t checkLength = ib.getChannelLength() / 2;
	sample_value_id_t shift = checkLength / 2;
	sample_value_id_t startInd;
	sample_value_id_t endInd;

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
	}
	if (result) {
		setLastDetection(true, currentBufIndex, voltageChannelId);
		_skipSwitchDetectionTriggers = 5;
	}

	return result;
}

void RecognizeSwitch::setLastDetection(bool aboveThreshold, buffer_id_t currentBufIndex, channel_id_t voltageChannelId) {
	cs_power_samples_header_t* header;
	int16_t* buf;
	if (aboveThreshold) {
		header = &_lastDetection;
		buf = _lastDetectionSamples;
	}
	else {
		header = &_lastAlmostDetection;
		buf = _lastAlmostDetectionSamples;
	}
//	header->type // type can be set once.
//	header->index // index is set on get.
//	header->count
	header->unixTimestamp = SystemTime::posix();
//	header->delayUs =
	header->sampleIntervalUs = CS_ADC_SAMPLE_INTERVAL_US;
//	header->offset =
//	header->multiplier =

	// Copy the samples.
	InterleavedBuffer & ib = InterleavedBuffer::getInstance();
	buffer_id_t bufIndices[_numStoredBuffers];
	bufIndices[_numStoredBuffers - 1] = currentBufIndex;
	for (uint8_t i = 0; i < _numStoredBuffers; ++i) {
		bufIndices[i] = ib.getPrevious(currentBufIndex, _numStoredBuffers - i - 1);
	}
//	bufIndices[0] = ib.getPrevious(currentBufIndex, 2);
//	bufIndices[1] = ib.getPrevious(currentBufIndex, 1);
//	bufIndices[2] = currentBufIndex;
	uint16_t numSamples = ib.getChannelLength();
	for (uint8_t i = 0; i < _numStoredBuffers; ++i) {
		for (sample_value_id_t j = 0; j < numSamples; ++j) {
			buf[i * numSamples + j] = ib.getValue(bufIndices[i], voltageChannelId, j);
		}
	}
}

void RecognizeSwitch::getLastDetection(PowerSamplesType type, uint8_t index, cs_result_t& result) {
	LOGd("getLastDetection type=%u ind=%u", type, index);
	cs_power_samples_header_t* header;
	int16_t* buf;

	// Check type.
	switch (type) {
		case POWER_SAMPLES_TYPE_SWITCHCRAFT:
			header = &_lastDetection;
			buf = _lastDetectionSamples;
			break;
		case POWER_SAMPLES_TYPE_SWITCHCRAFT_NON_TRIGGERED:
			header = &_lastAlmostDetection;
			buf = _lastAlmostDetectionSamples;
			break;
		default:
			return;
	}

	// Check index.
	if (index >= _numStoredBuffers) {
		LOGw("index=%u", index);
		result.returnCode = ERR_WRONG_PARAMETER;
		return;
	}

	// Check size.
	uint16_t numSamples = InterleavedBuffer::getInstance().getChannelLength();
	uint16_t samplesSize = numSamples * sizeof(sample_value_t);
	uint16_t requiredSize = sizeof(*header) + samplesSize;
	if (result.buf.len < requiredSize) {
		LOGw("size=%u required=%u", result.buf.len, requiredSize);
		result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}

	// Set header fields.
	header->type = type;
	header->index = index;
	header->count = numSamples;

	// Copy data to buffer.
	memcpy(result.buf.data, header, sizeof(*header));
	memcpy(result.buf.data + sizeof(*header), buf + index * numSamples, samplesSize);
	result.dataSize = requiredSize;
	result.returnCode = ERR_SUCCESS;
}
