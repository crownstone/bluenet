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

#define LOGSwitchcraftWarn LOGw
#define LOGSwitchcraftDebug LOGnone
#define LOGSwitchcraftVerbose LOGnone

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


bool RecognizeSwitch::detect(const CircularBuffer<adc_buffer_id_t>& bufQueue, adc_channel_id_t voltageChannelId) {
	if (!_running) {
		return false;
	}
	if (_skipSwitchDetectionTriggers > 0) {
		_skipSwitchDetectionTriggers--;
		return false;
	}

	// Last buffer is unfiltered.
	if (bufQueue.size() < _numBuffersRequired + 1) {
		LOGSwitchcraftDebug("Not enough buffers");
		return false;
	}

	FoundSwitch found = FoundSwitch::False;
	for (uint8_t i = 0; i < (_numBuffersRequired - 2); ++i) {
		FoundSwitch tempFound = detect(bufQueue, voltageChannelId, i);
		if (tempFound == FoundSwitch::True) {
			found = tempFound;
			break;
		}
		if (tempFound == FoundSwitch::Almost) {
			found = tempFound;
		}
	}

	switch (found) {
		case FoundSwitch::True: {
			setLastDetection(true, bufQueue, voltageChannelId);
			_skipSwitchDetectionTriggers = 5;
			return true;
		}
		case FoundSwitch::Almost: {
			LOGSwitchcraftDebug("Almost found switch");
			setLastDetection(false, bufQueue, voltageChannelId);
			return false;
		}
		case FoundSwitch::False: {

			return false;
		}
	}

//	FoundSwitch found = detectSwitch(bufQueue, voltageChannelId);
//
//	switch (found) {
//		case FoundSwitch::True: {
//			setLastDetection(true, bufQueue, voltageChannelId);
//			_skipSwitchDetectionTriggers = 5;
//			return true;
//		}
//		case FoundSwitch::Almost: {
//			setLastDetection(false, bufQueue, voltageChannelId);
//			return false;
//		}
//		case FoundSwitch::False: {
//			return false;
//		}
//	}

	return false;
}

RecognizeSwitch::FoundSwitch RecognizeSwitch::detectSwitch(const CircularBuffer<adc_buffer_id_t>& bufQueue, adc_channel_id_t voltageChannelId) {
	// Buffer index (size - 1) is unfiltered buffer.
	adc_buffer_id_t bufIndexFirst = bufQueue[bufQueue.size() - (1 + _numBuffersRequired)];
	adc_buffer_id_t bufIndexLast  = bufQueue[bufQueue.size() - (1 + 1)];

	bool foundAlmost = false;

	// Check only part of the buffer length (half buffer length).
	// Then repeat that at different parts of the buffer (start, mid, end).
	// Example: if channel length = 100, then check 0-49, 25-74, and 50-99.
	adc_sample_value_id_t bufferLength = AdcBuffer::getInstance().getChannelLength();
	adc_sample_value_id_t checkLength = bufferLength / 2;
	adc_sample_value_id_t shift = checkLength / 2;
	adc_sample_value_id_t startIndex;

	float diffFirstLast, diffCenterFirst, diffCenterLast;
	float minDiff;
	float lowerTheshold = 0.1 * _thresholdDifferent;

	AdcBuffer & ib = AdcBuffer::getInstance();

	for (startIndex = 0; startIndex < (bufferLength - shift); startIndex += shift) {

		// Difference between first and last buffer.
		diffFirstLast = calcDiff(bufQueue, voltageChannelId, bufIndexFirst, bufIndexLast, startIndex, checkLength);

		// Loop over the center buffers.
		for (uint8_t i = 0; i < (_numBuffersRequired - 2); ++i) {
			adc_buffer_id_t bufIndexCenter = bufQueue[bufQueue.size() - (1 + _numBuffersRequired - i - 1)];

			// Check buffer validity before doing the calculations.
			if (!ib.getBuffer(bufIndexFirst)->valid || !ib.getBuffer(bufIndexCenter)->valid || !ib.getBuffer(bufIndexLast)->valid) {
				LOGSwitchcraftWarn("Buffer not valid");
				return RecognizeSwitch::FoundSwitch::False;
			}

			// Difference between center and first buffer. And between center and last buffer.
			diffCenterFirst = calcDiff(bufQueue, voltageChannelId, bufIndexFirst, bufIndexCenter, startIndex, checkLength);
			diffCenterLast  = calcDiff(bufQueue, voltageChannelId, bufIndexLast,  bufIndexCenter, startIndex, checkLength);

			// Check buffer validity after doing the calculations.
			if (!ib.getBuffer(bufIndexFirst)->valid || !ib.getBuffer(bufIndexCenter)->valid || !ib.getBuffer(bufIndexLast)->valid) {
				LOGSwitchcraftWarn("Buffer not valid");
				return RecognizeSwitch::FoundSwitch::False;
			}

			LOGnone("buffer ind: first=%u center=%u last=%u", bufIndexFirst, bufIndexCenter, bufIndexLast);
			LOGSwitchcraftVerbose("center iter=%u sample start=%u %d %d %d", i, startIndex, (int32_t)diffCenterFirst, (int32_t)diffCenterLast, (int32_t)diffFirstLast);
			if (diffCenterFirst > _thresholdDifferent && diffCenterLast > _thresholdDifferent) {
				minDiff = diffCenterFirst < diffCenterLast ? diffCenterFirst : diffCenterLast;
				if (diffFirstLast < _thresholdSimilar || minDiff / diffFirstLast > _thresholdRatio) {
					LOGSwitchcraftDebug("Found switch: %i %i %i %i", (int)diffCenterFirst, (int)diffCenterLast, (int)diffFirstLast, (int)(minDiff / diffFirstLast));
					return FoundSwitch::True;
				}
			}

			// Check if it was almost recognized as switch.
			if (diffCenterFirst > lowerTheshold && diffCenterLast > lowerTheshold && diffFirstLast < _thresholdSimilar) {
				LOGSwitchcraftDebug("Almost found switch: %i %i %i", (int)diffCenterFirst, (int)diffCenterLast, (int)diffFirstLast);
				foundAlmost = true;
			}

		}
	}

	if (foundAlmost) {
		return RecognizeSwitch::FoundSwitch::Almost;
	}
	return RecognizeSwitch::FoundSwitch::False;
}

