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
#include "protocol/cs_ErrorCodes.h"
#include "protocol/cs_UartProtocol.h"

//#define PRINT_ADC_VERBOSE

// Define test pin to enable gpio debug.
//#define TEST_PIN  24
//#define TEST_PIN2 25
//#define TEST_PIN3 26


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
	LOGi("init: period=%uus", _config.samplingPeriodUs);

#ifdef TEST_PIN
	nrf_gpio_cfg_output(TEST_PIN);
#endif
#ifdef TEST_PIN2
	nrf_gpio_cfg_output(TEST_PIN2);
#endif

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
		// Start conversion in non-blocking mode. Sampling is not triggered yet.
		addBufferToSampleQueue(_bufferPointers[i]);
	}

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
	// TODO: No logs, this function can be called from interrupt
	LOGi("init channel %u on ain%u, range=%umV, ref=ain%u", channel, config.pin, config.rangeMilliVolt, config.referencePin);
	assert(config.pin < 8 || config.pin == CS_ADC_PIN_VDD, "Invalid pin");
	assert(config.referencePin < 8 || config.referencePin == CS_ADC_REF_PIN_NOT_AVAILABLE, "Invalid ref pin");
//	assert(config.pin != config.referencePin, "Pin and ref pin should be different");

	nrf_saadc_channel_config_t channelConfig;
	channelConfig.resistor_p = NRF_SAADC_RESISTOR_DISABLED;
//  // Measure ground
//	if (config.referencePin == 7) {
//		channelConfig.resistor_p = NRF_SAADC_RESISTOR_PULLDOWN;
//	}
	channelConfig.resistor_n = NRF_SAADC_RESISTOR_DISABLED;
	if (config.rangeMilliVolt <= 150) {
		LOGd("gain=4 range=150mV");
		channelConfig.gain = NRF_SAADC_GAIN4;
	}
	else if (config.rangeMilliVolt <= 300) {
		LOGd("gain=2 range=300mV");
		channelConfig.gain = NRF_SAADC_GAIN2;
	}
	else if (config.rangeMilliVolt <= 600) {
		LOGd("gain=1 range=600mV");
		channelConfig.gain = NRF_SAADC_GAIN1;
	}
	else if (config.rangeMilliVolt <= 1200) {
		LOGd("gain=1/2 range=1200mV");
		channelConfig.gain = NRF_SAADC_GAIN1_2;
	}
	else if (config.rangeMilliVolt <= 1800) {
		LOGd("gain=1/3 range=1800mV");
		channelConfig.gain = NRF_SAADC_GAIN1_3;
	}
	else if (config.rangeMilliVolt <= 2400) {
		LOGd("gain=1/4 range=2400mV");
		channelConfig.gain = NRF_SAADC_GAIN1_4;
	}
	else if (config.rangeMilliVolt <= 3000) {
		LOGd("gain=1/5 range=3000mV");
		channelConfig.gain = NRF_SAADC_GAIN1_5;
	}
//	else if (config.rangeMilliVolt <= 3600) {
	else {
		LOGd("gain=1/6 range=3600mV");
		channelConfig.gain = NRF_SAADC_GAIN1_6;
	}


	channelConfig.reference = NRF_SAADC_REFERENCE_INTERNAL;
	channelConfig.acq_time = NRF_SAADC_ACQTIME_10US;
	if (config.referencePin == CS_ADC_REF_PIN_NOT_AVAILABLE) {
		LOGd("single ended");
		channelConfig.mode = NRF_SAADC_MODE_SINGLE_ENDED;
		channelConfig.pin_n = NRF_SAADC_INPUT_DISABLED;
	}
	else {
		LOGd("differential");
		channelConfig.mode = NRF_SAADC_MODE_DIFFERENTIAL;
		channelConfig.pin_n = getAdcPin(config.referencePin);
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
	// TODO: don't just
	// Stop or shutdown timer.
//	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_SHUTDOWN);
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_STOP);
}

void ADC::start() {
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_START);
}

void ADC::addBufferToSampleQueue(nrf_saadc_value_t* buf) {
	// No logs, this function can be called from interrupt
#ifdef TEST_PIN2
	nrf_gpio_pin_toggle(TEST_PIN2);
#endif
	ret_code_t err_code;
	err_code = nrf_drv_saadc_buffer_convert(buf, CS_ADC_BUF_SIZE);
	APP_ERROR_CHECK(err_code); // TODO: got error 17 here (busy).
	_numBuffersQueued++;
}

bool ADC::releaseBuffer(nrf_saadc_value_t* buf) {
//	write("r %p\r\n", buf);
	if (_doneCallbackData.buffer != buf) {
		LOGe("buffer mismatch! %i vs %i", _doneCallbackData.buffer, buf);
		return false;
	}
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	// Clear the callback data
	_doneCallbackData.buffer = NULL;
	_doneCallbackData.bufSize = 0;
	_doneCallbackData.bufNum = CS_ADC_NUM_BUFFERS;

	if (_changeConfig) {
		// Don't queue up the the buffer, we need the adc to be idle.
		if (_numBuffersQueued == 0) {
			applyConfig();
		}
		return true;
	}
	addBufferToSampleQueue(buf);
	return true;
}

