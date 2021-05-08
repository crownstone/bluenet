/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cs_Crownstone.h>
#include <ble/cs_Nordic.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_Strings.h>
#include <drivers/cs_ADC.h>
#include <drivers/cs_RTC.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_ErrorCodes.h>
#include <uart/cs_UartHandler.h>
#include <structs/buffer/cs_AdcBuffer.h>
#include <util/cs_BleError.h>

#define LOGAdcDebug LOGd
#define LOGAdcVerbose LOGnone
#define LOGAdcInterruptWarn LOGnone
#define LOGAdcInterruptInfo LOGnone
#define LOGAdcInterruptDebug LOGnone
#define LOGAdcInterruptVerbose LOGnone
#define ADC_LOG_QUEUES false

// Define test pin to enable gpio debug.
//#define TEST_PIN_ZERO_CROSS 20
//#define TEST_PIN_START      22
//#define TEST_PIN_STOP       23
//#define TEST_PIN_PROCESS    24
//#define TEST_PIN_INT_END    25
//#define TEST_PIN_SAMPLE     20 // Controlled via GPIOTE


#if defined(TEST_PIN_ZERO_CROSS) || defined(TEST_PIN_START) || defined(TEST_PIN_STOP) || defined(TEST_PIN_PROCESS) || defined(TEST_PIN_INT_END) || defined(TEST_PIN_SAMPLE)
	#ifdef DEBUG
		#pragma message("ADC test pin enabled")
	#else
		#warning "ADC test pin enabled"
	#endif
#endif

// Defines specific for implementation
#define LIMIT_LOW_DISABLED  (-2048) // min adc value
#define LIMIT_HIGH_DISABLED (4095)  // max adc value
#define BUFFER_INDEX_NONE 255


// Called by app scheduler, from saadc interrupt.
void adc_done(void * p_event_data, uint16_t event_size) {
	adc_buffer_id_t* bufIndex = (adc_buffer_id_t*)p_event_data;
	ADC::getInstance()._handleAdcDone(*bufIndex);
}

// Called by app scheduler, from saadc interrupt.
void adc_restart(void * p_event_data, uint16_t event_size) {
	ADC::getInstance()._restart();
}


ADC::ADC() :
		_bufferQueue(CS_ADC_NUM_BUFFERS),
		_saadcBufferQueue(CS_ADC_NUM_SAADC_BUFFERS)
{
	_ppiChannelSample = getPpiChannel(CS_ADC_PPI_CHANNEL_START);
	_ppiChannelStart =  getPpiChannel(CS_ADC_PPI_CHANNEL_START + 1);
}

/**
 * The initialization function for ADC has to configure the ADC channels, but
 * also a timer that dictates when to sample.
 *
 *   - set the resolution to 12 bits
 *
 * @caller src/processing/cs_PowerSampling.cpp
 */
cs_ret_code_t ADC::init(const adc_config_t & config) {
	ret_code_t err_code;
	_config = config;
//	memcpy(&_config, &config, sizeof(adc_config_t));
	LOGi("init: period=%uus", _config.samplingIntervalUs);

#ifdef TEST_PIN_ZERO_CROSS
	nrf_gpio_cfg_output(TEST_PIN_ZERO_CROSS);
#endif
#ifdef TEST_PIN_START
	nrf_gpio_cfg_output(TEST_PIN_START);
#endif
#ifdef TEST_PIN_STOP
	nrf_gpio_cfg_output(TEST_PIN_STOP);
#endif
#ifdef TEST_PIN_PROCESS
	nrf_gpio_cfg_output(TEST_PIN_PROCESS);
#endif
#ifdef TEST_PIN_INT_END
	nrf_gpio_cfg_output(TEST_PIN_INT_END);
#endif


	// -------------------
	// Setup sample timer
	// -------------------
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_bit_width_set(CS_ADC_TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_frequency_set(CS_ADC_TIMER, CS_ADC_TIMER_FREQ);
	uint32_t ticks = nrf_timer_us_to_ticks(_config.samplingIntervalUs, CS_ADC_TIMER_FREQ);
	LOGv("ticks=%u", ticks);
	nrf_timer_cc_write(CS_ADC_TIMER, NRF_TIMER_CC_CHANNEL0, ticks);
	nrf_timer_mode_set(CS_ADC_TIMER, NRF_TIMER_MODE_TIMER);
	nrf_timer_shorts_enable(CS_ADC_TIMER, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(0));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(1));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(2));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(3));

	// Setup PPI: on timer compare event, call adc sample task.