float RecognizeSwitch::calcDiff(
		const CircularBuffer<adc_buffer_id_t>& bufQueue,
		const adc_channel_id_t voltageChannelId,
		const adc_buffer_id_t bufIndex1,
		const adc_buffer_id_t bufIndex2,
		const adc_sample_value_id_t startIndex,
		const adc_sample_value_id_t numSamples) {
	AdcBuffer & ib = AdcBuffer::getInstance();
	adc_sample_value_id_t endIndex = startIndex + numSamples;
	float diffSum = 0;
	float value1, value2, diff;
	LOGnone("start=%i end=%i", startIndex, endIndex);
	for (int i = startIndex; i < endIndex; ++i) {
		value1 = ib.getValue(bufIndex1, voltageChannelId, i);
		value2 = ib.getValue(bufIndex2, voltageChannelId, i);
		if (ignoreSample(value1, value2)) {
			diff = 0;
		}
		else {
			diff = (value1 - value2) * (value1 - value2);
		}
		diffSum += diff;
	}
	return diffSum;
}


RecognizeSwitch::FoundSwitch RecognizeSwitch::detect(const CircularBuffer<adc_buffer_id_t>& bufQueue, adc_channel_id_t voltageChannelId, uint8_t iteration) {
	AdcBuffer & ib = AdcBuffer::getInstance();

	// Check only part of the buffer length (half buffer length).
	// Then repeat that at different parts of the buffer (start, mid, end).
	// Example: if channel length = 100, then check 0-49, 25-74, and 50-99.
	adc_sample_value_id_t checkLength = ib.getChannelLength() / 2;
	adc_sample_value_id_t shift = checkLength / 2;
	adc_sample_value_id_t startInd;
	adc_sample_value_id_t endInd;

	// Buffer index (size - 1) is unfiltered buffer.
	adc_buffer_id_t bufIndexFirst  = bufQueue[bufQueue.size() - (1 + _numBuffersRequired)];
	adc_buffer_id_t bufIndexCenter = bufQueue[bufQueue.size() - (1 + _numBuffersRequired - iteration - 1)];
	adc_buffer_id_t bufIndexLast   = bufQueue[bufQueue.size() - (1 + 1)];
	LOGnone("buffer ind: first=%u center=%u last=%u", bufIndexFirst, bufIndexCenter, bufIndexLast);

	// Check buffer validity before doing the calculations.
	if (!ib.getBuffer(bufIndexFirst)->valid || !ib.getBuffer(bufIndexCenter)->valid || !ib.getBuffer(bufIndexLast)->valid) {
		LOGSwitchcraftWarn("Buffer not valid");
		return RecognizeSwitch::FoundSwitch::False;
	}

	float valueFirst, valueCenter, valueLast;
	float diffCenterFirst, diffCenterLast, diffFirstLast; // Diff between 2 values of 2 buffers.
	float diffSumCenterFirst, diffSumCenterLast, diffSumFirstLast; // Summed diff between all values of 2 buffers.
	float minDiffSum;
	float lowerTheshold = 0.1 * _thresholdDifferent;
	bool foundAlmost = false;

	for (startInd = 0; startInd < (ib.getChannelLength() - shift); startInd += shift) {
		diffSumCenterFirst = 0;
		diffSumCenterLast = 0;
		diffSumFirstLast = 0;
		endInd = startInd + checkLength;
		LOGnone("start=%i end=%i", startInd, endInd);
		for (int i = startInd; i < endInd; ++i) {
			valueFirst  = ib.getValue(bufIndexFirst,  voltageChannelId, i);
			valueCenter = ib.getValue(bufIndexCenter, voltageChannelId, i);
			valueLast   = ib.getValue(bufIndexLast,   voltageChannelId, i);
			if (ignoreSample(valueFirst, valueCenter, valueLast)) {
				diffCenterFirst = 0;
				diffCenterLast = 0;
				diffFirstLast = 0;
			}
			else {
				diffCenterFirst = (valueFirst - valueCenter) * (valueFirst - valueCenter);
				diffCenterLast = (valueCenter - valueLast) * (valueCenter - valueLast);
				diffFirstLast = (valueFirst - valueLast) * (valueFirst - valueLast);
			}
			diffSumCenterFirst += diffCenterFirst;
			diffSumCenterLast += diffCenterLast;
			diffSumFirstLast += diffFirstLast;
		}
		LOGSwitchcraftVerbose("center iter=%u sample start=%u %d %d %d", iteration, startInd, (int32_t)diffSumCenterFirst, (int32_t)diffSumCenterLast, (int32_t)diffSumFirstLast);

		// Check buffer validity after doing the calculations.
		if (!ib.getBuffer(bufIndexFirst)->valid || !ib.getBuffer(bufIndexCenter)->valid || !ib.getBuffer(bufIndexLast)->valid) {
			LOGSwitchcraftWarn("Buffer not valid");
			return RecognizeSwitch::FoundSwitch::False;
		}

		if (diffSumCenterFirst > _thresholdDifferent && diffSumCenterLast > _thresholdDifferent) {
			minDiffSum = diffSumCenterFirst < diffSumCenterLast ? diffSumCenterFirst : diffSumCenterLast;
			if (diffSumFirstLast < _thresholdSimilar || minDiffSum / diffSumFirstLast > _thresholdRatio) {
				LOGSwitchcraftDebug("Found switch: %i %i %i %i", (int)diffSumCenterFirst, (int)diffSumCenterLast, (int)diffSumFirstLast, (int)(minDiffSum / diffSumFirstLast));
				return RecognizeSwitch::FoundSwitch::True;
			}
		}

		// Check if it was almost recognized as switch.
		if (diffSumCenterFirst > lowerTheshold && diffSumCenterLast > lowerTheshold && diffSumFirstLast < _thresholdSimilar) {
			LOGSwitchcraftDebug("Almost found switch: %i %i %i", (int)diffSumCenterFirst, (int)diffSumCenterLast, (int)diffSumFirstLast);
			foundAlmost = true;
		}
	}

	if (foundAlmost) {
		return RecognizeSwitch::FoundSwitch::Almost;
	}
	return RecognizeSwitch::FoundSwitch::False;
}

