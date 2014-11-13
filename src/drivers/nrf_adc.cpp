/**
 * Author: Bart van Vliet
 * Date: 4 Nov., 2014
 * License: LGPLv3+
 */

#include "drivers/nrf_adc.h"
#include "nrf.h"
#include "nrf_gpio.h"

//#undef NRF51_USE_SOFTDEVICE
//#define NRF51_USE_SOFTDEVICE 0

#if(NRF51_USE_SOFTDEVICE == 1)
#include "nrf_sdm.h"
#endif

#include <common/boards.h>
#include <drivers/serial.h>
#include <util/ble_error.h>

#include <drivers/gpio_api.h>

#include <nRF51822.h>

// allocate buffer struct (not array in buffer yet)
buffer_t<uint16_t> adc_result;
	
// debugging
gpio_t led0;
gpio_t led1;

// FIXME BEWARE, because we are using fixed arrays, increasing the size will cause
//   memory and runtime problems.
#define ADC_BUFFER_SIZE 300
uint16_t adc_buffer[ADC_BUFFER_SIZE];

/**
 * The init function is called once before operating the AD converter. Call it after you start the SoftDevice. Check 
 * the section 31 "Analog to Digital Converter (ADC)" in the nRF51 Series Reference Manual.
 */
uint32_t nrf_adc_init(uint8_t pin) {
#if(NRF51_USE_SOFTDEVICE == 1)
	log(DEBUG, "Run ADC converter with SoftDevice");
#else 
	log(DEBUG, "Run ADC converter without SoftDevice!!!");
	
#endif
	log(DEBUG, "Allocate buffer for ADC results");
	if (adc_result.size != ADC_BUFFER_SIZE) {
		adc_result.size = ADC_BUFFER_SIZE;
//		adc_result.buffer = (uint16_t*)calloc( adc_result.size, sizeof(uint16_t*));
//		if (adc_result.buffer == NULL) {
//			log(FATAL, "Could not initialize buffer. Too big!?");
//			return 0xF0;
//		}
		adc_result.buffer = adc_buffer;
		adc_result.ptr = adc_result.buffer;
	} 
	// set some leds for debugging
	gpio_init_out(&led0, p8);
	gpio_write(&led0, 1);
	gpio_init_out(&led1, p9);

	log(DEBUG, "Init AD converter");
	uint32_t err_code;

	log(DEBUG, "Configure ADC on pin %u", pin);
	err_code = nrf_adc_config(pin);
	APP_ERROR_CHECK(err_code);

	log(DEBUG, "Enable ADC");
	NRF_ADC->EVENTS_END  = 0;    // Stop any running conversions.
	NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Enabled; // Pin will be configured as analog input
	
	log(DEBUG, "Enable Interrupts for END event");
	NRF_ADC->INTENSET   = ADC_INTENSET_END_Msk; // Interrupt adc

	// Enable ADC interrupt
	log(DEBUG, "Clear pending ADC interrupts");
#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_ClearPendingIRQ(ADC_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_ClearPendingIRQ(ADC_IRQn);
#endif

	log(DEBUG, "Set ADC priority");
#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
#endif

	log(DEBUG, "Tell to use ADC interrupt handler");
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
	log(INFO, "Stop ADC sampling");
	NRF_ADC->TASKS_STOP = 1;
}

/**
 * Start the AD converter.
 */
void nrf_adc_start() {
	log(INFO, "Start ADC sampling");
	NRF_ADC->EVENTS_END  = 0;
	NRF_ADC->TASKS_START = 1;
	gpio_write(&led1, 1);
}

/*
 * The interrupt handler for an ADC data ready event.
 */
extern "C" void ADC_IRQHandler(void) {
	uint32_t adc_value;

	// set led for debugging
	//gpio_write(&led0, 0);
	gpio_write(&led1, 0);

	// clear data-ready event
	NRF_ADC->EVENTS_END     = 0;

	// write value to buffer
	adc_value = NRF_ADC->RESULT;
	adc_result.push(adc_value);
	// Log RTC too
	adc_result.push(nrf_rtc_getCount());

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