#ifdef TEST_PIN_SAMPLE
	// Also toggle sample test pin each time the sample task is called.
	nrf_gpiote_task_configure(CS_ADC_GPIOTE_CHANNEL_START, TEST_PIN_SAMPLE, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_task_enable(CS_ADC_GPIOTE_CHANNEL_START);
	nrf_ppi_channel_and_fork_endpoint_setup(
			_ppiChannelSample,
			(uint32_t)nrf_timer_event_address_get(CS_ADC_TIMER, nrf_timer_compare_event_get(0)),
			nrf_saadc_task_address_get(NRF_SAADC_TASK_SAMPLE),
			nrf_gpiote_task_addr_get(getGpioteTaskOut(CS_ADC_GPIOTE_CHANNEL_START))
	);
#else
	nrf_ppi_channel_endpoint_setup(
			_ppiChannelSample,
			(uint32_t)nrf_timer_event_address_get(CS_ADC_TIMER, nrf_timer_compare_event_get(0)),
			nrf_saadc_task_address_get(NRF_SAADC_TASK_SAMPLE)
	);
	nrf_ppi_channel_enable(_ppiChannelSample);
#endif


	// -------------------
	// Setup SAADC
	// -------------------

	// Setup PPI: call START on END.
	// This avoids a SAMPLE being called before the START.
	// See https://devzone.nordicsemi.com/f/nordic-q-a/20291/offset-in-saadc-samples-with-easy-dma-and-ble/79053
	nrf_ppi_channel_endpoint_setup(
			_ppiChannelStart,
			(uint32_t)nrf_saadc_event_address_get(NRF_SAADC_EVENT_END),
			nrf_saadc_task_address_get(NRF_SAADC_TASK_START)
	);

	// Config adc
	err_code = initSaadc(config);
	APP_ERROR_CHECK(err_code);

	for (int i=0; i<_config.channelCount; ++i) {
//		_channelResultConfigs[i].samplingPeriodUs = nrf_timer_ticks_to_us(ticks);
		_channelResultConfigs[i].samplingIntervalUs = config.samplingIntervalUs;
		initChannel(i, _config.channels[i]);
	}
	// NRF52_PAN_74
	nrf_saadc_enable();

	initBufferQueue();

	this->listen();

	return ERR_SUCCESS;
}


cs_ret_code_t ADC::initSaadc(const adc_config_t & config) {
	// NRF52_PAN_74
	nrf_saadc_disable();

	nrf_saadc_resolution_set(CS_ADC_RESOLUTION);
	nrf_saadc_oversample_set(NRF_SAADC_OVERSAMPLE_DISABLED); // Oversample can only be used with 1 channel.
	nrf_saadc_continuous_mode_disable();

	nrf_saadc_int_disable(NRF_SAADC_INT_ALL);
	nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

	// Enable interrupts
	sd_nvic_SetPriority(SAADC_IRQn, CS_ADC_IRQ_PRIORITY);
	sd_nvic_ClearPendingIRQ(SAADC_IRQn);
	sd_nvic_EnableIRQ(SAADC_IRQn);
	nrf_saadc_int_enable(NRF_SAADC_INT_END);

	// NRF52_PAN_74: enable after config
//	nrf_saadc_enable();

	return ERR_SUCCESS;
}

cs_ret_code_t ADC::initBufferQueue() {
	if (!_saadcBufferQueue.init()) {
		return ERR_NO_SPACE;
	}
	if (!_bufferQueue.init()) {
		return ERR_NO_SPACE;
	}
	cs_ret_code_t retCode = AdcBuffer::getInstance().init();
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	adc_buffer_id_t bufCount = AdcBuffer::getInstance().getBufferCount();
	for (adc_buffer_id_t id = 0; id < bufCount; ++id) {
		_bufferQueue.push(id);
	}
	return ERR_SUCCESS;
}

/** Configure an ADC channel.
 *
 *   - set acquire time to 10 micro seconds
 *   - use the internal VGB reference of 0.6V (not the external one, so no need to use its multiplexer either)
 *   - set the prescaler for the input voltage (the input, not the input supply)
 *   - set either differential mode (pin - ref_pin), or single ended mode (pin - 0)
 */
