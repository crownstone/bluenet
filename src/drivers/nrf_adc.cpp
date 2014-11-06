/**
 * Author: Bart van Vliet
 * Date: 4 Nov., 2014
 * License: LGPLv3+
 */

#include "drivers/nrf_adc.h"
#include "nrf.h"
#include "nrf_gpio.h"
#if(USE_WITH_SOFTDEVICE == 1)
#include "nrf_sdm.h"
#endif

#include <common/boards.h>
#include <drivers/serial.h>
#include <util/ble_error.h>

//static uint32_t* adc_result;

/**
 * The init function is called once before operating the AD converter. Call it after you start the SoftDevice. Check 
 * the section 31 "Analog to Digital Converter (ADC)" in the nRF51 Series Reference Manual.
 */
uint32_t nrf_adc_init(uint8_t pin) {
	log(DEBUG, "Init AD converter");

	uint32_t err_code;
	NRF_ADC->INTENSET   = ADC_INTENSET_END_Msk; // Interrupt adc

#if(USE_WITH_SOFTDEVICE == 1)
	log(DEBUG, "Run ADC converter with SoftDevice");
#endif

	// Enable ADC interrupt
	log(DEBUG, "Clear pending ADC interrupts");
#if(USE_WITH_SOFTDEVICE == 1)
	err_code = sd_nvic_ClearPendingIRQ(ADC_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_ClearPendingIRQ(ADC_IRQn);
#endif

	
	log(DEBUG, "Set ADC priority");
#if(USE_WITH_SOFTDEVICE == 1)
	err_code = sd_nvic_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
	APP_ERROR_CHECK(err_code);
#else
//	NVIC_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
	NVIC_SetPriority(ADC_IRQn, 3);
#endif

	log(DEBUG, "Enable ADC Interrupt");
#if(USE_WITH_SOFTDEVICE == 1)
	err_code = sd_nvic_EnableIRQ(ADC_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_EnableIRQ(ADC_IRQn);
#endif
	
	log(DEBUG, "Configure ADC");
	err_code = nrf_adc_config(pin);
	APP_ERROR_CHECK(err_code);
	
	log(DEBUG, "Start ADC task");
	NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Enabled; // Pin will be configured as analog input
	NRF_ADC->EVENTS_END  = 0;    // Stop any running conversions.
	//NRF_ADC->TASKS_START = 1;    // If we start the task we cannot use the radio...

	return 0;
}

/**
 * Configure the AD converter.
 *   - set the resolution to 10 bits
 *   - set the prescaler for the input voltage (the input, not the input supply) 
 *   - use the internal VGB reference (not the external one, so no need to use its multiplexer either)
 *   - do not set the prescaler for the reference voltage, this means voltage is expected between 0 and 1.2V (VGB)
 * The prescaler for input is set to 1/3. This means that the AIN input can be from 0 to 3.6V.
 */
uint32_t nrf_adc_config(uint8_t pin) {
	NRF_ADC->CONFIG     =
			(ADC_CONFIG_RES_10bit                            << ADC_CONFIG_RES_Pos)     |
			(ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos)  | 
			(ADC_CONFIG_REFSEL_VBG                           << ADC_CONFIG_REFSEL_Pos)  |
			(ADC_CONFIG_EXTREFSEL_None                       << ADC_CONFIG_EXTREFSEL_Pos);
	if (pin < 8) {
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput0 << (pin+ADC_CONFIG_PSEL_Pos);
	} else {
		log(FATAL, "There is no such pin available");
		return 0xFFFFFFFF; // error
	}
	return 0;
}

/**
 * Stop the AD converter.
 */
void nrf_adc_stop() {
	NRF_ADC->TASKS_STOP = 1;
}

/**
 * This doesn't work, correctly. Must be done properly.
 */
uint32_t nrf_adc_read(uint8_t pin, uint32_t* result) {
//	adc_result = result;
	NRF_ADC->INTENSET   = ADC_INTENSET_END_Msk; // Interrupt adc

	//Anne: huh, config again!?!! if (nrf_adc_config(pin)) return 0xFFFFFFFF; // error
	
	NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Enabled; // Pin will be configured as analog input
	NRF_ADC->EVENTS_END  = 0;    // Stop any running conversions.
	NRF_ADC->TASKS_START = 1;

	while (!NRF_ADC->EVENTS_END) {} // block
	NRF_ADC->EVENTS_END  = 0;
	*result = NRF_ADC->RESULT;
	NRF_ADC->TASKS_STOP = 1;
	return 0;
}

/*
 * The interrupt handler for an ADC data ready event.
 */
void ADC_IRQHandler(void) {
	if (NRF_ADC->EVENTS_END != 0) {
		uint32_t     adc_result;
//		uint16_t    batt_lvl_in_milli_volts;
//		uint8_t     percentage_batt_lvl;
		uint32_t    err_code;

		NRF_ADC->EVENTS_END     = 0;
		adc_result             = NRF_ADC->RESULT;

		// Use the STOP task to save current. Workaround for PAN_028 rev1.5 anomaly 1.
		NRF_ADC->TASKS_STOP = 1;
	}
}

