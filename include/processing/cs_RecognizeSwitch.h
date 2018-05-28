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
	bool _running;

	// Keep up number of detect() calls to skip.
	// This is used to prevent multiple switch detections in a row, and to prevent switch detections on init.
	uint8_t _skipSwitchDetectionTriggers;

	// Threshold above which buffers are considered to be different.
	float _thresholdDifferent;

	// Threshold below which buffers are considered to be similar.
	float _thresholdSimilar;

	// Alternative to thresholdSimilar.
	float _thresholdRatio;

public:
	//! Gets a static singleton (no dynamic memory allocation)
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

	/** Start detecting
	 */
	void start();

	/** Stop detecting
	 */
	void stop();

	/** Don't detect anything for num detect() calls.
	 */
	void skip(uint16_t num);

	/** Recognize switch state.
	 *
	 * @param[in] bufIndex                       Buffer to check for a switch (should be previous buffer).
	 * @parampin] voltageChannelId               Channel in which the voltage values can be found.
	 * 
	 */
	bool detect(buffer_id_t bufIndex, channel_id_t voltageChannelId);
};