cs_ret_code_t ADC::initChannel(adc_channel_id_t channel, adc_channel_config_t& config) {
	LOGi("Init channel %u on AIN%u, range=%umV, ref=ain%u", channel, config.pin, config.rangeMilliVolt, config.referencePin);
	assert(config.pin < 8 || config.pin == CS_ADC_PIN_VDD, "Invalid pin");
	assert(config.referencePin < 8 || config.referencePin == CS_ADC_REF_PIN_NOT_AVAILABLE, "Invalid ref pin");
//	assert(config.pin != config.referencePin, "Pin and ref pin should be different");

	nrf_saadc_channel_config_t channelConfig;
	channelConfig.burst      = NRF_SAADC_BURST_DISABLED;
	channelConfig.resistor_p = NRF_SAADC_RESISTOR_DISABLED;
//  // Measure ground
//	if (config.referencePin == 7) {
//		channelConfig.resistor_p = NRF_SAADC_RESISTOR_PULLDOWN;
//	}
	channelConfig.resistor_n = NRF_SAADC_RESISTOR_DISABLED;
	if (config.rangeMilliVolt <= 150) {
		LOGv("gain=4 range=150mV");
		_channelResultConfigs[channel].maxValueMilliVolt = 150;
		channelConfig.gain = NRF_SAADC_GAIN4;
	}
	else if (config.rangeMilliVolt <= 300) {
		LOGv("gain=2 range=300mV");
		_channelResultConfigs[channel].maxValueMilliVolt = 300;
		channelConfig.gain = NRF_SAADC_GAIN2;
	}
	else if (config.rangeMilliVolt <= 600) {
		LOGv("gain=1 range=600mV");
		_channelResultConfigs[channel].maxValueMilliVolt = 600;
		channelConfig.gain = NRF_SAADC_GAIN1;
	}
	else if (config.rangeMilliVolt <= 1200) {
		LOGv("gain=1/2 range=1200mV");
		_channelResultConfigs[channel].maxValueMilliVolt = 1200;
		channelConfig.gain = NRF_SAADC_GAIN1_2;
	}
	else if (config.rangeMilliVolt <= 1800) {
		LOGv("gain=1/3 range=1800mV");
		_channelResultConfigs[channel].maxValueMilliVolt = 1800;
		channelConfig.gain = NRF_SAADC_GAIN1_3;
	}
	else if (config.rangeMilliVolt <= 2400) {
		LOGv("gain=1/4 range=2400mV");
		_channelResultConfigs[channel].maxValueMilliVolt = 2400;
		channelConfig.gain = NRF_SAADC_GAIN1_4;
	}
	else if (config.rangeMilliVolt <= 3000) {
		LOGv("gain=1/5 range=3000mV");
		_channelResultConfigs[channel].maxValueMilliVolt = 3000;
		channelConfig.gain = NRF_SAADC_GAIN1_5;
	}
	else {
		LOGv("gain=1/6 range=3600mV");
		_channelResultConfigs[channel].maxValueMilliVolt = 3600;
		channelConfig.gain = NRF_SAADC_GAIN1_6;
	}

	int adcBits = 8;
	switch (CS_ADC_RESOLUTION) {
		case NRF_SAADC_RESOLUTION_8BIT:
			adcBits = 8;
			break;
		case NRF_SAADC_RESOLUTION_10BIT:
			adcBits = 10;
			break;
		case NRF_SAADC_RESOLUTION_12BIT:
			adcBits = 12;
			break;
		case NRF_SAADC_RESOLUTION_14BIT:
			adcBits = 14;
			break;
	}

	channelConfig.reference = NRF_SAADC_REFERENCE_INTERNAL;
	channelConfig.acq_time = NRF_SAADC_ACQTIME_10US;
	if (config.referencePin == CS_ADC_REF_PIN_NOT_AVAILABLE) {
		LOGv("single ended");
		channelConfig.mode = NRF_SAADC_MODE_SINGLE_ENDED;
		channelConfig.pin_n = NRF_SAADC_INPUT_DISABLED;
		_channelResultConfigs[channel].maxSampleValue = (1 << adcBits) - 1;
		_channelResultConfigs[channel].minSampleValue = 0;
		_channelResultConfigs[channel].minValueMilliVolt = 0;
	}
	else {
		LOGv("differential");
		channelConfig.mode = NRF_SAADC_MODE_DIFFERENTIAL;
		channelConfig.pin_n = getAdcPin(config.referencePin);
		_channelResultConfigs[channel].maxSampleValue = (1 << (adcBits - 1)) - 1;
		_channelResultConfigs[channel].minSampleValue =    -1 * _channelResultConfigs[channel].maxSampleValue;
		_channelResultConfigs[channel].minValueMilliVolt = -1 * _channelResultConfigs[channel].maxValueMilliVolt;
	}
	channelConfig.pin_p = getAdcPin(config.pin);




//	ret_code_t err_code = nrf_drv_saadc_channel_init(channel, &channelConfig);
//	APP_ERROR_CHECK(err_code);

//	ASSERT(m_cb.state != NRF_DRV_STATE_UNINITIALIZED);
//	ASSERT(channel < NRF_SAADC_CHANNEL_COUNT);
//	//Oversampling can be used only with one channel.
//	ASSERT((nrf_saadc_oversample_get()==NRF_SAADC_OVERSAMPLE_DISABLED) || (m_cb.active_channels == 0));
//	ASSERT((p_config->pin_p <= NRF_SAADC_INPUT_VDD) && (p_config->pin_p > NRF_SAADC_INPUT_DISABLED));
//	ASSERT(p_config->pin_n <= NRF_SAADC_INPUT_VDD);

	nrf_saadc_channel_init(channel, &channelConfig);
//	nrf_saadc_channel_input_set(channel, channelConfig.pin_p, channelConfig.pin_n); // Already done by channel_init()

	return ERR_SUCCESS;
}

