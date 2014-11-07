/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 7 Nov., 2014
 * License: LGPLv3+
 */

#ifndef INCLUDE_DRIVERS_NRF_RTC_H_
#define INCLUDE_DRIVERS_NRF_RTC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t nrf_rtc_init(uint32_t seconds=0);
uint32_t nrf_rtc_config(uint32_t seconds=0);
void nrf_rtc_start();
void nrf_rtc_stop();
uint32_t nrf_rtc_getCount();

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_NRF_RTC_H_ */
