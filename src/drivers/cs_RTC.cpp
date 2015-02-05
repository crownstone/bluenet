/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 7 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "nrf.h"

#include "drivers/cs_RTC.h"

#if(NRF51_USE_SOFTDEVICE == 1)
#include "nrf_sdm.h"
#endif

#include "drivers/cs_Serial.h"
#include "util/cs_BleError.h"
// what for? #include <nRF51822.h>

#define LFCLK_FREQUENCY (32768UL)                               /**< LFCLK frequency in Hertz, constant. */

// bounded to local compilation unit
static int rtc_timer_flag = 0;

uint32_t RealTimeClock::init(uint32_t ms) {
	uint32_t err_code = 0;

	// Enable ADC interrupt
	LOGd("Clear pending RTC interrupts");
#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_ClearPendingIRQ(RTC1_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_ClearPendingIRQ(RTC1_IRQn);
#endif

	LOGd("Set RTC priority");
#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_SetPriority(RTC1_IRQn, NRF_APP_PRIORITY_LOW);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_SetPriority(RTC1_IRQn, NRF_APP_PRIORITY_LOW);
#endif

	LOGd("Tell to use RTC interrupt handler");
#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_EnableIRQ(RTC1_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_EnableIRQ(RTC1_IRQn);
#endif

	if (ms) {
		// 0xFFFFFF = 16777215
		NRF_RTC1->PRESCALER = ms * LFCLK_FREQUENCY / 0xFFFFFFUL; // 12-bit prescaler for COUNTER frequency (32768/(PRESCALER+1)).
		NRF_RTC1->CC[0] = ms * LFCLK_FREQUENCY / (NRF_RTC1->PRESCALER + 1) / 1000;
		// Enable COMPARE0 event and COMPARE0 interrupt:
		NRF_RTC1->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
		NRF_RTC1->INTENSET = RTC_INTENSET_COMPARE0_Msk;
	}
	else {
		// 24 bit counter --> 2^24 / (32768/(prescaler+1)) = 512 seconds to overflow
		NRF_RTC1->PRESCALER = 0; // 12-bit prescaler for COUNTER frequency (32768/(PRESCALER+1)).
		// Enable TICK event and TICK interrupt:
		NRF_RTC1->EVTENSET = RTC_EVTENSET_TICK_Msk;
		NRF_RTC1->INTENSET = RTC_INTENSET_TICK_Msk;
	}

	return 0;
}

//uint32_t RealTimeClock::config(uint32_t ms) {
//	return 0;
//}

void RealTimeClock::start() {
	// Clear all events
	NRF_RTC1->EVENTS_TICK = 0;
	NRF_RTC1->EVENTS_COMPARE[0] = 0;
	NRF_RTC1->EVENTS_COMPARE[1] = 0;
	NRF_RTC1->EVENTS_COMPARE[2] = 0;
	NRF_RTC1->EVENTS_COMPARE[3] = 0;
	NRF_RTC1->EVENTS_OVRFLW = 0;
	rtc_timer_flag = 0;
	// Start the clock
	NRF_RTC1->TASKS_START = 1;
}

void RealTimeClock::stop() {
	NRF_RTC1->TASKS_STOP = 1;
	//	nrf_delay_us(100);
	NRF_RTC1->TASKS_CLEAR = 1;
}

void RealTimeClock::tick() {
	dispatch();
}

/*
 * The interrupt handler for an RTC data ready event.
 */
extern "C" void RTC1_IRQHandler(void) {

	// Tick event
	if (NRF_RTC1->EVENTS_TICK != 0 && (NRF_RTC1->INTENSET & RTC_INTENSET_TICK_Msk) != 0) {
		NRF_RTC1->EVENTS_TICK = 0;
	}

	// Compare event
	if (NRF_RTC1->EVENTS_COMPARE[0] != 0 && (NRF_RTC1->INTENSET & RTC_INTENSET_COMPARE0_Msk) != 0) {
		NRF_RTC1->EVENTS_COMPARE[0] = 0;
		rtc_timer_flag = 1;
		NRF_RTC1->TASKS_STOP = 1;
		//    	NRF_ADC->TASKS_START = 1;
	}
}

uint32_t RealTimeClock::getCount() {
	uint32_t count = NRF_RTC1->COUNTER;
	return count;
}