void ADC::setDoneCallback(adc_done_cb_t callback) {
	_doneCallback = callback;
}

void ADC::stop() {
	LOGAdcDebug("stop");
	switch (_state) {
		case ADC_STATE_IDLE:
			LOGw("already stopped");
			return;
		default:
			break;
	}

	// TODO: state = stopping ?
	_state = ADC_STATE_IDLE;

#ifdef TEST_PIN_STOP
	nrf_gpio_pin_toggle(TEST_PIN_STOP);
#endif

	enterCriticalRegion();

	nrf_ppi_channel_disable(_ppiChannelStart);

	// Stop sample timer
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(0));
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_CLEAR);

	if (_saadcState != ADC_SAADC_STATE_IDLE) {
		LOGAdcDebug("stop saadc");
		_saadcState = ADC_SAADC_STATE_STOPPING;

		nrf_saadc_event_clear(NRF_SAADC_EVENT_STOPPED);
		nrf_saadc_int_enable(NRF_SAADC_INT_STOPPED);
		// The stop task triggers an END interrupt.
		nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);
		// Wait for ADC being stopped.
		while (_saadcState != ADC_SAADC_STATE_IDLE);
	}

	// The SAADC queue is now cleared, so move the queued buffers to the regular queue.
	while (!_saadcBufferQueue.empty()) {
		_bufferQueue.pushUnique(_saadcBufferQueue.pop());
	}
	printQueues();

	// Clear any pending saadc END event.
	// Doesn't prevent one more interrupt though?
	nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

	exitCriticalRegion();

#ifdef TEST_PIN_STOP
	nrf_gpio_pin_toggle(TEST_PIN_STOP);
#endif
}

void ADC::start() {
	LOGAdcDebug("start");
	switch (_state) {
//		case ADC_STATE_WAITING_TO_START:
		case ADC_STATE_BUSY:
			LOGw("already started");
			return;
		default:
			break;
	}

#ifdef TEST_PIN_START
	nrf_gpio_pin_toggle(TEST_PIN_START);
#endif

#ifdef DEBUG
	if (nrf_saadc_busy_check()) {
		LOGe("busy");
	}

	uint16_t numSamplesInBuffer = nrf_saadc_amount_get(); // Only valid in END or STOPPED interrupt.
	if (numSamplesInBuffer != 0 && numSamplesInBuffer != AdcBuffer::getInstance().getBufferLength()) {
		LOGw("Buffer not empty or full amount=%u", numSamplesInBuffer);
	}
#endif

	if (_changeConfig) {
		applyConfig();
	}

	cs_ret_code_t retCode = fillSaadcQueue();
	if (retCode != ERR_SUCCESS) {
		LOGAdcDebug("SAADC queue not full yet, wait for more buffers");
		_state = ADC_STATE_WAITING_TO_START;
		return;
	}

	// Start sampling.
	LOGAdcDebug("start sampling");
	_state = ADC_STATE_BUSY;
	_firstBuffer = true;

	// Start saadc on end event
	nrf_ppi_channel_enable(_ppiChannelStart);

	// Start sample timer
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_START);

#ifdef TEST_PIN_START
	nrf_gpio_pin_toggle(TEST_PIN_START);
#endif
}

cs_ret_code_t ADC::fillSaadcQueue() {
	enterCriticalRegion();
	cs_ret_code_t retCode = _fillSaadcQueue(false);
	exitCriticalRegion();
	return retCode;
}

