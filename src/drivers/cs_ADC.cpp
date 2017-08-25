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
//#include "processing/cs_Switch.h"

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
	for (int i=0; i<CS_ADC_NUM_BUFFERS; i++) {
		_bufferPointers[i] = NULL;
	}
	_doneCallbackData.callback = NULL;
	_doneCallbackData.buffer = NULL;
	_doneCallbackData.bufSize = 0;
	// TODO: misuse: overload of bufNum field to indicate also initialization
	_doneCallbackData.bufNum = CS_ADC_NUM_BUFFERS;
	_zeroCrossingCallback = NULL;
	_numBuffersQueued = 0;
	_lastZeroCrossUpTime = 0;
}

/**
 * The initialization function for ADC has to configure the ADC channels, but
 * also a timer that dictates when to sample.
 *
 *   - set the resolution to 12 bits
 *
 * @caller src/processing/cs_PowerSampling.cpp
 */
cs_adc_error_t ADC::init(const adc_config_t & config) {
	_config = config;
//	memcpy(&_config, &config, sizeof(adc_config_t));

	// Setup timer
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_bit_width_set(CS_ADC_TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_frequency_set(CS_ADC_TIMER, CS_ADC_TIMER_FREQ);
	uint32_t ticks = nrf_timer_us_to_ticks(_config.samplingPeriodUs, CS_ADC_TIMER_FREQ);
	LOGd("maxTicks=%u", ticks);
	nrf_timer_cc_write(CS_ADC_TIMER, NRF_TIMER_CC_CHANNEL0, ticks);
	nrf_timer_mode_set(CS_ADC_TIMER, NRF_TIMER_MODE_TIMER);
	nrf_timer_shorts_enable(CS_ADC_TIMER, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(0));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(1));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(2));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(3));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(4));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(5));

	// Setup PPI
	_ppiChannel = getPpiChannel(CS_ADC_PPI_CHANNEL_START);

	nrf_ppi_channel_endpoint_setup(
			_ppiChannel,
			(uint32_t)nrf_timer_event_address_get(CS_ADC_TIMER, nrf_timer_compare_event_get(0)),
			nrf_drv_saadc_sample_task_get()
	);

	nrf_ppi_channel_enable(_ppiChannel);


	// Config adc
	nrf_drv_saadc_config_t adcConfig = {
			.resolution         = NRF_SAADC_RESOLUTION_12BIT, // 14 bit can only be achieved with oversampling
			.oversample         = NRF_SAADC_OVERSAMPLE_DISABLED, // Oversampling can only be used when sampling 1 channel
			.interrupt_priority = SAADC_CONFIG_IRQ_PRIORITY
	};

	ret_code_t err_code;
	err_code = nrf_drv_saadc_init(&adcConfig, saadc_callback);
	APP_ERROR_CHECK(err_code);

	for (int i=0; i<_config.channelCount; ++i) {
		initChannel(i, _config.channels[i]);
	}

	// Allocate buffers
	for (int i=0; i<CS_ADC_NUM_BUFFERS; i++) {
		_bufferPointers[i] = new nrf_saadc_value_t[CS_ADC_BUF_SIZE];
		/* Start conversion in non-blocking mode. Sampling is not triggered yet. */
		addBufferToSampleQueue(_bufferPointers[i]);
	}

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

/** Configure an ADC channel.
 *
 *   - set acquire time to 10 micro seconds
 *   - use the internal VGB reference of 0.6V (not the external one, so no need to use its multiplexer either)
 *   - set the prescaler for the input voltage (the input, not the input supply)
 *   - set either differential mode (pin - ref_pin), or single ended mode (pin - 0)
 */
