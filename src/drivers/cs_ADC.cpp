/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "drivers/cs_ADC.h"

//#include "nrf.h"
//
//
//#if(NRF51_USE_SOFTDEVICE == 1)
//#include "nrf_sdm.h"
//#endif
//
#include "cfg/cs_Boards.h"
#include "drivers/cs_Serial.h"
#include "util/cs_BleError.h"
//
//#include "cs_nRF51822.h"

#include "drivers/cs_RTC.h"

//! Check the section 31 "Analog to Digital Converter (ADC)" in the nRF51 Series Reference Manual.
uint32_t ADC::init(uint8_t pins[], uint8_t numPins) {
	uint32_t err_code;
	assert(numPins <= CS_ADC_MAX_PINS && numPins > 0, "Too many or few pins");
	for (uint8_t i=0; i<numPins; i++) {
		_pins[i] = pins[i];
	}
	_numPins = numPins;

	LOGd("Configure ADC with %u pins, starting on pin %u", numPins, pins[0]);
	err_code = config(0);
	APP_ERROR_CHECK(err_code);

	NRF_ADC->EVENTS_END = 0;    //! Stop any running conversions.
	NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Enabled; //! Pin will be configured as analog input
	NRF_ADC->INTENSET   = ADC_INTENSET_END_Msk; //! Interrupt adc

	//! Enable ADC interrupt
#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_ClearPendingIRQ(ADC_IRQn);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_EnableIRQ(ADC_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_ClearPendingIRQ(ADC_IRQn);
	NVIC_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
	NVIC_EnableIRQ(ADC_IRQn);
#endif

	//! Configure timer
	CS_ADC_TIMER->TASKS_CLEAR = 1;
	CS_ADC_TIMER->BITMODE =    (TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos); //! Counter is 16bit
	CS_ADC_TIMER->MODE =       (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
	CS_ADC_TIMER->PRESCALER =  (4 << TIMER_PRESCALER_PRESCALER_Pos); //! 16MHz / 2^4 = 1Mhz, 1us period

	//! Configure timer events
	CS_ADC_TIMER->CC[0] = 1000*1000/CS_ADC_SAMPLE_RATE;

	//! Shortcut clear timer at compare0 event
	CS_ADC_TIMER->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);

	//! Configure ADC start task via PPI
#if(NRF51_USE_SOFTDEVICE == 1)
	sd_ppi_channel_assign(CS_ADC_PPI_CHANNEL, &CS_ADC_TIMER->EVENTS_COMPARE[0], &NRF_ADC->TASKS_START);
	sd_ppi_channel_enable_set(1UL << CS_ADC_PPI_CHANNEL);
#else
	NRF_PPI->CH[CS_ADC_PPI_CHANNEL].EEP = (uint32_t)&CS_ADC_TIMER->EVENTS_COMPARE[0];
	NRF_PPI->CH[CS_ADC_PPI_CHANNEL].TEP = (uint32_t)&NRF_ADC->TASKS_START;
	NRF_PPI->CHENSET = (1UL << CS_ADC_PPI_CHANNEL);
#endif

//	_sampleNum = 0;
	return 0;
}

bool ADC::setBuffers(StackBuffer<uint16_t>* buffer, uint8_t pinNum) {
	if (pinNum >= _numPins) {
		return false;
	}
	LOGd("Set buffer of pin %u at %u", pinNum, buffer);
	_buffers[pinNum] = buffer;
	return true;
}

bool ADC::setBuffers(CircularBuffer<uint16_t>* buffer, uint8_t pinNum) {
	if (pinNum >= _numPins) {
		return false;
	}
	LOGd("Set buffer of pin %u at %u", pinNum, buffer);
	_circularBuffers[pinNum] = buffer;
	return true;
}

bool ADC::setTimestampBuffers(DifferentialBuffer<uint32_t>* buffer, uint8_t pinNum) {
	if (pinNum >= _numPins) {
		return false;
	}
	LOGd("Set buffer of pin %u at %u", pinNum, buffer);
	_timeBuffers[pinNum] = buffer;
	return true;
}

void ADC::setDoneCallback(adc_done_cb_t callback) {
	_doneCallback = callback;
}