bool RecognizeSwitch::ignoreSample(const adc_sample_value_t value1, const adc_sample_value_t value2) {
	return ignoreSample(value1, value2, 0);
}

bool RecognizeSwitch::ignoreSample(const adc_sample_value_t value0, const adc_sample_value_t value1, const adc_sample_value_t value2) {
	// Observed: sometimes, or often, the builtin one 1B10 measures value 2047 around the top of the curve.
	// This triggers a false positive when the width of this block changes.
	if (value0 == 2047 || value1 == 2047 || value2 == 2047) {
		return true;
	}
	return false;
}

void RecognizeSwitch::setLastDetection(bool aboveThreshold, const CircularBuffer<adc_buffer_id_t>& bufQueue, adc_channel_id_t voltageChannelId) {
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
	AdcBuffer & ib = AdcBuffer::getInstance();
	adc_buffer_id_t bufIndices[_numStoredBuffers];
	for (uint8_t i = 0; i < _numStoredBuffers; ++i) {
		bufIndices[i] = bufQueue[bufQueue.size() - (1 + _numBuffersRequired) + i]; // Last buffer is the unfiltered version.
	}

	uint16_t numSamples = ib.getChannelLength();
	for (uint8_t i = 0; i < _numStoredBuffers; ++i) {
		for (adc_sample_value_id_t j = 0; j < numSamples; ++j) {
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
	uint16_t numSamples = AdcBuffer::getInstance().getChannelLength();
	uint16_t samplesSize = numSamples * sizeof(adc_sample_value_t);
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