cs_ret_code_t ADC::_fillSaadcQueue(bool fromInterrupt) {
	LOGAdcInterruptVerbose("fillSaadcQueue");
	cs_ret_code_t retCode = ERR_SUCCESS;

	switch (_saadcState) {
		case ADC_SAADC_STATE_STOPPING:
			// Don't fill the SAADC queue.
			return ERR_WRONG_STATE;
		case ADC_SAADC_STATE_IDLE:
		case ADC_SAADC_STATE_BUSY:
			break;
	}

	bool keepLooping = true;
	while (keepLooping && !_bufferQueue.empty()) {
		// Try to add a buffer from queue to the SAADC queue.
		auto bufIndex = _bufferQueue.peek();
		if (fromInterrupt) {
			retCode = _addBufferToSaadcQueue(bufIndex);
		}
		else {
			retCode = addBufferToSaadcQueue(bufIndex);
		}

		switch (retCode) {
			case ERR_SUCCESS:
			case ERR_ALREADY_EXISTS:
				// Buffer has been added to SAADC queue, so remove it from regular queue.
				_bufferQueue.pop();
				break;
			case ERR_NO_SPACE:
				// The SAADC queue is full, we can stop looping.
				keepLooping = false;
				break;
			default:
				// Stop on failure.
				LOGAdcInterruptWarn("Error %u", retCode);
				return retCode;
		}
	}

	printQueues();

	if (!_saadcBufferQueue.full()) {
		LOGAdcInterruptWarn("SAADC queue not full");
		return ERR_NOT_FOUND;
	}

	return ERR_SUCCESS;
}

cs_ret_code_t ADC::addBufferToSaadcQueue(adc_buffer_id_t bufIndex) {
	enterCriticalRegion();
	cs_ret_code_t retCode = _addBufferToSaadcQueue(bufIndex);
	exitCriticalRegion();
	return retCode;
}

cs_ret_code_t ADC::_addBufferToSaadcQueue(adc_buffer_id_t bufIndex) {
	LOGAdcInterruptVerbose("addBufferToSaadcQueue buf=%u", bufIndex);

	if (_saadcBufferQueue.size() >= CS_ADC_NUM_SAADC_BUFFERS) {
		LOGAdcInterruptVerbose("SAADC queue full");
		return ERR_NO_SPACE;
	}

	if (_saadcBufferQueue.find(bufIndex) != CS_CIRCULAR_BUFFER_INDEX_NOT_FOUND) {
		LOGAdcInterruptWarn("Buf already in SAADC queue");
		return ERR_ALREADY_EXISTS;
	}

	adc_buffer_t* buf = AdcBuffer::getInstance().getBuffer(bufIndex);
	nrf_saadc_value_t* samplesBuf = buf->samples;

	// This buffer is going to be filled with samples: set the config that's used, and mark invalid.
	for (int i=0; i<_config.channelCount; ++i) {
		buf->valid = false;
		buf->config[i] = _channelResultConfigs[i];
	}

	switch (_saadcState) {
		case ADC_SAADC_STATE_BUSY: {
			LOGAdcInterruptVerbose("queue buf");
			_saadcBufferQueue.push(bufIndex);
			{
				// Make sure to queue the next buffer only after the STARTED event
				// which should follow quickly after the START task.
				while (nrf_saadc_event_check(NRF_SAADC_EVENT_STARTED) == 0);
				nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
				nrf_saadc_buffer_init(samplesBuf, AdcBuffer::getInstance().getBufferLength());
			}
			break;
		}
		case ADC_SAADC_STATE_IDLE: {
			LOGAdcInterruptDebug("add buf and start");
			_saadcState = ADC_SAADC_STATE_BUSY;
			_saadcBufferQueue.push(bufIndex);
			nrf_saadc_buffer_init(samplesBuf, AdcBuffer::getInstance().getBufferLength());
			nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
			nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
			break;
		}
		default: {
			LOGAdcInterruptWarn("don't queue, wrong state: %u", _saadcState);
			return ERR_WRONG_STATE;
		}
	}
	return ERR_SUCCESS;
}

void ADC::printQueues() {
	if (ADC_LOG_QUEUES) {
		enterCriticalRegion();
		_log(SERIAL_DEBUG, false, "queued: ");
		for (uint8_t i = 0; i < _bufferQueue.size(); ++i) {
			_log(SERIAL_DEBUG, false, "%u, ", _bufferQueue[i]);
		}
		_log(SERIAL_DEBUG, true, "");

		_log(SERIAL_DEBUG, false, "saadc queue: ");
		for (uint8_t i = 0; i < _saadcBufferQueue.size(); ++i) {
			_log(SERIAL_DEBUG, false, "%u, ", _saadcBufferQueue[i]);
		}
		_log(SERIAL_DEBUG, true, "");
		exitCriticalRegion();
	}
}

