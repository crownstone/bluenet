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
	//! A number of cycles will be skipped not to have multiple switch detections in a row
	uint8_t _skipSwitchDetectionTriggers;

	//! A threshold close to zero that defines the slope
	int16_t _threshold;

	//! A circular buffer to the sliding window
	CircularBuffer<int16_t> * _circBuffer;

	bool _running;

	
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

	/** Don't detect anything for num cycles.
	 */
	void skip(uint16_t num);

	/** Recognize switch state.
	 *
	 * @param[in] bufIndex                       Buffer that was just populated with values.
	 * @parampin] voltageChannelId               Channel in which the voltage values can be found.
	 * 
	 */
	bool detect(buffer_id_t bufIndex, channel_id_t voltageChannelId);
};
