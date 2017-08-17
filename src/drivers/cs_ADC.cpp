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
#include "processing/cs_Switch.h"

//#define PRINT_ADC_VERBOSE
//#define TEST_PIN 22
//#define TEST_PIN 8 // PWM pin
#define TEST_PIN 20 // RX pin
#define TEST_ZERO 2000
//#define TEST_ZERO 0
#define TEST_ZERO_HYST 0


extern "C" void saadc_callback(nrf_drv_saadc_evt_t const * p_event);

ADC::ADC()
{
	_timer = new nrf_drv_timer_t();
	_timer->p_reg = CS_ADC_TIMER; // Or use CONCAT_2(NRF_TIMER, CS_ADC_TIMER_ID)
	_timer->instance_id = CS_ADC_INSTANCE_INDEX; // Or use CONCAT_3(TIMER, CS_ADC_TIMER_ID, _INSTANCE_INDEX)
	_timer->cc_channel_count = NRF_TIMER_CC_CHANNEL_COUNT(CS_ADC_TIMER_ID);

	for (int i=0; i<CS_ADC_NUM_BUFFERS; i++) {
		_bufferPointers[i] = NULL;
	}
	_doneCallbackData.callback = NULL;
	_doneCallbackData.buffer = NULL;
	_doneCallbackData.bufSize = 0;
	// TODO: misuse: overload of bufNum field to indicate also initialization
	_doneCallbackData.bufNum = CS_ADC_NUM_BUFFERS;
	_numBuffersQueued = 0;
	_lastZeroCrossUpTime = 0;
	_lastPwmSyncTime = 0;
}

/**
 * The initialization function for ADC has to configure the ADC channels, but
 * also a timer that dictates when to sample.
 *
 * @caller src/processing/cs_PowerSampling.cpp
 */