cs_adc_error_t ADC::initChannel(cs_adc_channel_id_t channel, adc_channel_config_t& config) {

	assert(config.pin < 8, "Invalid pin");
	assert(config.referencePin < 8 || config.referencePin == CS_ADC_REF_PIN_NOT_AVAILABLE, "Invalid ref pin");
	assert(config.pin != config.referencePin, "Pin and ref pin should be different");

	nrf_saadc_channel_config_t channelConfig;
	channelConfig.resistor_p = NRF_SAADC_RESISTOR_DISABLED;
	channelConfig.resistor_n = NRF_SAADC_RESISTOR_DISABLED;
//	channelConfig.gain = config.gain;
	if (config.rangeMilliVolt <= 150) {
		channelConfig.gain = NRF_SAADC_GAIN4;
	}
	else if (config.rangeMilliVolt <= 300) {
		channelConfig.gain = NRF_SAADC_GAIN2;
	}
	else if (config.rangeMilliVolt <= 600) {
		channelConfig.gain = NRF_SAADC_GAIN1;
	}
	else if (config.rangeMilliVolt <= 1200) {
		channelConfig.gain = NRF_SAADC_GAIN1_2;
	}
	else if (config.rangeMilliVolt <= 1800) {
		channelConfig.gain = NRF_SAADC_GAIN1_3;
	}
	else if (config.rangeMilliVolt <= 2400) {
		channelConfig.gain = NRF_SAADC_GAIN1_4;
	}
	else if (config.rangeMilliVolt <= 3000) {
		channelConfig.gain = NRF_SAADC_GAIN1_5;
	}
//	else if (config.rangeMilliVolt <= 3600) {
	else {
		channelConfig.gain = NRF_SAADC_GAIN1_6;
	}


	channelConfig.reference = NRF_SAADC_REFERENCE_INTERNAL;
	channelConfig.acq_time = NRF_SAADC_ACQTIME_10US;
	if (config.referencePin == CS_ADC_REF_PIN_NOT_AVAILABLE) {
		channelConfig.mode = NRF_SAADC_MODE_SINGLE_ENDED;
		channelConfig.pin_n = NRF_SAADC_INPUT_DISABLED;
	}
	else {
		channelConfig.mode = NRF_SAADC_MODE_DIFFERENTIAL;
		channelConfig.pin_n = getAdcPin(config.pin);
	}
	channelConfig.pin_p = getAdcPin(config.pin);

	ret_code_t err_code = nrf_drv_saadc_channel_init(channel, &channelConfig);
	APP_ERROR_CHECK(err_code);
	return 0;
}

void ADC::setDoneCallback(adc_done_cb_t callback) {
	_doneCallbackData.callback = callback;
}

void ADC::stop() {
	// Stop or shutdown timer.
//	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_SHUTDOWN);
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_STOP);
}

void ADC::start() {
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_START);
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

void ADC::enableZeroCrossingInterrupt(cs_adc_channel_id_t channel, int32_t zeroVal) {
//	nrf_gpio_cfg_output(TEST_PIN);
	_zeroValue = zeroVal;
	_zeroCrossingChannel = channel;

	setLimitUp();

	// Enable adc limit interrupts
	nrf_saadc_int_enable(getAdcLimitLowInterruptMask(channel));
	nrf_saadc_int_enable(getAdcLimitHighInterruptMask(channel));
}

void ADC::setLimitUp() {
	nrf_saadc_channel_limits_set(_zeroCrossingChannel, -4096, _zeroValue);
}

void ADC::setLimitDown() {
	nrf_saadc_channel_limits_set(_zeroCrossingChannel, _zeroValue, 4096);
}

void adc_done(void * p_event_data, uint16_t event_size) {
	adc_done_cb_data_t* cbData = (adc_done_cb_data_t*)p_event_data;
	cbData->callback(cbData->buffer, cbData->bufSize, cbData->bufNum);
}

