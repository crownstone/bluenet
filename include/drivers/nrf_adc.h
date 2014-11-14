/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#ifndef __NRF_ADC__H__
#define __NRF_ADC__H__

#include <stdint.h>

#include <drivers/nrf_rtc.h>

#include <common/buffer.h>

// TODO: do we really need this?
//#ifdef __cplusplus
//extern "C" {
//#endif

// declare buffer for ADC results
extern buffer_t<uint16_t> adc_result;

uint32_t nrf_adc_init(uint8_t pin);
uint32_t nrf_adc_config(uint8_t pin);
void nrf_adc_start();
void nrf_adc_stop();

//uint32_t nrf_adc_read(uint8_t pin, uint32_t* result);


//#ifdef __cplusplus
//}
//#endif

#endif