cs_adc_error_t ADC::init(const pin_id_t pins[], const pin_count_t numPins) {
	ret_code_t err_code;

	for (uint8_t i=0; i<numPins; i++) {
		_pins[i] = pins[i];
	}
	_numPins = numPins;

	err_code = nrf_drv_ppi_init();
	if (err_code != MODULE_ALREADY_INITIALIZED) {
		APP_ERROR_CHECK(err_code);
	}

	// TODO: Where is this configuration set? We also have cs_Config.h and third/nrf/nrf_drv_config.h
	nrf_drv_timer_config_t timerConfig = {
		.frequency          = NRF_TIMER_FREQ_16MHz,
		.mode               = (nrf_timer_mode_t)TIMER_MODE_MODE_Timer,
		.bit_width          = (nrf_timer_bit_width_t)TIMER_BITMODE_BITMODE_32Bit,
		.interrupt_priority = SAADC_TIMER_CONFIG_IRQ_PRIORITY,
		.p_context          = NULL
	};

	err_code = nrf_drv_timer_init(_timer, &timerConfig, staticTimerHandler);
	APP_ERROR_CHECK(err_code);

	//! Setup timer for compare event every CS_ADC_SAMPLE_INTERVAL_US us
	uint32_t ticks = nrf_drv_timer_us_to_ticks(_timer, CS_ADC_SAMPLE_INTERVAL_US);
//	LOGd("adc timer ticks: %u", ticks);
	nrf_drv_timer_extended_compare(_timer, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

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
		.interrupt_priority = SAADC_CONFIG_IRQ_PRIORITY
	};

	err_code = nrf_drv_saadc_init(&adcConfig, saadc_callback);
	APP_ERROR_CHECK(err_code);

	for (int i=0; i<_numPins; i++) {
		configPin(i, _pins[i]);
	}

	for (int i=0; i<CS_ADC_NUM_BUFFERS; i++) {
		_bufferPointers[i] = new nrf_saadc_value_t[CS_ADC_BUF_SIZE];
		/* Start conversion in non-blocking mode. Sampling is not triggered yet. */
		addBufferToSampleQueue(_bufferPointers[i]);
	}

	nrf_gpio_cfg_output(TEST_PIN);
	nrf_drv_saadc_limits_set(1, NRF_DRV_SAADC_LIMITL_DISABLED + 1, TEST_ZERO + TEST_ZERO_HYST); // Triggers when above zero



//	// Setup timer
//	nrf_timer_task_trigger(NRF_TIMER3, NRF_TIMER_TASK_CLEAR);
//	nrf_timer_bit_width_set(NRF_TIMER3, NRF_TIMER_BIT_WIDTH_32);
//	nrf_timer_frequency_set(NRF_TIMER3, NRF_TIMER_FREQ_4MHz);
//	nrf_timer_mode_set(NRF_TIMER3, NRF_TIMER_MODE_TIMER);
//
//	nrf_ppi_channel_endpoint_setup(
//			NRF_PPI_CHANNEL10,
//			(uint32_t)nrf_timer_event_address_get(NRF_TIMER3, NRF_TIMER_EVENT_COMPARE0),
//			nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_2)
//	);
//	nrf_ppi_channel_enable(NRF_PPI_CHANNEL10);
//
//	nrf_ppi_channel_endpoint_setup(
//			NRF_PPI_CHANNEL11,
//			(uint32_t)nrf_timer_event_address_get(NRF_TIMER3, NRF_TIMER_EVENT_COMPARE1),
//			nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_2)
//	);
//	nrf_ppi_channel_enable(NRF_PPI_CHANNEL11);
//
//
//	nrf_timer_shorts_enable(NRF_TIMER3, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK);
//	nrf_timer_shorts_enable(NRF_TIMER3, NRF_TIMER_SHORT_COMPARE1_STOP_MASK);
//	// 10 ms is 40000 ticks at 4MHz
//	uint16_t value = 15;
//	uint32_t valueTicks = 40000 * value / 100;
//	nrf_timer_cc_write(NRF_TIMER3, NRF_TIMER_CC_CHANNEL0, 20000 - valueTicks/2);
//	nrf_timer_cc_write(NRF_TIMER3, NRF_TIMER_CC_CHANNEL1, 20000 + valueTicks/2);



	return 0;
}

//#define CS_ADC_PIN_P(pinNum) CONCAT_2(NRF_SAADC_INPUT_AIN, pinNum)

/** Configure the AD converter.
 *
 *   - set the resolution to 10 bits
 *   - set the prescaler for the input voltage (the input, not the input supply)
 *   - use the internal VGB reference (not the external one, so no need to use its multiplexer either)
 *   - do not set the prescaler for the reference voltage, this means voltage is expected between 0 and 1.2V (VGB)
 * The prescaler for input is set to 1/3. This means that the AIN input can be from 0 to 3.6V.
 */