/** Configure the AD converter.
 *
 *   - set the resolution to 10 bits
 *   - set the prescaler for the input voltage (the input, not the input supply)
 *   - use the internal VGB reference (not the external one, so no need to use its multiplexer either)
 *   - do not set the prescaler for the reference voltage, this means voltage is expected between 0 and 1.2V (VGB)
 * The prescaler for input is set to 1/3. This means that the AIN input can be from 0 to 3.6V.
 */
uint32_t ADC::config(uint8_t pinNum) {
	NRF_ADC->CONFIG     =
			(ADC_CONFIG_RES_10bit                            << ADC_CONFIG_RES_Pos)     |
#if(HARDWARE_BOARD==CROWNSTONE)
//#if(HARDWARE_BOARD==CROWNSTONE || HARDWARE_BOARD==CROWNSTONE4 || HARDWARE_BOARD==CROWNSTONE5)
			(ADC_CONFIG_INPSEL_AnalogInputNoPrescaling       << ADC_CONFIG_INPSEL_Pos)  |
#else
			(ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos)  |
#endif
			(ADC_CONFIG_REFSEL_VBG                           << ADC_CONFIG_REFSEL_Pos)  |
			(ADC_CONFIG_EXTREFSEL_None                       << ADC_CONFIG_EXTREFSEL_Pos);
	assert(_pins[pinNum] < 8, "No such pin");
	NRF_ADC->CONFIG |= ADC_CONFIG_PSEL_AnalogInput0 << (_pins[pinNum] + ADC_CONFIG_PSEL_Pos);
	_lastPinNum = pinNum;
	return 0;
}

void ADC::stop() {
	CS_ADC_TIMER->TASKS_STOP = 1;
	NRF_ADC->TASKS_STOP = 1;
//	NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Disabled;
}

void ADC::start() {
//	NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Enabled;
	config(0);
	NRF_ADC->EVENTS_END  = 0;
//	NRF_ADC->TASKS_START = 1;
	CS_ADC_TIMER->TASKS_START = 1;
}

void adc_done(void * p_event_data, uint16_t event_size) {
	(*(adc_done_cb_t*)p_event_data)();
}

void ADC::update(uint32_t value) {
	if (_circularBuffers[_lastPinNum] != NULL) {
		_circularBuffers[_lastPinNum]->push(value);
	}
	else if (_buffers[_lastPinNum] != NULL) {
		if (!_buffers[_lastPinNum]->push(value)) {
//		if (!_buffers[_lastPinNum]->push(_buffers[_lastPinNum]->size())) {
			//! If this buffer is full, stop sampling
			// todo: can we trigger an event here that the sample is ready instead of
			//   having a tick function in the PowerSampler that polls if the buffers are full?
			//   either using a
			//      - timer
			//      - app_scheduler_put
			if (_doneCallback) {
				// decouple done callback from adc interrupt handler, and put it on app scheduler
				// instead
				app_sched_event_put(&_doneCallback, sizeof(_doneCallback), adc_done);
			}
			stop();
			return;
		}
		if (_timeBuffers[_lastPinNum] != NULL) {
			if (!_timeBuffers[_lastPinNum]->push(RTC::getCount())) {
				//! Difference was too large: clear all buffers of this pin nr?
				_buffers[_lastPinNum]->clear();
			}
		}
	}

	//! Next pin
	config((_lastPinNum+1) % _numPins);
	if (_lastPinNum == 0) {
		//! Sampled last pin of the list, use the STOP task to save current. Workaround for PAN_028 rev1.5 anomaly 1.
		NRF_ADC->TASKS_STOP = 1;
	}
	else {
		//! Sample next pin
		NRF_ADC->TASKS_START = 1;
	}
}

/** The interrupt handler for an ADC data ready event.
 */
extern "C" void ADC_IRQHandler(void) {
	uint32_t adc_value;

	//! Clear data-ready event
	NRF_ADC->EVENTS_END = 0;

	//! Get value
	adc_value = NRF_ADC->RESULT;
	ADC &adc = ADC::getInstance();
	adc.update(adc_value);

//	//! Use the STOP task to save current. Workaround for PAN_028 rev1.5 anomaly 1.
//	NRF_ADC->TASKS_STOP = 1;

//	//! next sample
//	NRF_ADC->TASKS_START = 1;

}


