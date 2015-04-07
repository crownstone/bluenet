/**
 * Author: Anne van Rossum
 * Copyrights: Distributed Organism B.V. (DoBots, http://dobots.nl)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "common/cs_Timer.h"
#include "nrf.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"

#define LFCLK_FREQUENCY           (32768UL)                               /**< LFCLK frequency in Hertz, constant. */

/**
 * Code to set a timer for so many milliseconds. Will be used to calculate the next semi-cycle in a 50Hz signal, so
 * after 10ms. This uses the slow RTC timer (more energy efficient if there is a slow crystal).
 *
 * The RTC1 clock is used, because RTC0 is used by the softdevice.
 */


/** @brief Function for configuring the RTC with TICK to 100Hz and COMPARE0 to 10 sec.
*/
void timerConfig(uint8_t ms)
{
	_timerFlag = 0;

	// enable the interrupt 
	NVIC_EnableIRQ(RTC1_IRQn); 

	// set exact number of ticks
	NRF_RTC1->PRESCALER = ms * LFCLK_FREQUENCY / 0xFFFFFFUL; 
	NRF_RTC1->CC[0]     = ms * LFCLK_FREQUENCY / (NRF_RTC1->PRESCALER + 1);

	// Enable TICK event and TICK interrupt:
	//NRF_RTC1->EVTENSET = RTC_EVTENSET_TICK_Msk;
	//NRF_RTC1->INTENSET = RTC_INTENSET_TICK_Msk;

	// Enable COMPARE0 event and COMPARE0 interrupt:
	NRF_RTC1->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
	NRF_RTC1->INTENSET = RTC_INTENSET_COMPARE0_Msk;
}

void timerStart() {
	NRF_RTC1->TASKS_CLEAR = 1;
	NRF_RTC1->TASKS_START = 1;
}

void timerDisable() {
	NRF_RTC1->INTENSET = 0;
	NRF_RTC1->TASKS_STOP = 1;
}

/** @brief: Function for handling the RTC1 interrupts.
 * Triggered on TICK and COMPARE0 match.
 */
/*
extern "C" void RTC1_IRQHandler()
{
	if ((NRF_RTC1->EVENTS_TICK != 0) &&
			((NRF_RTC1->INTENSET & RTC_INTENSET_TICK_Msk) != 0))
	{
		NRF_RTC1->EVENTS_TICK = 0;
		nrf_gpio_pin_toggle(GPIO_TOGGLE_TICK_EVENT);
	}

	if ((NRF_RTC1->EVENTS_COMPARE[0] != 0) &&
			((NRF_RTC1->INTENSET & RTC_INTENSET_COMPARE0_Msk) != 0))
	{
		NRF_RTC1->EVENTS_COMPARE[0] = 0;
		nrf_gpio_pin_write(GPIO_TOGGLE_COMPARE_EVENT, 1);
	}
} */

extern "C" void RTC1_IRQHandler()
{
	if ((NRF_RTC1->EVENTS_COMPARE[0] != 0) && ((NRF_RTC1->INTENSET & RTC_INTENSET_COMPARE0_Msk) != 0))
	{
		_timerFlag = 1;
		NRF_RTC1->EVENTS_COMPARE[0] = 0;
		//NRF_RTC1->INTENSET = 0;
		NRF_RTC1->TASKS_STOP = 1;
	}
}