cs_adc_error_t ADC::configPin(const channel_id_t channelNum, const pin_id_t pinNum) {
	LOGd("Configuring channel %i, pin %i", channelNum, pinNum);
	ret_code_t err_code;

	nrf_saadc_channel_config_t channelConfig;
	if (channelNum == 1)  {
		// voltage sits on channel 1
		channelConfig = {
			.resistor_p = NRF_SAADC_RESISTOR_DISABLED,
			.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
//			.gain       = NRF_SAADC_GAIN2,   //! gain is 2/1, maps [0, 0.3] to [0, 0.6]
//			.gain       = NRF_SAADC_GAIN1,   //! gain is 1/1, maps [0, 0.6] to [0, 0.6]
			.gain       = NRF_SAADC_GAIN1_2, //! gain is 1/2, maps [0, 1.2] to [0, 0.6]
//			.gain       = NRF_SAADC_GAIN1_4, //! gain is 1/4, maps [0, 2.4] to [0, 0.6]
//			.gain       = NRF_SAADC_GAIN1_6, //! gain is 1/6, maps [0, 3.6] to [0, 0.6]
			.reference  = NRF_SAADC_REFERENCE_INTERNAL, //! 0.6V
			.acq_time   = NRF_SAADC_ACQTIME_10US, //! 10 micro seconds (10e-6 seconds)
			.mode       = NRF_SAADC_MODE_SINGLE_ENDED,
//			.mode		= NRF_SAADC_MODE_DIFFERENTIAL,
			.pin_p      = getAdcPin(pinNum),
			.pin_n      = NRF_SAADC_INPUT_DISABLED
//			.pin_n      = getAdcPin(4) // gpio 28
		};
	} else if (channelNum == 0) {
		// current sits on channel 0
		channelConfig = {
			.resistor_p = NRF_SAADC_RESISTOR_DISABLED,
			.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
//			.gain       = NRF_SAADC_GAIN2,   //! gain is 2/1, maps [0, 0.3] to [0, 0.6]
			.gain       = NRF_SAADC_GAIN1_2, //! gain is 1/2, maps [0, 1.2] to [0, 0.6]
//			.gain       = NRF_SAADC_GAIN1_6, //! gain is 1/6, maps [0, 3.6] to [0, 0.6]
			.reference  = NRF_SAADC_REFERENCE_INTERNAL, //! 0.6V
			.acq_time   = NRF_SAADC_ACQTIME_10US, //! 10 micro seconds (10e-6 seconds)
			.mode       = NRF_SAADC_MODE_SINGLE_ENDED,
//			.mode		= NRF_SAADC_MODE_DIFFERENTIAL,
			.pin_p      = getAdcPin(pinNum),
			.pin_n      = NRF_SAADC_INPUT_DISABLED
//			.pin_n      = getAdcPin(4) // gpio 28
		};
	} else {
		// no idea!!
		assert(false, "Unknown channel");
	}

	err_code = nrf_drv_saadc_channel_init(channelNum, &channelConfig);
	APP_ERROR_CHECK(err_code);

	return 0;
}

/**
 * The NC field disables the ADC and is actually set to value 0. 
 * SAADC_CH_PSELP_PSELP_AnalogInput0 has value 1.
 */
nrf_saadc_input_t ADC::getAdcPin(const pin_id_t pinNum) {
	switch (pinNum) {
	case 0:
		return (nrf_saadc_input_t)SAADC_CH_PSELP_PSELP_AnalogInput0;
	case 1:
		return (nrf_saadc_input_t)SAADC_CH_PSELP_PSELP_AnalogInput1;
	case 2:
		return (nrf_saadc_input_t)SAADC_CH_PSELP_PSELP_AnalogInput2;
	case 3:
		return (nrf_saadc_input_t)SAADC_CH_PSELP_PSELP_AnalogInput3;
	case 4:
		return (nrf_saadc_input_t)SAADC_CH_PSELP_PSELP_AnalogInput4;
	case 5:
		return (nrf_saadc_input_t)SAADC_CH_PSELP_PSELP_AnalogInput5;
	case 6:
		return (nrf_saadc_input_t)SAADC_CH_PSELP_PSELP_AnalogInput6;
	case 7:
		return (nrf_saadc_input_t)SAADC_CH_PSELP_PSELP_AnalogInput7;
	default:
		return (nrf_saadc_input_t)SAADC_CH_PSELP_PSELP_NC;
	}
}

void ADC::setDoneCallback(adc_done_cb_t callback) {
	_doneCallbackData.callback = callback;
}

void ADC::stop() {
	nrf_drv_timer_disable(_timer);
}

void ADC::start() {
	nrf_drv_timer_enable(_timer);
}

void ADC::addBufferToSampleQueue(nrf_saadc_value_t* buf) {
	ret_code_t err_code;
	err_code = nrf_drv_saadc_buffer_convert(buf, CS_ADC_BUF_SIZE);
	APP_ERROR_CHECK(err_code);
	_numBuffersQueued++;
}

