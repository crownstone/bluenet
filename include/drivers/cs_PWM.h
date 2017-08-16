/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 9 Mar., 2016
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <cstdint>

#include "ble/cs_Nordic.h"
#include "cfg/cs_Config.h"

#define ERR_PWM_NOT_ENABLED 1


typedef struct {
	uint16_t pin;
	bool inverted;
} pwm_channel_config_t;

typedef struct {
	uint32_t period_us;
	uint8_t channelCount;
	pwm_channel_config_t channels[CS_PWM_MAX_CHANNELS];
} pwm_config_t;

/** Pulse Wide Modulation class
 *
 * To turn on/off the power, as well as all intermediate stages, for example with dimming, the PWM class is used.
 */
class PWM {
public:
	//! Gets a static singleton (no dynamic memory allocation) of the PWM class
	static PWM& getInstance() {
		static PWM instance;
		return instance;
	}

	//! Initialize the PWM settings.
	//! config can be safely deleted after calling this function.
	uint32_t init(pwm_config_t& config);

	//! De-Initialize the PWM instance, i.e. free allocated resources
	uint32_t deinit();

	//! Set the value of a specific channel
	void setValue(uint8_t channel, uint16_t value);

	//! Get current value of a specific channel
	uint16_t getValue(uint8_t channel);

//	void start();
//	void stop();
	void sync();

	//! Interrupt handler: internal function, implementation specific.
	void _handleInterrupt();

private:
	//! Private PWM constructor
	PWM();

	//! Private PWM copy constructor
	PWM(PWM const&);
	//! Private PWM copy assignment definition
	void operator=(PWM const &);

	pwm_config_t _config;

	uint32_t _values[CS_PWM_MAX_CHANNELS];

	//! Flag to indicate that the init function has been successfully performed
	bool _initialized;

	//! Init a channel
	uint32_t initChannel(uint8_t index, pwm_channel_config_t& config);

	// -----------------------------------
	// ----- Implementation specific -----
	// -----------------------------------

	//! Pointer to the timer.
	nrf_drv_timer_t* _timer;

	//! Max value of channel, in ticks.
	uint32_t _maxTickVal;

	//! Values of the channels in ticks.
	uint32_t _tickValues[CS_PWM_MAX_CHANNELS];

	//! PPI channels to be used to communicate from Timer to GPIOTE.
	nrf_ppi_channel_t _ppiChannels[2*CS_PWM_MAX_CHANNELS];

	//! GPIOTE init states cache
	nrf_gpiote_outinit_t _gpioteInitStates[CS_PWM_MAX_CHANNELS];
	nrf_gpiote_outinit_t _gpioteInitStatesInversed[CS_PWM_MAX_CHANNELS];

	//! Returns whether a channel is currently dimming (value > 0 and < max).
	bool _isPwmEnabled[CS_PWM_MAX_CHANNELS];

	//! Enables pwm for given channel (to be used when value is > 0 and < max).
	void enablePwm(uint8_t channel);

	//! Disables pwm for given channel (to be used when value is 0 or max).
	void disablePwm(uint8_t channel);

	//! Helper function to get the timer channel, given the index.
	nrf_timer_cc_channel_t getTimerChannel(uint8_t index);
	//! Helper function to get the gpiote task out, given the index.
	nrf_gpiote_tasks_t getGpioteTaskOut(uint8_t index);
	//! Helper function to get the ppi channel, given the index.
	nrf_ppi_channel_t getPpiChannel(uint8_t index);
};