void ADC::setZeroCrossingCallback(adc_zero_crossing_cb_t callback) {
	_zeroCrossingCallback = callback;
}

void ADC::enableZeroCrossingInterrupt(cs_adc_channel_id_t channel, int32_t zeroVal) {
	LOGd("enable zero chan=%u zero=%i", channel, zeroVal);
	_zeroValue = zeroVal;
	_zeroCrossingChannel = channel;
	setLimitUp();
}

cs_adc_error_t ADC::changeChannel(cs_adc_channel_id_t channel, adc_channel_config_t& config) {
	if (channel >= _config.channelCount) {
		return ERR_INVALID_CHANNEL;
	}
	// Copy the channel config
	_config.channels[channel].pin = config.pin;
	_config.channels[channel].rangeMilliVolt = config.rangeMilliVolt;
	_config.channels[channel].referencePin = config.referencePin;
	_changeConfig = true;

//	cs_adc_error_t err = initChannel(channel, config);
//	start();
	return ERR_SUCCESS;
}

void ADC::applyConfig() {
	// Apply channel configs
	// TODO: No logs, this function can be called from interrupt
	LOGd("apply config");
	for (int i=0; i<_config.channelCount; ++i) {
		initChannel(i, _config.channels[i]);
	}

	UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_ADC_CONFIG, (uint8_t*)(&_config), sizeof(_config));

	// Add all buffers again
	for (int i=0; i<CS_ADC_NUM_BUFFERS; i++) {
		addBufferToSampleQueue(_bufferPointers[i]);
	}

	// Mark as done
	_changeConfig = false;
}

void ADC::setLimitUp() {
	// No logs, this function can be called from interrupt
	nrf_drv_saadc_limits_set(_zeroCrossingChannel, NRF_DRV_SAADC_LIMITL_DISABLED, _zeroValue);
}

void ADC::setLimitDown() {
	// No logs, this function can be called from interrupt
	nrf_drv_saadc_limits_set(_zeroCrossingChannel, _zeroValue, NRF_DRV_SAADC_LIMITH_DISABLED);
}

void adc_done(void * p_event_data, uint16_t event_size) {
	adc_done_cb_data_t* cbData = (adc_done_cb_data_t*)p_event_data;
//	write("d %p\r\n", cbData->buffer);
	cbData->callback(cbData->buffer, cbData->bufSize, cbData->bufNum);
}

void ADC::_handleAdcDoneInterrupt(nrf_saadc_value_t* buf) {
	// No logs, this function is called from interrupt
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif
	_numBuffersQueued--;

	if (_doneCallbackData.callback != NULL && _doneCallbackData.buffer == NULL) {
		//! Fill callback data object, should become available again in releaseBuffer()
		_doneCallbackData.buffer = buf;
		_doneCallbackData.bufSize = CS_ADC_BUF_SIZE;
		_doneCallbackData.bufNum = CS_ADC_NUM_BUFFERS;

		// Decouple done callback from adc interrupt handler, and put it on app scheduler instead
		uint32_t errorCode = app_sched_event_put(&_doneCallbackData, sizeof(_doneCallbackData), adc_done);
		APP_ERROR_CHECK(errorCode);
	}
	else {
//		// Don't queue up the the buffer, we need the adc to be idle.
//		if (_changeConfig) {
//			if (_numBuffersQueued == 0) {
//				applyConfig();
//			}
//			return;
//		}
		// Skip the callback, just put buffer in queue again.
		addBufferToSampleQueue(buf);
	}
}

void ADC::_handleAdcLimitInterrupt(nrf_saadc_limit_t type) {
	// No logs, this function is called from interrupt
	if (type == NRF_SAADC_LIMIT_LOW) {
		// NRF_SAADC_LIMIT_LOW  triggers when adc value is below lower limit
		setLimitUp();
	}
	else {
		// NRF_SAADC_LIMIT_HIGH triggers when adc value is above upper limit
		setLimitDown();

//#ifdef TEST_PIN
//		nrf_gpio_pin_toggle(TEST_PIN);
//#endif

		// Only call zero crossing callback when there was about 20ms between the two events.
		// This makes it more likely that this was an actual zero crossing.
		uint32_t curTime = RTC::getCount();
		uint32_t diffTicks = RTC::difference(curTime, _lastZeroCrossUpTime);
		if ((_zeroCrossingCallback != NULL) && (diffTicks > RTC::msToTicks(19)) && (diffTicks < RTC::msToTicks(21))) {
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

// Timer interrupt handler
extern "C" void CS_ADC_TIMER_IRQ(void) {
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
		return NRF_SAADC_INPUT_AIN0;
	case 1:
		return NRF_SAADC_INPUT_AIN1;
	case 2:
		return NRF_SAADC_INPUT_AIN2;
	case 3:
		return NRF_SAADC_INPUT_AIN3;
	case 4:
		return NRF_SAADC_INPUT_AIN4;
	case 5:
		return NRF_SAADC_INPUT_AIN5;
	case 6:
		return NRF_SAADC_INPUT_AIN6;
	case 7:
		return NRF_SAADC_INPUT_AIN7;
	case CS_ADC_PIN_VDD:
		return NRF_SAADC_INPUT_VDD;
	default:
		return NRF_SAADC_INPUT_DISABLED;
	}
}
