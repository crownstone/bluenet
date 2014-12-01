/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "drivers/nrf_adc.h"
#include "nrf.h"

#if(NRF51_USE_SOFTDEVICE == 1)
#include "nrf_sdm.h"
#endif

#include <common/boards.h>
#include <drivers/serial.h>
#include <util/ble_error.h>

#include <nRF51822.h>

// allocate buffer struct (not array in buffer yet)
buffer_t<uint16_t> adc_result;
	
buffer_t<uint16_t>* ADC::getBuffer() {
	return &adc_result;
}

/**
 * The init function is called once before operating the AD converter. Call it after you start the SoftDevice. Check 
 * the section 31 "Analog to Digital Converter (ADC)" in the nRF51 Series Reference Manual.
 */
uint32_t ADC::nrf_adc_init(uint8_t pin) {
#if(NRF51_USE_SOFTDEVICE == 1)
	log(DEBUG, "Run ADC converter with SoftDevice");
#else 
	log(DEBUG, "Run ADC converter without SoftDevice!!!");
	
#endif
	if (adc_result.size != _buffer_size) {
		adc_result.size = _buffer_size;
		adc_result.buffer = (uint16_t*)calloc( adc_result.size, sizeof(uint16_t*));
		if (adc_result.buffer == NULL) {
			log(FATAL, "Could not initialize buffer. Too big!?");
			return 0xF0;
		}
		//adc_result.buffer = adc_buffer;
		adc_result.ptr = adc_result.buffer;
	} 

	uint32_t err_code;

	log(DEBUG, "Configure ADC on pin %u", pin);
	err_code = nrf_adc_config(pin);
	APP_ERROR_CHECK(err_code);

	NRF_ADC->EVENTS_END  = 0;    // Stop any running conversions.
	NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Enabled; // Pin will be configured as analog input
	
	NRF_ADC->INTENSET   = ADC_INTENSET_END_Msk; // Interrupt adc

	// Enable ADC interrupt
#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_ClearPendingIRQ(ADC_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_ClearPendingIRQ(ADC_IRQn);
#endif

#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
#endif

#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_EnableIRQ(ADC_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_EnableIRQ(ADC_IRQn);
#endif
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
uint32_t ADC::nrf_adc_config(uint8_t pin) {
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
void ADC::nrf_adc_stop() {
	NRF_ADC->TASKS_STOP = 1;
}

/**
 * Start the AD converter.
 */
void ADC::nrf_adc_start() {
	NRF_ADC->EVENTS_END  = 0;
	NRF_ADC->TASKS_START = 1;
}

void ADC::update(uint32_t value) {
	adc_result.push(value);
	// Log RTC too
	adc_result.push(nrf_rtc_getCount());

}

void ADC::tick() {
	dispatch();
}

/*
 * The interrupt handler for an ADC data ready event.
 */
extern "C" void ADC_IRQHandler(void) {
	uint32_t adc_value;

	// clear data-ready event
	NRF_ADC->EVENTS_END     = 0;

	// write value to buffer
	adc_value = NRF_ADC->RESULT;

	ADC &adc = ADC::getInstance();
	adc.update(adc_value);

	if (adc_result.full()) {
		//log(INFO, "Buffer is full");
		NRF_ADC->TASKS_STOP = 1;
		//log(INFO, "Stopped task");
	       	return;
	}

	// Use the STOP task to save current. Workaround for PAN_028 rev1.5 anomaly 1.
	//NRF_ADC->TASKS_STOP = 1;

	// next sample
	NRF_ADC->TASKS_START = 1;

}


