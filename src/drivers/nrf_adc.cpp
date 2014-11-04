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

uint32_t nrf_adc_config(uint8_t pin) {
	// Configure ADC
	NRF_ADC->CONFIG     =
			(ADC_CONFIG_RES_10bit                            << ADC_CONFIG_RES_Pos)     |
//			(ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling      << ADC_CONFIG_INPSEL_Pos)  | // Useful to measure supply voltage (disable input pin)
//			(ADC_CONFIG_INPSEL_AnalogInputNoPrescaling       << ADC_CONFIG_INPSEL_Pos)  |
			(ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos)  | // Multiply read value by 3, useful to read out higher voltages
			(ADC_CONFIG_REFSEL_VBG                           << ADC_CONFIG_REFSEL_Pos)  |
			(ADC_CONFIG_EXTREFSEL_None                       << ADC_CONFIG_EXTREFSEL_Pos);
#ifdef NRF6310_BOARD
	// --------- TEST -------------
	NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput0 << ADC_CONFIG_PSEL_Pos;
	NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput1 << ADC_CONFIG_PSEL_Pos;
	NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput2 << ADC_CONFIG_PSEL_Pos;
	NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput3 << ADC_CONFIG_PSEL_Pos;
	NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput4 << ADC_CONFIG_PSEL_Pos;
	NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput5 << ADC_CONFIG_PSEL_Pos;
	NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput6 << ADC_CONFIG_PSEL_Pos;
	NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput7 << ADC_CONFIG_PSEL_Pos;
#else
	if (pin < 8) {
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput0 << (pin+ADC_CONFIG_PSEL_Pos);
	}
	else {
		return 0xFFFFFFFF; // error
//		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_Disabled << ADC_CONFIG_PSEL_Pos;
	}
/*
	switch (pin) {
	case 0:
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput0 << ADC_CONFIG_PSEL_Pos;
		break;
	case 1:
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput1 << ADC_CONFIG_PSEL_Pos;
		break;
	case 2:
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput2 << ADC_CONFIG_PSEL_Pos;
		break;
	case 3:
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput3 << ADC_CONFIG_PSEL_Pos;
		break;
	case 4:
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput4 << ADC_CONFIG_PSEL_Pos;
		break;
	case 5:
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput5 << ADC_CONFIG_PSEL_Pos;
		break;
	case 6:
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput6 << ADC_CONFIG_PSEL_Pos;
		break;
	case 7:
		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput7 << ADC_CONFIG_PSEL_Pos;
		break;
	default:
		return 0xFFFFFFFF; // error
//		NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_Disabled << ADC_CONFIG_PSEL_Pos;
	}
*/
#endif

	return 0;
}

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

#ifdef NRF6310_BOARD
	log(DEBUG, "Test pins");
	// --------- TEST -------------
	// Configure pins as outputs.
	for (uint8_t i=8; i<16; ++i) {
		nrf_gpio_cfg_output(i);
		nrf_gpio_pin_write(i, 1); // led goes off
	}
#endif

	return 0;
}

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



void ADC_IRQHandler(void) {
	if (NRF_ADC->EVENTS_END != 0) {
		uint32_t     adc_result;
//		uint16_t    batt_lvl_in_milli_volts;
//		uint8_t     percentage_batt_lvl;
		uint32_t    err_code;

#ifdef NRF6310_BOARD
		for (uint8_t i=8, j=0; i<16; ++i, ++j) {
			uint32_t comp = 1;
			if (NRF_ADC->EVENTS_END && comp<<(24+j))
				nrf_gpio_pin_write(i, 1); // led goes on
			else
				nrf_gpio_pin_write(i, 0); // led goes off
		}
#endif

		NRF_ADC->EVENTS_END     = 0;
		adc_result             = NRF_ADC->RESULT;
//		NRF_ADC->TASKS_STOP     = 1;


//		batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result) +
//				DIODE_FWD_VOLT_DROP_MILLIVOLTS;
//		percentage_batt_lvl     = battery_level_in_percent(batt_lvl_in_milli_volts);
//
//		err_code = ble_bas_battery_level_update(&m_bas, percentage_batt_lvl);
//		if (
//				(err_code != NRF_SUCCESS) &&
//				(err_code != NRF_ERROR_INVALID_STATE) &&
//				(err_code != BLE_ERROR_NO_TX_BUFFERS) &&
//				(err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)) {
//			APP_ERROR_HANDLER(err_code);
//		}

	}
}