void ADC::setZeroCrossingCallback(adc_zero_crossing_cb_t callback) {
	_zeroCrossingCallback = callback;
}

void ADC::enableZeroCrossingInterrupt(adc_channel_id_t channel, int32_t zeroVal) {
//	return;
	LOGd("enable zero chan=%u zero=%i", channel, zeroVal);
	_zeroValue = zeroVal;
	_zeroCrossingChannel = channel;
	_eventLimitLow = getLimitLowEvent(_zeroCrossingChannel);
	_eventLimitHigh = getLimitHighEvent(_zeroCrossingChannel);
	_zeroCrossingEnabled = true;
	setLimitUp();
}

cs_ret_code_t ADC::changeChannel(adc_channel_id_t channel, adc_channel_config_t& config) {
	if (channel >= _config.channelCount) {
		return ERR_ADC_INVALID_CHANNEL;
	}
	// Copy the channel config
	_config.channels[channel].pin = config.pin;
	_config.channels[channel].rangeMilliVolt = config.rangeMilliVolt;
	_config.channels[channel].referencePin = config.referencePin;
	_changeConfig = true;
	return ERR_SUCCESS;
}

void ADC::applyConfig() {
	LOGd("apply config");
	// NRF52_PAN_74
	nrf_saadc_disable();
	// Apply channel configs
	for (int i=0; i<_config.channelCount; ++i) {
		initChannel(i, _config.channels[i]);
	}
	nrf_saadc_enable();
	_changeConfig = false;

	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ADC_CONFIG, (uint8_t*)(&_config), sizeof(_config));
}

// No logs, this function can be called from interrupt
void ADC::setLimitUp() {
	nrf_saadc_channel_limits_set(_zeroCrossingChannel, LIMIT_LOW_DISABLED, _zeroValue);

	uint32_t int_mask = nrf_saadc_limit_int_get(_zeroCrossingChannel, NRF_SAADC_LIMIT_LOW);
	nrf_saadc_int_disable(int_mask);

	int_mask = nrf_saadc_limit_int_get(_zeroCrossingChannel, NRF_SAADC_LIMIT_HIGH);
	nrf_saadc_int_enable(int_mask);
}

// No logs, this function can be called from interrupt
void ADC::setLimitDown() {
	nrf_saadc_channel_limits_set(_zeroCrossingChannel, _zeroValue, LIMIT_HIGH_DISABLED);

	uint32_t int_mask = nrf_saadc_limit_int_get(_zeroCrossingChannel, NRF_SAADC_LIMIT_LOW);
	nrf_saadc_int_enable(int_mask);

	int_mask = nrf_saadc_limit_int_get(_zeroCrossingChannel, NRF_SAADC_LIMIT_HIGH);
	nrf_saadc_int_disable(int_mask);
}


void ADC::handleEvent(event_t & event) {
	switch (event.type) {
		default: {}
	}
}


void ADC::_restart() {
	LOGAdcDebug("restart");
	stop();
	start();
}


void ADC::_handleAdcDone(adc_buffer_id_t bufIndex) {
#ifdef TEST_PIN_PROCESS
	nrf_gpio_pin_toggle(TEST_PIN_PROCESS);
#endif

	if (dataCallbackRegistered()) {
		if (_firstBuffer) {
			LOGw("ADC restarted (ignore first warning on boot)");
			event_t event(CS_TYPE::EVT_ADC_RESTARTED, NULL, 0);
			event.dispatch();
		}
		_firstBuffer = false;

		LOGAdcVerbose("process buf %u", bufIndex);
		printQueues();

		_doneCallback(bufIndex);
	}
}

void ADC::enterCriticalRegion() {
	assert(_criticalRegionEntered < 100, "Exit and enter critical region unbalanced.");
	if (!_criticalRegionEntered) {
//		LOGAdcVerbose("disable interrupts");
		nrf_saadc_int_disable(NRF_SAADC_INT_END & NRF_SAADC_EVENT_STOPPED);
	}
	_criticalRegionEntered++;
}

void ADC::exitCriticalRegion() {
	_criticalRegionEntered--;
	assert(_criticalRegionEntered >= 0, "Exit and enter critical region unbalanced.");
	if (!_criticalRegionEntered) {
//		LOGAdcVerbose("enable interrupts");
		nrf_saadc_int_enable(NRF_SAADC_INT_END & NRF_SAADC_EVENT_STOPPED);
	}
}

