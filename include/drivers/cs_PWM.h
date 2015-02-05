/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <stdint.h>

// The maximum number of channels supported by the library. Should NOT be changed!
#define PWM_MAX_CHANNELS        3

// To change the timer used for the PWM library replace the three defines below
#define PWM_TIMER               NRF_TIMER2
#define PWM_IRQHandler          TIMER2_IRQHandler
#define PWM_IRQn                TIMER2_IRQn

typedef enum {
    PWM_MODE_LED_100,   // 0-100 resolution, 156 Hz PWM frequency, 32 kHz timer frequency (prescaler 9)
    PWM_MODE_LED_255,   // 8-bit resolution, 122 Hz PWM frequency, 65 kHz timer frequency (prescaler 8)
    PWM_MODE_LED_1000,  // 0-1000 resolution, 125 Hz PWM frequency, 500 kHz timer frequency (prescaler 5)

    PWM_MODE_MTR_100,   // 0-100 resolution, 20 kHz PWM frequency, 4MHz timer frequency (prescaler 2)
    PWM_MODE_MTR_255    // 8-bit resolution, 31 kHz PWM frequency, 16MHz timer frequency (prescaler 0)
} pwm_mode_t;

typedef struct {
    uint8_t         num_channels;
    uint8_t         gpio_pin[3];
    uint8_t         ppi_channel[6];
    uint8_t         gpiote_channel[3];
    uint8_t         mode;
} pwm_config_t;

#define PWM_DEFAULT_CONFIG  {.num_channels   = 3,                \
                             .gpio_pin       = {8,9,10},         \
                             .ppi_channel    = {0,1,2,3,4,5},    \
                             .gpiote_channel = {2,3,0},          \
                             .mode           = PWM_MODE_LED_100}


class PWM {
private:
	PWM() {}
	PWM(PWM const&); // singleton, deny implementation
	void operator=(PWM const &); // singleton, deny implementation
public:
	// use static variant of singleton, no dynamic memory allocation
	static PWM& getInstance() {
		static PWM instance;
		return instance;
	}

	uint32_t init(pwm_config_t *config);
	void setValue(uint8_t pwm_channel, uint32_t pwm_value);

	void getValue(uint8_t &pwm_channel, uint32_t &pwm_value);
private:
	// store values last set
	uint8_t _pwmChannel;
	uint32_t _pwmValue;

	int32_t ppiEnableChannel(uint32_t ch_num, volatile uint32_t *event_ptr, volatile uint32_t *task_ptr);

public: // TODO: make these private
	uint8_t _numChannels;
	uint16_t _maxValue;
	uint16_t _nextValue[PWM_MAX_CHANNELS];
	uint8_t _gpioteChannel[PWM_MAX_CHANNELS];
	uint8_t _gpioPin[PWM_MAX_CHANNELS];
	bool _running[PWM_MAX_CHANNELS];
};
