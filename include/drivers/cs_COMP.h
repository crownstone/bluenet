/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan 31, 2017
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <cstdint>


extern "C" {
#include <nrf_drv_comp.h>
}

enum CompEvent_t {
	COMP_EVENT_NONE,
	COMP_EVENT_UP,
	COMP_EVENT_DOWN,
	COMP_EVENT_BOTH,
	COMP_EVENT_CROSS,
};

typedef void (*comp_event_cb_t) (CompEvent_t event);

struct comp_event_cb_data_t {
	comp_event_cb_t callback;
	CompEvent_t event;
};


class COMP {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static COMP& getInstance() {
		static COMP instance;
		return instance;
	}

	void init(uint8_t ainPin, float thresholdDown, float thresholdUp);
	void start(CompEvent_t event);

	uint32_t sample();

	/** Set the callback which is called on an event
	 */
	void setEventCallback(comp_event_cb_t callback);

	void handleEvent(nrf_comp_event_t event);

private:
	COMP();

	//! This class is singleton, deny implementation
	COMP(COMP const&);
	//! This class is singleton, deny implementation
	void operator=(COMP const &);

	void applyWorkarounds() {
		// PAN 12 COMP: Reference ladder is not correctly calibrated
		*(volatile uint32_t *)0x40013540 = (*(volatile uint32_t *)0x10000324 & 0x00001F00) >> 8;
	}

	comp_event_cb_data_t _eventCallbackData;
	uint32_t _lastEventTimestamp;

};

