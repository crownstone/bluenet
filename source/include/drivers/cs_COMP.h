/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 31, 2017
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

// TODO: requires symbol names to be preceded by std::
#include <cstdint>
#include "ble/cs_Nordic.h"

/**
 * Compare events, also used for configuration.
 */
enum CompEvent_t {
	// The sampled value went above the upper threshold.
	COMP_EVENT_UP,
	// The sampled value went below the lower threshold.
	COMP_EVENT_DOWN,
	// The sampled value went above the upper threshold, or below the lower threshold.
	COMP_EVENT_CROSS,
	// Only used for configuration: When you want to get both UP, and DOWN events.
	COMP_EVENT_BOTH,
};

/**
 * Define comp_event_cb_t to be a void function with a CompEvent_t parameter as single argument.
 * This function can be used as callback after an event occurred.
 */
typedef void (*comp_event_cb_t) (CompEvent_t event);

/**
 * Class that enables you to get events when a the voltage on an AIN pin goes over or below a given threshold.
 *
 * There are 2 thresholds, as this class supports hysteresis.
 */
class COMP {
public:
	// Use static variant of singleton, no dynamic memory allocation
	static COMP& getInstance() {
		static COMP instance;
		return instance;
	}

	/**
	 * The COMP unit gets initialized by indicating a pin.
	 * TODO: use const where appropriate
	 * TODO: use nrf_drv_comp_ain_to_gpio in implementation
	 *
	 * @param[in] ainPin           An analog input pin: so for pin AIN6 the value should be 6.
	 * @param[in] thresholdDown    Threshold to be triggered when input drops below a certain value.
	 * @param[in] thresholdUp      Threshold to be triggered when input goes beyond a certain value.
	 * @param[in] callback         The function to be called when the value goes over/under the threshold. Use null pointer for none.
	 */
	void init(uint8_t ainPin, float thresholdDown, float thresholdUp, comp_event_cb_t callback);

	/**
	 * Start the comparator.
	 *
	 * @param[in] event            Selects which events to get.
	 */
	void start(CompEvent_t event);

	/**
	 * Sample the comparator.
	 *
	 * @return                     True when the value is above the threshold, false when below.
	 */
	bool sample();

	/**
	 * Internal usage.
	 *
	 * Handle a comparator event, on main thread.
	 */
	void handleEventDecoupled(nrf_comp_event_t event);

	/**
	 * Internal usage.
	 *
	 * Handle a comparator event, in interrupt.
	 */
	void handleEvent(nrf_comp_event_t event);

private:
	// This class is singleton, constructor is private
	COMP();

	// This class is singleton, deny implementation
	COMP(COMP const&);

	// This class is singleton, deny implementation
	void operator=(COMP const &);

	/**
	 * Apply work arounds that are suggested by Nordic. For the NRF52 PAN 12 refers to COMP.
	 * + PAN 12 COMP: Reference ladder is not correctly calibrated
	 */
	void applyWorkarounds() {
#ifdef NRF52_PAN_12
		*(volatile uint32_t *)0x40013540 = (*(volatile uint32_t *)0x10000324 & 0x00001F00) >> 8;
#endif
	}

	comp_event_cb_t _callback = nullptr;
};