// No logs, this function is called from interrupt
void ADC::_handleAdcLimitInterrupt(nrf_saadc_limit_t type) {
	if (type == NRF_SAADC_LIMIT_LOW) {
		// NRF_SAADC_LIMIT_LOW  triggers when adc value is below lower limit
		setLimitUp();
	}
	else {
		// NRF_SAADC_LIMIT_HIGH triggers when adc value is above upper limit
		setLimitDown();

#ifdef TEST_PIN_ZERO_CROSS
		nrf_gpio_pin_toggle(TEST_PIN_ZERO_CROSS);
#endif

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




void ADC::_handleAdcInterrupt() {
	if (nrf_saadc_event_check(NRF_SAADC_EVENT_END)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

#ifdef TEST_PIN_INT_END
		nrf_gpio_pin_toggle(TEST_PIN_INT_END);
#endif

		LOGAdcInterruptDebug("NRF_SAADC_EVENT_END");
		LOGAdcInterruptVerbose("end: amount=%u", nrf_saadc_amount_get()); // Only valid in END or STOPPED interrupt.

		if (_saadcState != ADC_SAADC_STATE_BUSY) {
			// Don't handle end events when state is not busy.
			// The end event also fires when we stop the saadc.
			LOGAdcInterruptInfo("Not busy: saadcState=%u", _saadcState);
			return;
		}

		if (_saadcBufferQueue.empty()) {
			// This shouldn't happen: a buffer is done, so it should be in the SAADC queue.
			LOGAdcInterruptWarn("No buffer");

			// Let's restart.
			_saadcState = ADC_SAADC_STATE_STOPPING;
			uint32_t errorCode = app_sched_event_put(NULL, 0, adc_restart);
			APP_ERROR_CHECK(errorCode);
			return;
		}

		// This buffer is no longer in use by saadc: move it to the buffer queue.
		adc_buffer_id_t bufIndex = _saadcBufferQueue.pop();

		// Mark buffer valid.
		// TODO: In case processing is really slow, it might be marked valid again while it's being processed.
		// Idea: Only have 1 call in the app scheduler, without buf index.
		//       Let processing loop over all valid buffers in _bufferQueue.
		AdcBuffer::getInstance().getBuffer(bufIndex)->valid = true;
		AdcBuffer::getInstance().getBuffer(bufIndex)->seqNr = _bufSeqNr++;

		_bufferQueue.pushUnique(bufIndex);

		// Decouple handling of buffer from adc interrupt handler, copy buffer index.
		uint32_t errorCode = app_sched_event_put(&bufIndex, sizeof(bufIndex), adc_done);
		// Don't stop application when it failed to put the buffer on the scheduler.
		// Simply don't put it on the scheduler and continue sampling.
//		APP_ERROR_CHECK(errorCode);
		if (errorCode != NRF_SUCCESS) {
			LOGAdcInterruptWarn("Failed to schedule");
		}

		LOGAdcInterruptDebug("Done bufIndex=%u queueSize=%u nextBuf=%u", bufIndex, _saadcBufferQueue.size(), _saadcBufferQueue.peek());

		if (_saadcBufferQueue.empty()) {
			// There is no buffer queued in the SAADC peripheral, so it has no more buffers to fill.

			// Stop sampling.
			nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_STOP);
			nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(0));
			nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_CLEAR);
//			nrf_ppi_channel_disable(_ppiChannelSample);

			LOGAdcInterruptInfo("No buffer queued");

			// Let's restart.
			_saadcState = ADC_SAADC_STATE_STOPPING;
			uint32_t errorCode = app_sched_event_put(NULL, 0, adc_restart);
			APP_ERROR_CHECK(errorCode);
			return;
		}

		if (_changeConfig) {
			// Don't add buffers to SAADC queue, so we can stop gracefully at the end of a buffer.
			// Since we don't queue a buffer to the SAADC, it will be empty next END interrupt, and thus will restart.
			return;
		}

		// We should have a buffer in queue for the SAADC.
		if (_fillSaadcQueue(true) != ERR_SUCCESS) {
			LOGAdcInterruptWarn("No buffer to queue");
			// Don't restart here, because the SAADC is already sampling to the next buffer.
			// Since we didn't queue a buffer to the SAADC, it will be empty next END interrupt, and thus will restart.
		}

		// SAADC will continue with sampling to the queued buffer.
		// The start task was called via PPI.
	}

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_STOPPED)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_STOPPED);
		LOGAdcInterruptDebug("NRF_SAADC_EVENT_STOPPED");
		_saadcState = ADC_SAADC_STATE_IDLE;
	}

	// No zero crossing events if we stop the SAADC.
	else if (_zeroCrossingEnabled) {
		if (nrf_saadc_event_check(_eventLimitLow)) {
			nrf_saadc_event_clear(_eventLimitLow);
			_handleAdcLimitInterrupt(NRF_SAADC_LIMIT_LOW);
		}
		if (nrf_saadc_event_check(_eventLimitHigh)) {
			nrf_saadc_event_clear(_eventLimitHigh);
			_handleAdcLimitInterrupt(NRF_SAADC_LIMIT_HIGH);
		}
	}
}


