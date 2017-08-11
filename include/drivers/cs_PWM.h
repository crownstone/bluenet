/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 9 Mar., 2016
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <cstdint>

#include "ble/cs_Nordic.h"

extern "C" {
#include "nrf_drv_pwm.h"
}

#define ERR_PWM_NOT_ENABLED 1

#define PWM_MAX_CHANNELS 2

typedef struct {
	uint16_t pin;
	bool inverted;
} pwm_channel_config_t;

typedef struct {
	uint32_t period_us;
	uint8_t channelCount;
	pwm_channel_config_t channels[PWM_MAX_CHANNELS];
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
	void restart();

private:
	//! Private PWM constructor
	PWM();

	//! Private PWM copy constructor
	PWM(PWM const&);
	//! Private PWM copy assignment definition
	void operator=(PWM const &);

	pwm_config_t _config;

	uint32_t values[PWM_MAX_CHANNELS];

	//! Flag to indicate that the init function has been successfully performed
	bool _initialized;

	// -- Hardware specific --
	// This array cannot be allocated on stack (hence "static") and it must be in RAM (hence no "const").
	static nrf_pwm_values_individual_t _nrfValues;
	static nrf_drv_pwm_t _nrfPwm;
	static nrf_pwm_sequence_t _nrfSeq;
};
