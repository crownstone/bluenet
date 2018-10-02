/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 31, 2017
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

// TODO: requires symbol names to be preceded by std::
#include <cstdint>
#include <cs_Nordic.h>

/**
 * Compare events indicate if the sampled values (be it current, voltage, or temperature) go through a predefined threshold from
 * below, from above, or both.
 */
enum CompEvent_t {
	//! No event observed
	COMP_EVENT_NONE,
	//! The sampled value is beyond the predefined threshold, on VIN+ > VIN-
	COMP_EVENT_UP,
	//! The sampled value drops below the predefined threshold, on VIN+ < VIN-
	COMP_EVENT_DOWN,
	//! TODO: what is this? why is it different from COMP_EVENT_CROSS and why is that insufficient?
	COMP_EVENT_BOTH,
	//! The sampled value crosses the threshold, either from below or above, after VIN+ == VIN-
	COMP_EVENT_CROSS,
};

/**
 * Define comp_event_cb_t to be a void function with a CompEvent_t parameter as single argument. This function can be used as
 * callback after an event occurred.
 */
typedef void (*comp_event_cb_t) (CompEvent_t event);

/**
 * The comp_event_cb_data_t struct defines an event and a callback function.
 */
struct comp_event_cb_data_t {
	//! A function of type comp_event_cb_t to be used as callback
	comp_event_cb_t callback;
	//! An argument for the above callback
	CompEvent_t event;
};

/**
 * The comparator is a hardware peripheral on the nRF52. It is addressed through the COMP HAL which is addressed through the COMP
 * driver. 
 * Requires a real-time clock, the RTC to attach timestamps to the events.
 *
 * TODO: Use VOLTAGE_THRESHOLD_TO_INT
 */
class COMP {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static COMP& getInstance() {
		static COMP instance;
		return instance;
	}

	/**
	 * The COMP unit gets initialized by indicating a pin.
	 * TODO: use const where appropriate
	 * TODO: use nrf_drv_comp_ain_to_gpio in implementation
	 * 
	 * @param[in] ainPin           An input pin, TODO: use pin_id_t or nrf_comp_input_t, not uint8_t
	 * @param[in] thresholdDown    Threshold to be triggered when input drops below a certain value. TODO: check.
	 * @param[in] thresholdUp      Threshold to be triggered when input goes beyond a certain value. TODO: check. 
	 */
	void init(uint8_t ainPin, float thresholdDown, float thresholdUp);

	/**
	 * The hardware peripheral is started. First, init() has to be called. 
	 *
	 * @param[in] event            Start the comparator with "event" as additional configuration option.
	 */
	void start(CompEvent_t event);

	/**
	 * Return a single sample which uses 1 bit of the 32 available bits. This is:
	 *   0 if VIN+ < VIN-
         *   1 if VIN+ > VIN-
	 *
	 * @return                     A sample from the comparator indicating relation between VIN+ and VIN-.
	 */
	uint32_t sample();

	/** 
	 * Set the callback which is called on an event. This callback is put in the scheduler and not executed directly.
	 *
	 * @param[in] callback         A struct with a function and parameter for that function.
	 */
	void setEventCallback(comp_event_cb_t callback);

	/**
	 * Handle a comparator event. This is automatically called on each event. In this function the event is copied and if
	 * there is an event handler set, it will be called indirectly by adding the event callback to the app scheduler.
	 *
	 * @param[in] event            Handle a particular event.
	 */
	void handleEvent(nrf_comp_event_t & nrf_comp_event);

private:
	//! This class is singleton, constructor is private
	COMP();

	//! This class is singleton, deny implementation
	COMP(COMP const&);

	//! This class is singleton, deny implementation
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

	//! Particular callback to be used, set by setEventCallback().
	comp_event_cb_data_t _eventCallbackData;

	//! Timestamp of the last event, this uses the RTC
	uint32_t _lastEventTimestamp;

};

