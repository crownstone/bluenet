/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "drivers/cs_ADC.h"

#include <nrf.h>
#include <app_util_platform.h>

#include "cfg/cs_Boards.h"
#include "drivers/cs_Serial.h"
#include "util/cs_BleError.h"
#include "drivers/cs_RTC.h"
#include "cfg/cs_Strings.h"
#include "cfg/cs_Config.h"

//#define PRINT_ADC_VERBOSE
extern "C" void saadc_callback(nrf_drv_saadc_evt_t const * p_event);

ADC::ADC(): _timer(NULL)
{
	_timer = new nrf_drv_timer_t();
	_timer->p_reg = CS_ADC_TIMER; // Or use CONCAT_2(NRF_TIMER, CS_ADC_TIMER_ID)
	_timer->instance_id = CS_ADC_INSTANCE_INDEX; // Or use CONCAT_3(TIMER, CS_ADC_TIMER_ID, _INSTANCE_INDEX)
	_timer->cc_channel_count = NRF_TIMER_CC_CHANNEL_COUNT(CS_ADC_TIMER_ID);

	for (int i=0; i<CS_ADC_NUM_BUFFERS; i++) {
		_bufferPointers[i] = NULL;
	}
	for (int i=0; i<CS_ADC_MAX_PINS; i++) {
		_stackBuffers[i] = NULL;
		_timeBuffers[i] = NULL;
		_circularBuffers[i] = NULL;
	}

}