// SAADC interrupt handler
extern "C" void CS_ADC_IRQ(void) {
	ADC::getInstance()._handleAdcInterrupt();

	// Update the stack statistic in this interrupt, so that can be updated anywhere in the main loop code.
	Crownstone::updateMinStackEnd();
}

// Timer interrupt handler
extern "C" void CS_ADC_TIMER_IRQ(void) {
}

nrf_ppi_channel_t ADC::getPpiChannel(uint8_t index) {
	switch (index) {
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
		case 16:
			return NRF_PPI_CHANNEL16;
	}
	assert(false, "invalid ppi channel index");
	return NRF_PPI_CHANNEL0;
}

/**
 * The NC field disables the ADC and is actually set to value 0.
 * SAADC_CH_PSELP_PSELP_AnalogInput0 has value 1.
 */
nrf_saadc_input_t ADC::getAdcPin(const adc_pin_id_t pinNum) {
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
			assert(false, "invalid adc pin");
			return NRF_SAADC_INPUT_DISABLED;
	}
}

nrf_saadc_event_t ADC::getLimitLowEvent(adc_channel_id_t channel) {
	switch (channel) {
		case 0:
			return NRF_SAADC_EVENT_CH0_LIMITL;
		case 1:
			return NRF_SAADC_EVENT_CH1_LIMITL;
		case 2:
			return NRF_SAADC_EVENT_CH2_LIMITL;
		case 3:
			return NRF_SAADC_EVENT_CH3_LIMITL;
		case 4:
			return NRF_SAADC_EVENT_CH4_LIMITL;
		case 5:
			return NRF_SAADC_EVENT_CH5_LIMITL;
		case 6:
			return NRF_SAADC_EVENT_CH6_LIMITL;
		case 7:
			return NRF_SAADC_EVENT_CH7_LIMITL;
	}
	assert(false, "invalid limit low channel");
	return NRF_SAADC_EVENT_CH7_LIMITL;
}

nrf_saadc_event_t ADC::getLimitHighEvent(adc_channel_id_t channel) {
	switch (channel) {
		case 0:
			return NRF_SAADC_EVENT_CH0_LIMITH;
		case 1:
			return NRF_SAADC_EVENT_CH1_LIMITH;
		case 2:
			return NRF_SAADC_EVENT_CH2_LIMITH;
		case 3:
			return NRF_SAADC_EVENT_CH3_LIMITH;
		case 4:
			return NRF_SAADC_EVENT_CH4_LIMITH;
		case 5:
			return NRF_SAADC_EVENT_CH5_LIMITH;
		case 6:
			return NRF_SAADC_EVENT_CH6_LIMITH;
		case 7:
			return NRF_SAADC_EVENT_CH7_LIMITH;
	}
	assert(false, "invalid limit high channel");
	return NRF_SAADC_EVENT_CH7_LIMITH;
}

nrf_gpiote_tasks_t ADC::getGpioteTaskOut(uint8_t index) {
	switch (index) {
		case 0:
			return NRF_GPIOTE_TASKS_OUT_0;
		case 1:
			return NRF_GPIOTE_TASKS_OUT_1;
		case 2:
			return NRF_GPIOTE_TASKS_OUT_2;
		case 3:
			return NRF_GPIOTE_TASKS_OUT_3;
		case 4:
			return NRF_GPIOTE_TASKS_OUT_4;
		case 5:
			return NRF_GPIOTE_TASKS_OUT_5;
		case 6:
			return NRF_GPIOTE_TASKS_OUT_6;
		case 7:
			return NRF_GPIOTE_TASKS_OUT_7;
	}
	assert(false, "invalid gpiote task index");
	return NRF_GPIOTE_TASKS_OUT_0;
}
