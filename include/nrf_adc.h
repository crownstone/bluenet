#ifndef __NRF_ADC__H__
#define __NRF_ADC__H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Set this to 1 if the application uses a SoftDevice, 0 otherwise
#define USE_WITH_SOFTDEVICE 0

//#define NRF6310_BOARD

uint32_t nrf_adc_config(uint8_t pin);
uint32_t nrf_adc_init(uint8_t pin);
void nrf_adc_stop();

uint32_t nrf_adc_read(uint8_t pin, uint32_t* result);


#ifdef __cplusplus
}
#endif

#endif