uint32_t ADC::init(uint8_t pins[], uint8_t numPins) {
	ret_code_t err_code;

	for (uint8_t i=0; i<numPins; i++) {
		_pins[i] = pins[i];
	}
	_numPins = numPins;

	err_code = nrf_drv_ppi_init();
	if (err_code != MODULE_ALREADY_INITIALIZED) {
		APP_ERROR_CHECK(err_code);
	}

	nrf_drv_timer_config_t timerConfig = {
		.frequency          = NRF_TIMER_FREQ_16MHz,
		.mode               = (nrf_timer_mode_t)TIMER_MODE_MODE_Timer,
		.bit_width          = (nrf_timer_bit_width_t)TIMER_BITMODE_BITMODE_32Bit,
		.interrupt_priority = APP_IRQ_PRIORITY_LOW,
		.p_context          = NULL
	};

	err_code = nrf_drv_timer_init(_timer, &timerConfig, staticTimerHandler);
	APP_ERROR_CHECK(err_code);

	//! Setup timer for compare event every 100ms
//	uint32_t ticks = nrf_drv_timer_ms_to_ticks(_timer, 100);
	//! Setup timer for compare event every 100us
	uint32_t ticks = nrf_drv_timer_us_to_ticks(_timer, 500);
//	LOGd("adc timer ticks: %u", ticks);
	nrf_drv_timer_extended_compare(_timer, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
//	nrf_drv_timer_extended_compare(_timer, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
//	nrf_drv_timer_enable(_timer);

	uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(_timer, NRF_TIMER_CC_CHANNEL0);
	uint32_t saadc_sample_event_addr = nrf_drv_saadc_sample_task_get();

	//! Setup ppi channel so that timer compare event is triggering sample task in SAADC
	err_code = nrf_drv_ppi_channel_alloc(&_ppiChannel);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_ppi_channel_assign(_ppiChannel, timer_compare_event_addr, saadc_sample_event_addr);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_ppi_channel_enable(_ppiChannel);
	APP_ERROR_CHECK(err_code);

	//! Config adc
	nrf_drv_saadc_config_t adcConfig = {
		.resolution         = NRF_SAADC_RESOLUTION_12BIT, //! 14 bit can only be achieved with oversampling
		.oversample         = NRF_SAADC_OVERSAMPLE_DISABLED, //! Oversampling can only be used when sampling 1 channel
		.interrupt_priority = APP_IRQ_PRIORITY_LOW
	};

	err_code = nrf_drv_saadc_init(&adcConfig, saadc_callback);
	APP_ERROR_CHECK(err_code);

	for (int i=0; i<numPins; i++) {
		configPin(pins[i]);
	}

	for (int i=0; i<CS_ADC_NUM_BUFFERS; i++) {
		_bufferPointers[i] = new nrf_saadc_value_t[CS_ADC_BUF_SIZE];
		err_code = nrf_drv_saadc_buffer_convert(_bufferPointers[i], CS_ADC_BUF_SIZE);
		APP_ERROR_CHECK(err_code);
	}

	return 0;
}

#define CS_ADC_PIN_P(pinNum) CONCAT_2(NRF_SAADC_INPUT_AIN, pinNum)

/** Configure the AD converter.
 *
 *   - set the resolution to 10 bits
 *   - set the prescaler for the input voltage (the input, not the input supply)
 *   - use the internal VGB reference (not the external one, so no need to use its multiplexer either)
 *   - do not set the prescaler for the reference voltage, this means voltage is expected between 0 and 1.2V (VGB)
 * The prescaler for input is set to 1/3. This means that the AIN input can be from 0 to 3.6V.
 */
uint32_t ADC::configPin(uint8_t pinNum) {

	ret_code_t err_code;

	nrf_saadc_channel_config_t channelConfig = {
		.resistor_p = NRF_SAADC_RESISTOR_DISABLED,
		.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
		.gain       = NRF_SAADC_GAIN1_2, //! gain is 0.5, maps [0, 1.2] to [0, 0.6]
//		.gain       = NRF_SAADC_GAIN1_6, //! gain is 1/6, maps [0, 3.6] to [0, 0.6]
		.reference  = NRF_SAADC_REFERENCE_INTERNAL, //! 0.6V
		.acq_time   = NRF_SAADC_ACQTIME_10US, //! 10 micro seconds (10e-6 seconds)
		.mode       = NRF_SAADC_MODE_SINGLE_ENDED,
		.pin_p      = (nrf_saadc_input_t)(pinNum+1),
//		.pin_p      = CONCAT_2(NRF_SAADC_INPUT_AIN, pinNum),
//		.pin_p      = nrf_drv_saadc_gpio_to_ain(_pins(pinNum)),
		.pin_n      = NRF_SAADC_INPUT_DISABLED
	};

	err_code = nrf_drv_saadc_channel_init(pinNum, &channelConfig);
	APP_ERROR_CHECK(err_code);

	return 0;
}

bool ADC::setBuffers(StackBuffer<uint16_t>* buffer, uint8_t pinNum) {
	if (pinNum >= _numPins) {
		return false;
	}
#ifdef PRINT_ADC_VERBOSE
	LOGd("Set buffer of pin %u at %u", pinNum, buffer);
#endif
	_stackBuffers[pinNum] = buffer;
	return true;
}

bool ADC::setBuffers(CircularBuffer<uint16_t>* buffer, uint8_t pinNum) {
	if (pinNum >= _numPins) {
		return false;
	}
#ifdef PRINT_ADC_VERBOSE
	LOGd("Set buffer of pin %u at %u", pinNum, buffer);
#endif
	_circularBuffers[pinNum] = buffer;
	return true;
}

bool ADC::setTimestampBuffers(DifferentialBuffer<uint32_t>* buffer, uint8_t pinNum) {
	if (pinNum >= _numPins) {
		return false;
	}
#ifdef PRINT_ADC_VERBOSE
	LOGd("Set buffer of pin %u at %u", pinNum, buffer);
#endif
	_timeBuffers[pinNum] = buffer;
	return true;
}

void ADC::setDoneCallback(adc_done_cb_t callback) {
	_doneCallback = callback;
}

void ADC::stop() {
	nrf_drv_timer_disable(_timer);
}

void ADC::start() {
	nrf_drv_timer_enable(_timer);
}

void adc_done(void * p_event_data, uint16_t event_size) {
	(*(adc_done_cb_t*)p_event_data)();
}

void ADC::update(uint32_t value) {
	//! nrf51 code goes here
}

void ADC::update(nrf_saadc_value_t* buf) {
	//! TODO: this is just to merge the new adc with the old code, should be optimized!

	uint32_t startTime = RTC::getCount(); //! Not really the start time
	uint32_t dT = 500 * RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000 / 1000;
//	LOGd("startTime=%d dT=%d", startTime, dT);
	for (int i=0; i<CS_ADC_BUF_SIZE; i++) {
		uint8_t pinNum = i%_numPins;
		if (_circularBuffers[pinNum] != NULL) {
			_circularBuffers[pinNum]->push(buf[i]);
		}
		else if (_stackBuffers[pinNum] != NULL) {
			if (_stackBuffers[pinNum]->full()) {
				if (_doneCallback) {
//					LOGd("doneCB");
					//! Decouple done callback from adc interrupt handler, and put it on app scheduler instead
					app_sched_event_put(&_doneCallback, sizeof(_doneCallback), adc_done);
				}
				stop();
				return;
			}
			_stackBuffers[pinNum]->push(buf[i]);
//			LOGd("%d %d", _stackBuffers[0]->size(), _stackBuffers[1]->size());
			if (_timeBuffers[pinNum] != NULL) {
				if (!_timeBuffers[pinNum]->push(startTime + (i/_numPins) * dT)) {
					_stackBuffers[pinNum]->clear();
				}
			}
		}
	}
}

/** The interrupt handler for an ADC data ready event.
 */
//extern "C" void ADC_IRQHandler(void) {
//#if (NORDIC_SDK_VERSION < 11)
//	uint32_t adc_value;
//
//	//! Clear data-ready event
//	NRF_ADC->EVENTS_END = 0;
//
//	//! Get value
//	adc_value = NRF_ADC->RESULT;
//	ADC &adc = ADC::getInstance();
//	adc.update(adc_value);
//
////	//! Use the STOP task to save current. Workaround for PAN_028 rev1.5 anomaly 1.
////	NRF_ADC->TASKS_STOP = 1;
//
////	//! next sample
////	NRF_ADC->TASKS_START = 1;
//#endif
//}

//extern "C" void TIMER1_IRQHandler(void) {
//#if (NORDIC_SDK_VERSION < 11)
//	CS_ADC_TIMER->EVENTS_COMPARE[0] = 0; // Clear compare match register
//	CS_ADC_TIMER->TASKS_CLEAR = 1; // Reset timer
//	NRF_ADC->TASKS_START = 1;
//	ADC::getInstance()._lastStartTime = RTC::getCount();
//#endif
//}

extern "C" void saadc_callback(nrf_drv_saadc_evt_t const * p_event) {
	if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {
		ret_code_t err_code;

		//! Put this buffer in queue again
		err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, CS_ADC_BUF_SIZE);
		APP_ERROR_CHECK(err_code);

//		LOGd("samples: ");
//		for (int i=0; i<CS_ADC_BUF_SIZE; i++) {
//			_log(SERIAL_DEBUG, "%i ", p_event->data.done.p_buffer[i]);
//			if ((i+1) % 10 == 0) {
//				_log(SERIAL_DEBUG, SERIAL_CRLF);
//			}
//		}
//		_log(SERIAL_DEBUG, SERIAL_CRLF);

		ADC::getInstance().update(p_event->data.done.p_buffer);
	}
}