bool ADC::releaseBuffer(nrf_saadc_value_t* buf) {
	if (_doneCallbackData.buffer != buf) {
		LOGe("buffer mismatch! %i vs %i", _doneCallbackData.buffer, buf);
		return false;
	}

	//! Clear the callback data
	_doneCallbackData.buffer = NULL;
	_doneCallbackData.bufSize = 0;
	_doneCallbackData.bufNum = CS_ADC_NUM_BUFFERS;

	addBufferToSampleQueue(buf);
	return true;
}

void adc_done(void * p_event_data, uint16_t event_size) {
	adc_done_cb_data_t* cbData = (adc_done_cb_data_t*)p_event_data;
	cbData->callback(cbData->buffer, cbData->bufSize, cbData->bufNum);
}

void ADC::update(nrf_saadc_value_t* buf) {
	_numBuffersQueued--;
	if (_doneCallbackData.callback != NULL && _doneCallbackData.buffer == NULL) {
		//! Fill callback data object, should become available again in releaseBuffer()
		_doneCallbackData.buffer = buf;
		_doneCallbackData.bufSize = CS_ADC_BUF_SIZE;
		_doneCallbackData.bufNum = CS_ADC_NUM_BUFFERS;

		// Decouple done callback from adc interrupt handler, and put it on app scheduler instead
		uint32_t errorCode = app_sched_event_put(&_doneCallbackData, sizeof(_doneCallbackData), adc_done);
		APP_ERROR_CHECK(errorCode);
	} else {
		//! Skip the callback, just put buffer in queue again.
//		write("/!\\");
		addBufferToSampleQueue(buf);
	}
}

void ADC::limit(nrf_saadc_limit_t type) {
	if (type == NRF_SAADC_LIMIT_LOW) {
		// NRF_SAADC_LIMIT_LOW  triggers when adc value is below lower limit
		nrf_drv_saadc_limits_set(1, NRF_DRV_SAADC_LIMITL_DISABLED + 1, TEST_ZERO + TEST_ZERO_HYST); // Triggers when above zero

//		nrf_gpiote_task_configure(2, TEST_PIN, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
//		nrf_gpiote_task_enable(2);
//		nrf_timer_task_trigger(NRF_TIMER3, NRF_TIMER_TASK_START);
	}
	else {
		// NRF_SAADC_LIMIT_HIGH triggers when adc value is above upper limit
		nrf_drv_saadc_limits_set(1, TEST_ZERO - TEST_ZERO_HYST, NRF_DRV_SAADC_LIMITH_DISABLED - 1); // Triggers when below zero

//		nrf_gpiote_task_configure(2, TEST_PIN, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
//		nrf_gpiote_task_enable(2);
//		nrf_timer_task_trigger(NRF_TIMER3, NRF_TIMER_TASK_START);
		nrf_gpio_pin_toggle(TEST_PIN);


		// Only call syncPwm when there was about 20ms between the two events.
		// This makes it more likely that this was actually the correct zero crossing.
		uint32_t curTime = RTC::getCount();
		uint32_t diffTicks = RTC::difference(curTime, _lastZeroCrossUpTime);
		if (diffTicks > RTC::msToTicks(19) && diffTicks < RTC::msToTicks(21)) {
//			if (RTC::difference(curTime, _lastPwmSyncTime) > RTC::msToTicks(100)) {
				Switch::getInstance().onZeroCrossing();
//				_lastPwmSyncTime = curTime;
//			}
		}
		_lastZeroCrossUpTime = curTime;

	}
}

void ADC::staticTimerHandler(nrf_timer_event_t event_type, void* ptr) {
}

extern "C" void saadc_callback(nrf_drv_saadc_evt_t const * p_event) {
	if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {
		ADC::getInstance().update(p_event->data.done.p_buffer);
	}
	if (p_event->type == NRF_DRV_SAADC_EVT_LIMIT) {
		ADC::getInstance().limit(p_event->data.limit.limit_type);
	}
}


