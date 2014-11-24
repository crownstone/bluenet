/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef INCLUDE_DRIVERS_NRF_RTC_H_
#define INCLUDE_DRIVERS_NRF_RTC_H_

#include <stdint.h>

// TODO: do we really need this?
//#ifdef __cplusplus
//extern "C" {
//#endif

// make callback
static int rtc_timer_flag;

uint32_t nrf_rtc_init(uint32_t ms=0);
//uint32_t nrf_rtc_config(uint32_t ms=0);
void nrf_rtc_start();
void nrf_rtc_stop();
uint32_t nrf_rtc_getCount();

//#ifdef __cplusplus
//}
//#endif

#endif /* INCLUDE_DRIVERS_NRF_RTC_H_ */
