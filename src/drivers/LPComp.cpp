/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "drivers/LPComp.h"

#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

#if(NRF51_USE_SOFTDEVICE == 1)
#include "nrf_sdm.h"
#endif

#include <common/boards.h>
#include <drivers/serial.h>
#include <util/ble_error.h>

#include <nRF51822.h>

LPComp::LPComp() {

}

LPComp::~LPComp() {
	stop();
}

/**
 * The init function is called once before operating the LPComp. Call it after you start the SoftDevice. Check
 * the section 32 "Low Power Comparator (LPCOMP)" in the nRF51 Series Reference Manual.
 *   - select the input pin (analog in)
 *   - use the internal VDD reference
 * The level can be set to from 0 to 6 , making it compare to 1/8 to 7/8 of the supply voltage.
 */
uint32_t LPComp::config(uint8_t pin, uint8_t level, Event_t event) {
#if(NRF51_USE_SOFTDEVICE == 1)
	LOGd("Run LPComp with SoftDevice");
#else 
	LOGd("Run LPComp without SoftDevice!!!");
#endif

	// Enable interrupt on given event
	switch (event) {
	case (CROSS):
		NRF_LPCOMP->INTENSET = LPCOMP_INTENSET_CROSS_Enabled << LPCOMP_INTENSET_CROSS_Pos;
		break;
	case (UP):
		NRF_LPCOMP->INTENSET = LPCOMP_INTENSET_UP_Enabled << LPCOMP_INTENSET_UP_Pos;
		break;
	case (DOWN):
		NRF_LPCOMP->INTENSET = LPCOMP_INTENSET_DOWN_Enabled << LPCOMP_INTENSET_DOWN_Pos;
		break;
	default:
		LOGf("Choose an event to listen for");
		APP_ERROR_CHECK(0xFFFFFFFF); // error
		break;
	}

	// Enable LPComp interrupt
	uint32_t err_code;
#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_ClearPendingIRQ(LPCOMP_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_ClearPendingIRQ(LPCOMP_IRQn);
#endif

#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_SetPriority(LPCOMP_IRQn, NRF_APP_PRIORITY_LOW);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_SetPriority(LPCOMP_IRQn, NRF_APP_PRIORITY_LOW);
#endif

#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_EnableIRQ(LPCOMP_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_EnableIRQ(LPCOMP_IRQn);
#endif

	LOGd("Configure LPComp on ain %u, level %u", pin, level);
	if (level < 7) {
		NRF_LPCOMP->REFSEL = level; // See LPCOMP_REFSEL_REFSEL_SupplyOneEighthPrescaling
	}
	else {
		LOGf("There is no such level");
		APP_ERROR_CHECK(0xFFFFFFFF); // error
	}
	if (pin < 8) {
		NRF_LPCOMP->PSEL = pin; // See LPCOMP_PSEL_PSEL_AnalogInput0
	} else {
		LOGf("There is no such pin available");
		APP_ERROR_CHECK(0xFFFFFFFF); // error
	}

//	// TODO: don't know if we need this..
//	// Wait for the LCOMP config to have effect
//	nrf_delay_us(100);
//
//
//	// TODO: don't know if we need this..
//	// Wait for the LCOMP config to have effect
//	nrf_delay_us(100);

	NRF_LPCOMP->POWER = LPCOMP_POWER_POWER_Enabled << LPCOMP_POWER_POWER_Pos;
	NRF_LPCOMP->ENABLE = LPCOMP_ENABLE_ENABLE_Enabled << LPCOMP_ENABLE_ENABLE_Pos; // Pin will be configured as analog input

	return 0;
}

/**
 * Stop the LP comparator.
 */
void LPComp::stop() {
	// TODO: should we clear pending interrupts?
	#if(NRF51_USE_SOFTDEVICE == 1)
		APP_ERROR_CHECK(sd_nvic_ClearPendingIRQ(LPCOMP_IRQn));
	#else
		NVIC_ClearPendingIRQ(LPCOMP_IRQn);
	#endif
	NRF_LPCOMP->TASKS_STOP = 1;
}

/**
 * Start the LP comparator.
 */
void LPComp::start() {
	LOGd("Starting LPComp");
	NRF_LPCOMP->EVENTS_UP = 0;
	NRF_LPCOMP->EVENTS_DOWN = 0;
	NRF_LPCOMP->EVENTS_CROSS = 0;
	lastEvent = NONE;
	NRF_LPCOMP->TASKS_START = 1;
}


void LPComp::update(Event_t event) {
	lastEvent = event;
//	// Test
//	LOGd("update!");
//	nrf_pwm_set_value(0, 0);
}

void LPComp::tick() {
	if (lastEvent != NONE) {
		dispatch();
		lastEvent = NONE;
	}
}


/*
 * The interrupt handler for the LCOMP events.
 * name defined in nRF51822.c
 */
extern "C" void WUCOMP_COMP_IRQHandler(void) {
//	{
//		LPComp &lpcomp = LPComp::getInstance();
//		lpcomp.update(LPComp::NONE);
//	}
	if ((NRF_LPCOMP->INTENSET & LPCOMP_INTENSET_CROSS_Msk) != 0 && NRF_LPCOMP->EVENTS_CROSS != 0) {
		LPComp &lpcomp = LPComp::getInstance();
		lpcomp.update(LPComp::CROSS);

		// Continue
		NRF_LPCOMP->EVENTS_CROSS = 0;
//		NRF_LPCOMP->TASKS_START = 1; // Do we need this?
	}
	else if ((NRF_LPCOMP->INTENSET & LPCOMP_INTENSET_UP_Msk) != 0 && NRF_LPCOMP->EVENTS_UP != 0) {
		LPComp &lpcomp = LPComp::getInstance();
		lpcomp.update(LPComp::UP);

		// Continue
		NRF_LPCOMP->EVENTS_UP = 0;
//		NRF_LPCOMP->TASKS_START = 1; // Do we need this?
	}
	else if ((NRF_LPCOMP->INTENSET & LPCOMP_INTENSET_DOWN_Msk) != 0 && NRF_LPCOMP->EVENTS_DOWN > 0) {
		LPComp &lpcomp = LPComp::getInstance();
		lpcomp.update(LPComp::DOWN);

		// Continue
		NRF_LPCOMP->EVENTS_DOWN = 0;
//		NRF_LPCOMP->TASKS_START = 1; // Do we need this?
	}
}