void ADC::_handleAdcDoneInterrupt(nrf_saadc_value_t* buf) {
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

void ADC::_handleAdcLimitInterrupt(nrf_saadc_limit_t type) {
	if (type == NRF_SAADC_LIMIT_LOW) {
		// NRF_SAADC_LIMIT_LOW  triggers when adc value is below lower limit
		setLimitUp();

//		nrf_gpiote_task_configure(2, TEST_PIN, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
//		nrf_gpiote_task_enable(2);
//		nrf_timer_task_trigger(NRF_TIMER3, NRF_TIMER_TASK_START);
	}
	else {
		// NRF_SAADC_LIMIT_HIGH triggers when adc value is above upper limit
		setLimitDown();

//		nrf_gpiote_task_configure(2, TEST_PIN, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
//		nrf_gpiote_task_enable(2);
//		nrf_timer_task_trigger(NRF_TIMER3, NRF_TIMER_TASK_START);
		nrf_gpio_pin_toggle(TEST_PIN);


		// Only call zero crossing callback when there was about 20ms between the two events.
		// This makes it more likely that this was an actual zero crossing.
		uint32_t curTime = RTC::getCount();
		uint32_t diffTicks = RTC::difference(curTime, _lastZeroCrossUpTime);
		if ((_zeroCrossingCallback != NULL) && (diffTicks > RTC::msToTicks(19)) && (diffTicks < RTC::msToTicks(21))) {
//			Switch::getInstance().onZeroCrossing();
			_zeroCrossingCallback();
		}
		_lastZeroCrossUpTime = curTime;

	}
}

extern "C" void saadc_callback(nrf_drv_saadc_evt_t const * p_event) {
	switch(p_event->type) {
	case NRF_DRV_SAADC_EVT_DONE:
		ADC::getInstance()._handleAdcDoneInterrupt(p_event->data.done.p_buffer);
		break;
	case NRF_DRV_SAADC_EVT_LIMIT:
		ADC::getInstance()._handleAdcLimitInterrupt(p_event->data.limit.limit_type);
		break;
	}
}

nrf_ppi_channel_t ADC::getPpiChannel(uint8_t index) {
	assert(index < 16, "invalid ppi channel index");
	switch(index) {
	case 0:
		return NRF_PPI_CHANNEL0;
	case 1:
		return NRF_PPI_CHANNEL1;
	case 2:
		return NRF_PPI_CHANNEL2;
	case 3:
		return NRF_PPI_CHANNEL3;
	case 4:
		return NRF_PPI_CHANNEL4;
	case 5:
		return NRF_PPI_CHANNEL5;
	case 6:
		return NRF_PPI_CHANNEL6;
	case 7:
		return NRF_PPI_CHANNEL7;
	case 8:
		return NRF_PPI_CHANNEL8;
	case 9:
		return NRF_PPI_CHANNEL9;
	case 10:
		return NRF_PPI_CHANNEL10;
	case 11:
		return NRF_PPI_CHANNEL11;
	case 12:
		return NRF_PPI_CHANNEL12;
	case 13:
		return NRF_PPI_CHANNEL13;
	case 14:
		return NRF_PPI_CHANNEL14;
	case 15:
		return NRF_PPI_CHANNEL15;
	}
	return NRF_PPI_CHANNEL0;
}

/**
 * The NC field disables the ADC and is actually set to value 0.
 * SAADC_CH_PSELP_PSELP_AnalogInput0 has value 1.
 */
nrf_saadc_input_t ADC::getAdcPin(const cs_adc_pin_id_t pinNum) {
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

nrf_saadc_int_mask_t ADC::getAdcLimitLowInterruptMask(uint8_t index) {
	switch(index) {
	case 0:
		return NRF_SAADC_INT_CH0LIMITL;
	case 1:
		return NRF_SAADC_INT_CH1LIMITL;
	case 2:
		return NRF_SAADC_INT_CH2LIMITL;
	case 3:
		return NRF_SAADC_INT_CH3LIMITL;
	case 4:
		return NRF_SAADC_INT_CH4LIMITL;
	case 5:
		return NRF_SAADC_INT_CH5LIMITL;
	case 6:
		return NRF_SAADC_INT_CH6LIMITL;
	case 7:
		return NRF_SAADC_INT_CH7LIMITL;
	default:
		return NRF_SAADC_INT_CH0LIMITL;
	}
}

nrf_saadc_int_mask_t ADC::getAdcLimitHighInterruptMask(uint8_t index) {
	switch(index) {
	case 0:
		return NRF_SAADC_INT_CH0LIMITH;
	case 1:
		return NRF_SAADC_INT_CH1LIMITH;
	case 2:
		return NRF_SAADC_INT_CH2LIMITH;
	case 3:
		return NRF_SAADC_INT_CH3LIMITH;
	case 4:
		return NRF_SAADC_INT_CH4LIMITH;
	case 5:
		return NRF_SAADC_INT_CH5LIMITH;
	case 6:
		return NRF_SAADC_INT_CH6LIMITH;
	case 7:
		return NRF_SAADC_INT_CH7LIMITH;
	default:
		return NRF_SAADC_INT_CH0LIMITH;
	}
}
