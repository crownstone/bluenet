/**
 * Authors: Crownstone Team
 * Copyright: Crownstone B.V.
 * Date: March 23, 2018
 * License: LGPLv3+, MIT, Apache
 */

#pragma once

#include <structs/buffer/cs_InterleavedBuffer.h>

class RecognizeSwitch {
private:
	//! A number of cycles will be skipped not to have multiple switch detections in a row
	uint8_t _skipSwitchDetectionTriggers;

public:
	//! Gets a static singleton (no dynamic memory allocation)
	static RecognizeSwitch& getInstance() {
		static RecognizeSwitch instance;
		return instance;
	}

	/** Constructor sets some default values like number of times switch detection has to be skipped after a detection
	 * event.
	 */
	RecognizeSwitch();

	/** Recognize switch state.
	 *
	 * @param[in] bufIndex                       Buffer that was just populated with values.
	 * @parampin] voltageChannelId               Channel in which the voltage values can be found.
	 * 
	 */
	bool detect(buffer_id_t bufIndex, channel_id_t voltageChannelId);

};
