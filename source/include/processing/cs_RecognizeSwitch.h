/**
 * Authors: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: March 23, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <structs/buffer/cs_CircularBuffer.h>
#include <structs/buffer/cs_InterleavedBuffer.h>

class RecognizeSwitch {
private:
	// Keep up whether this class is running (started).
	bool _running = false;

	// Keep up number of detect() calls to skip.
	// This is used to prevent multiple switch detections in a row, and to prevent switch detections on init.
	uint8_t _skipSwitchDetectionTriggers = 200;

	// Threshold above which buffers are considered to be different.
	float _thresholdDifferent = SWITCHCRAFT_THRESHOLD;

	// Threshold below which buffers are considered to be similar.
	float _thresholdSimilar = SWITCHCRAFT_THRESHOLD;

	// Alternative to thresholdSimilar.
	float _thresholdRatio = 100.0;



	const static uint8_t _numStoredBuffers = 3;

	// Store the samples and meta data of the last detection.
	cs_power_samples_header_t _lastDetection;
	cs_power_samples_header_t _lastAlmostDetection;
	int16_t _lastDetectionSamples[_numStoredBuffers * InterleavedBuffer::getChannelLength()] = {0};
	int16_t _lastAlmostDetectionSamples[_numStoredBuffers * InterleavedBuffer::getChannelLength()] = {0};

	bool ignoreSample(sample_value_t value0, sample_value_t value1, sample_value_t value2);

	void setLastDetection(bool aboveThreshold, const CircularBuffer<buffer_id_t>& bufQueue, channel_id_t voltageChannelId);

public:
	// Gets a static singleton (no dynamic memory allocation)
	static RecognizeSwitch& getInstance() {
		static RecognizeSwitch instance;
		return instance;
	}

	/** Constructor: sets some default config values.
	 */
	RecognizeSwitch();

	/** Initialize: set initial values and allocate buffers.
	 */
	void init();

	/** Deinitialize: deallocate buffers.
	 */
	void deinit();

	/** Configure threshold
	 *
	 * @param[in] threshold                      Sets threshold different and similar.
	 */
	void configure(float threshold);

	/** Start detecting
	 */
	void start();

	/** Stop detecting
	 */
	void stop();

	/** Don't detect anything for num detect() calls.
	 */
	void skip(uint16_t num);

	/**
	 * Recognize switch event.
	 *
	 * @param[in] bufQueue                       The queue of available buffers.
	 * @param[in] voltageChannelId               Channel in which the voltage values can be found.
	 * @return                                   True when switch event was detected.
	 */
	bool detect(const CircularBuffer<buffer_id_t>& bufQueue, channel_id_t voltageChannelId);

	/**
	 * Get the samples of the last (almost) detected switch event.
	 *
	 * @param[in] type                           Either SWITCHCRAFT or SWITCHCRAFT_NON_TRIGGERED.
	 * @param[in/out]                            Result with buffer to fill with data (header + samples), and result code to set.
	 */
	void getLastDetection(PowerSamplesType type, uint8_t index, cs_result_t& result);
};
