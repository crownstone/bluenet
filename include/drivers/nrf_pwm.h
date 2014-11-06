/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+
 */
#ifndef __NRF_PWM__H__
#define __NRF_PWM__H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The maximum number of channels supported by the library. Should NOT be changed! 
#define PWM_MAX_CHANNELS        3

// Set this to 1 if the application uses a SoftDevice, 0 otherwise
#define USE_WITH_SOFTDEVICE     1

// To change the timer used for the PWM library replace the three defines below
#define PWM_TIMER               NRF_TIMER2
#define PWM_IRQHandler          TIMER2_IRQHandler
#define PWM_IRQn                TIMER2_IRQn

#define PWM_DEFAULT_CONFIG  {.num_channels   = 3,                \
                             .gpio_num       = {8,9,10},         \
                             .ppi_channel    = {0,1,2,3,4,5},    \
                             .gpiote_channel = {2,3,0},          \
                             .mode           = PWM_MODE_LED_100};

typedef enum
{
    PWM_MODE_LED_100,   // 0-100 resolution, 156 Hz PWM frequency, 32 kHz timer frequency (prescaler 9)
    PWM_MODE_LED_255,   // 8-bit resolution, 122 Hz PWM frequency, 65 kHz timer frequency (prescaler 8)
    PWM_MODE_LED_1000,  // 0-1000 resolution, 125 Hz PWM frequency, 500 kHz timer frequency (prescaler 5)
    
    PWM_MODE_MTR_100,   // 0-100 resolution, 20 kHz PWM frequency, 4MHz timer frequency (prescaler 2)
    PWM_MODE_MTR_255    // 8-bit resolution, 31 kHz PWM frequency, 16MHz timer frequency (prescaler 0)
} nrf_pwm_mode_t;

typedef struct
{
    uint8_t         num_channels;
    uint8_t         gpio_num[3];
    uint8_t         ppi_channel[6];
    uint8_t         gpiote_channel[3];
    uint8_t         mode;
} nrf_pwm_config_t; 

uint32_t nrf_pwm_init(nrf_pwm_config_t *config);

void nrf_pwm_set_value(uint32_t pwm_channel, uint32_t pwm_value);

#ifdef __cplusplus
}
#endif

#endif
