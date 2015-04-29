/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <cstdint>

//#include <stdint.h>
//#include <common/cs_Types.h>

// The maximum number of channels supported by the library. Should NOT be changed!
#define PWM_MAX_CHANNELS        3

// To change the timer used for the PWM library replace the three defines below
#define PWM_TIMER               NRF_TIMER2
#define PWM_IRQHandler          TIMER2_IRQHandler
#define PWM_IRQn                TIMER2_IRQn

/* Pulse Wide Modulation mode typedef
 */
typedef enum {
	// 122 Hz PWM
    PWM_MODE_122,
	// 244 Hz PWM
    PWM_MODE_244,
	// 976 Hz PWM
    PWM_MODE_976,
	// 15625 Hz PWM
    PWM_MODE_15625,
	// 62500 Hz PWM
    PWM_MODE_62500
} pwm_mode_t;

/* Pulse Wide Modulation struct
 */
struct pwm_config_t {
    uint8_t         num_channels;
    uint8_t         gpio_pin[3];
    uint8_t         ppi_channel[6];
    uint8_t         gpiote_channel[3];
    uint8_t         mode;

    // default values
    pwm_config_t() :
    	num_channels   (3),
		gpio_pin       {8,9,10},
		ppi_channel    {0,1,2,3,4,5},
		gpiote_channel {2,3,0},
		mode           (PWM_MODE_122)
    {}
};

/*
#define PWM_DEFAULT_CONFIG  {.num_channels   = 3,                \
                             .gpio_pin       = {8,9,10},         \
                             .ppi_channel    = {0,1,2,3,4,5},    \
                             .gpiote_channel = {2,3,0},          \
                             .mode           = PWM_MODE_122}
*/

/* Pulse Wide Modulation class
 *
 * To turn on/off the power, as well as all intermediate stages, for example with dimming, the PWM class is used.
 */
class PWM {
private:
	// Private PWM constructor
	PWM() {}
	// Private PWM copy constructor
	PWM(PWM const&);
	// Private PWM copy assignment definition
	void operator=(PWM const &);
	
	// store values last set
	uint8_t _pwmChannel;
	uint32_t _pwmValue;

	int32_t ppiEnableChannel(uint32_t ch_num, volatile uint32_t *event_ptr, volatile uint32_t *task_ptr);
public:
	// Gets a static singleton (no dynamic memory allocation) of the PWM clss
	static PWM& getInstance() {
		static PWM instance;
		return instance;
	}
	// Initialize the pulse wide modulation settings
	uint32_t init(pwm_config_t *config);

	// Set the value of a specific channel
	void setValue(uint8_t pwm_channel, uint32_t pwm_value);

	void getValue(uint8_t &pwm_channel, uint32_t &pwm_value);
	
	//TODO: make the following private

	// number of channels
	uint8_t _numChannels;
	uint16_t _maxValue;
	uint16_t _nextValue[PWM_MAX_CHANNELS];
	uint8_t _gpioteChannel[PWM_MAX_CHANNELS];
	uint8_t _gpioPin[PWM_MAX_CHANNELS];
	bool _running[PWM_MAX_CHANNELS];
};
