/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_Strings.h>
#include <drivers/cs_ADC.h>
#include <drivers/cs_RTC.h>
#include <drivers/cs_Serial.h>
#include <protocol/cs_ErrorCodes.h>
#include <protocol/cs_UartProtocol.h>
#include <structs/buffer/cs_InterleavedBuffer.h>
#include <util/cs_BleError.h>

#define LOGAdcDebug LOGd
#define LOGAdcVerbose LOGd
#define LOGAdcInterrupt LOGnone

// Define test pin to enable gpio debug.
//#define TEST_PIN_ZERO_CROSS 20
//#define TEST_PIN_TIMEOUT    20 // Controlled via GPIOTE
//#define TEST_PIN_START      22
//#define TEST_PIN_STOP       23
//#define TEST_PIN_PROCESS    24
//#define TEST_PIN_INT_END    25


// Defines specific for implementation
#define LIMIT_LOW_DISABLED  (-2048) // min adc value
#define LIMIT_HIGH_DISABLED (4095)  // max adc value
#define BUFFER_INDEX_NONE 255


// Called by app scheduler, from saadc interrupt.
void adc_done(void * p_event_data, uint16_t event_size) {
	buffer_id_t* bufIndex = (buffer_id_t*)p_event_data;
	ADC::getInstance()._handleAdcDone(*bufIndex);
}

// Called by app scheduler, from saadc interrupt.
void adc_restart(void * p_event_data, uint16_t event_size) {
	ADC::getInstance()._restart();
}

// Called by app scheduler, from timeout timer interrupt.
void adc_timeout(void* pEventData, uint16_t dataSize) {
	ADC::getInstance()._handleTimeout();
}

ADC::ADC() :
		_changeConfig(false),
		_bufferQueue(CS_ADC_NUM_BUFFERS),
		_saadcBufferQueue(CS_ADC_NUM_SAADC_BUFFERS),
		_firstBuffer(true),
		_state(ADC_STATE_IDLE),
		_saadcState(ADC_SAADC_STATE_IDLE),
		_zeroCrossingChannel(0),
		_zeroCrossingEnabled(false),
		_lastZeroCrossUpTime(0),
		_zeroValue(0)
{
	_doneCallback = NULL;
	_zeroCrossingCallback = NULL;
	for (int i=0; i<CS_ADC_NUM_BUFFERS; i++) {
		_inProgress[i] = false;
	}
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
	LOGi("init: period=%uus", _config.samplingPeriodUs);

#ifdef TEST_PIN_ZERO_CROSS
	nrf_gpio_cfg_output(TEST_PIN_ZERO_CROSS);
#endif
//#ifdef TEST_PIN_TIMEOUT
//	nrf_gpio_cfg_output(TEST_PIN_TIMEOUT);
//#endif
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
	uint32_t ticks = nrf_timer_us_to_ticks(_config.samplingPeriodUs, CS_ADC_TIMER_FREQ);
	LOGv("ticks=%u", ticks);
	nrf_timer_cc_write(CS_ADC_TIMER, NRF_TIMER_CC_CHANNEL0, ticks);
	nrf_timer_mode_set(CS_ADC_TIMER, NRF_TIMER_MODE_TIMER);
	nrf_timer_shorts_enable(CS_ADC_TIMER, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(0));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(1));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(2));
	nrf_timer_event_clear(CS_ADC_TIMER, nrf_timer_compare_event_get(3));

	// Setup PPI: on timer compare event, call adc sample task, and count for timeout.
	_ppiChannelSample = getPpiChannel(CS_ADC_PPI_CHANNEL_START);
	nrf_ppi_channel_and_fork_endpoint_setup(
			_ppiChannelSample,
			(uint32_t)nrf_timer_event_address_get(CS_ADC_TIMER, nrf_timer_compare_event_get(0)),
			nrf_saadc_task_address_get(NRF_SAADC_TASK_SAMPLE),
			(uint32_t)nrf_timer_task_address_get(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_TASK_COUNT)
	);
	nrf_ppi_channel_enable(_ppiChannelSample);


	// -------------------
	// Setup timeout timer
	// -------------------
	nrf_timer_task_trigger(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_bit_width_set(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_BIT_WIDTH_32);
	uint32_t timeoutCount = CS_ADC_BUF_SIZE / _config.channelCount - 1 - CS_ADC_TIMEOUT_SAMPLES; // Timeout N samples before END event.
	LOGv("timeoutCount=%u", timeoutCount);
	nrf_timer_cc_write(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_CC_CHANNEL0, timeoutCount);
	nrf_timer_mode_set(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_MODE_COUNTER);
	nrf_timer_event_clear(CS_ADC_TIMEOUT_TIMER, nrf_timer_compare_event_get(0));
	nrf_timer_event_clear(CS_ADC_TIMEOUT_TIMER, nrf_timer_compare_event_get(1));
	nrf_timer_event_clear(CS_ADC_TIMEOUT_TIMER, nrf_timer_compare_event_get(2));
	nrf_timer_event_clear(CS_ADC_TIMEOUT_TIMER, nrf_timer_compare_event_get(3));

	// Setup short: stop the timeout timer on compare event (timeout).
	nrf_timer_shorts_enable(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_SHORT_COMPARE0_STOP_MASK);

	// Setup timeout PPI: on compare event (timeout), stop the sample timer.
	_ppiTimeout = getPpiChannel(CS_ADC_PPI_CHANNEL_START+2);
#ifdef TEST_PIN_TIMEOUT
	nrf_gpiote_task_configure(CS_ADC_GPIOTE_CHANNEL_START, TEST_PIN_TIMEOUT, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_task_enable(CS_ADC_GPIOTE_CHANNEL_START);
	nrf_ppi_channel_and_fork_endpoint_setup(
			_ppiTimeout,
			(uint32_t)nrf_timer_event_address_get(CS_ADC_TIMEOUT_TIMER, nrf_timer_compare_event_get(0)),
			(uint32_t)nrf_timer_task_address_get(CS_ADC_TIMER, NRF_TIMER_TASK_STOP),
			nrf_gpiote_task_addr_get(getGpioteTaskOut(CS_ADC_GPIOTE_CHANNEL_START))
	);
#else
	nrf_ppi_channel_endpoint_setup(
			_ppiTimeout,
			(uint32_t)nrf_timer_event_address_get(CS_ADC_TIMEOUT_TIMER, nrf_timer_compare_event_get(0)),
			(uint32_t)nrf_timer_task_address_get(CS_ADC_TIMER, NRF_TIMER_TASK_STOP)
	);
#endif
	nrf_ppi_channel_enable(_ppiTimeout);

	// Setup PPI: on saadc end event, reset sample count, and start the timer.
	_ppiTimeoutStart = getPpiChannel(CS_ADC_PPI_CHANNEL_START+3);
	nrf_ppi_channel_and_fork_endpoint_setup(
			_ppiTimeoutStart,
			(uint32_t)nrf_saadc_event_address_get(NRF_SAADC_EVENT_END),
			(uint32_t)nrf_timer_task_address_get(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_TASK_CLEAR),
			(uint32_t)nrf_timer_task_address_get(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_TASK_START)
	);

	// Enable timeout timer interrupt.
	err_code = sd_nvic_SetPriority(CS_ADC_TIMEOUT_TIMER_IRQn, CS_ADC_TIMEOUT_TIMER_IRQ_PRIORITY);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_EnableIRQ(CS_ADC_TIMEOUT_TIMER_IRQn);
	APP_ERROR_CHECK(err_code);
	nrf_timer_int_enable(CS_ADC_TIMEOUT_TIMER, nrf_timer_compare_int_get(0));


	// -------------------
	// Setup SAADC
	// -------------------

	// Setup PPI: call START on END.
	// This avoids a SAMPLE being called before the START.
	// See https://devzone.nordicsemi.com/f/nordic-q-a/20291/offset-in-saadc-samples-with-easy-dma-and-ble/79053
	_ppiChannelStart = getPpiChannel(CS_ADC_PPI_CHANNEL_START+1);
	nrf_ppi_channel_endpoint_setup(
			_ppiChannelStart,
			(uint32_t)nrf_saadc_event_address_get(NRF_SAADC_EVENT_END),
			nrf_saadc_task_address_get(NRF_SAADC_TASK_START)
	);

	// Config adc
	err_code = initSaadc(config);
	APP_ERROR_CHECK(err_code);

	for (int i=0; i<_config.channelCount; ++i) {
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
	cs_ret_code_t retCode = InterleavedBuffer::getInstance().init();
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}
	buffer_id_t bufCount = InterleavedBuffer::getInstance().getBufferCount();
	for (buffer_id_t id = 0; id < bufCount; ++id) {
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
cs_ret_code_t ADC::initChannel(channel_id_t channel, adc_channel_config_t& config) {
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
		channelConfig.gain = NRF_SAADC_GAIN4;
	}
	else if (config.rangeMilliVolt <= 300) {
		LOGv("gain=2 range=300mV");
		channelConfig.gain = NRF_SAADC_GAIN2;
	}
	else if (config.rangeMilliVolt <= 600) {
		LOGv("gain=1 range=600mV");
		channelConfig.gain = NRF_SAADC_GAIN1;
	}
	else if (config.rangeMilliVolt <= 1200) {
		LOGv("gain=1/2 range=1200mV");
		channelConfig.gain = NRF_SAADC_GAIN1_2;
	}
	else if (config.rangeMilliVolt <= 1800) {
		LOGv("gain=1/3 range=1800mV");
		channelConfig.gain = NRF_SAADC_GAIN1_3;
	}
	else if (config.rangeMilliVolt <= 2400) {
		LOGv("gain=1/4 range=2400mV");
		channelConfig.gain = NRF_SAADC_GAIN1_4;
	}
	else if (config.rangeMilliVolt <= 3000) {
		LOGv("gain=1/5 range=3000mV");
		channelConfig.gain = NRF_SAADC_GAIN1_5;
	}
//	else if (config.rangeMilliVolt <= 3600) {
	else {
		LOGv("gain=1/6 range=3600mV");
		channelConfig.gain = NRF_SAADC_GAIN1_6;
	}


	channelConfig.reference = NRF_SAADC_REFERENCE_INTERNAL;
	channelConfig.acq_time = NRF_SAADC_ACQTIME_10US;
	if (config.referencePin == CS_ADC_REF_PIN_NOT_AVAILABLE) {
		LOGv("single ended");
		channelConfig.mode = NRF_SAADC_MODE_SINGLE_ENDED;
		channelConfig.pin_n = NRF_SAADC_INPUT_DISABLED;
	}
	else {
		LOGv("differential");
		channelConfig.mode = NRF_SAADC_MODE_DIFFERENTIAL;
		channelConfig.pin_n = getAdcPin(config.referencePin);
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

	// Make sure the end interrupt doesn't interrupt this piece of code.
	nrf_saadc_int_disable(NRF_SAADC_INT_END);
	// TODO: state = stopping ?
	_state = ADC_STATE_IDLE;

#ifdef TEST_PIN_STOP
	nrf_gpio_pin_toggle(TEST_PIN_STOP);
#endif

	nrf_ppi_channel_disable(_ppiChannelStart);
	nrf_ppi_channel_disable(_ppiTimeoutStart);
	stopTimeout();

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

	// Clear any pending saadc END event.
	nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

	// Re-enable end interrupt.
	nrf_saadc_int_enable(NRF_SAADC_INT_END);

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

	bool inProgress = false;
	for (uint8_t i=0; i<CS_ADC_NUM_BUFFERS; ++i) {
		if (_inProgress[i]) {
			inProgress = true;
		}
	}
	if (inProgress) {
		// Wait for buffers to be released.
//		_state = ADC_STATE_WAITING_TO_START;
		LOGAdcDebug("wait to start");
//		return;
	}

	cs_ret_code_t retCode = fillSaadcQueue();
	if (retCode != ERR_SUCCESS) {
		LOGAdcDebug("SAADC queue not full yet, wait for more buffers");
		_state = ADC_STATE_WAITING_TO_START;
		return;
	}

	// Start sampling.
	_state = ADC_STATE_BUSY;
	_firstBuffer = true;

	// Start saadc on end event
	nrf_ppi_channel_enable(_ppiChannelStart);

	// Clear timeout counter
	nrf_timer_task_trigger(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_TASK_CLEAR);

	// Start timeout on saadc end event
	nrf_ppi_channel_enable(_ppiTimeoutStart);

	// Start sample timer
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_START);

#ifdef TEST_PIN_START
	nrf_gpio_pin_toggle(TEST_PIN_START);
#endif
}

cs_ret_code_t ADC::fillSaadcQueue() {
	LOGAdcVerbose("fillSaadcQueue");
	cs_ret_code_t retCode = ERR_SUCCESS;
	while (!_bufferQueue.empty()) {
		// Try to add a buffer from queue to the SAADC queue.
		auto bufIndex = _bufferQueue.peek();
		retCode = addBufferToSaadcQueue(bufIndex);
		switch (retCode) {
			case ERR_SUCCESS:
			case ERR_ALREADY_EXISTS:
				// Buffer has been added to SAADC queue, so remove it from regular queue.
				_bufferQueue.pop();
				break;
			case ERR_NO_SPACE:
				// The SAADC queue is full, we can stop.
				return ERR_SUCCESS;
			default:
				// Stop on failure.
				LOGAdcVerbose("err %u", retCode);
				return retCode;
		}
	}

	nrf_saadc_int_disable(NRF_SAADC_INT_END);
	bool saadcQueueFull = _saadcBufferQueue.full();
	nrf_saadc_int_enable(NRF_SAADC_INT_END);

	if (!saadcQueueFull) {
		LOGAdcDebug("SAADC queue not full");
		return ERR_NOT_FOUND;
	}

	stopTimeout();
	return ERR_SUCCESS;
}

cs_ret_code_t ADC::addBufferToSaadcQueue(buffer_id_t bufIndex) {
	LOGAdcVerbose("addBufferToSaadcQueue buf=%u", bufIndex);
	if (_inProgress[bufIndex]) {
		LOGe("Buffer %u in progress", bufIndex);
//		APP_ERROR_CHECK(NRF_ERROR_BUSY);
//		return ERR_WRONG_PARAMETER;
	}

	// Make sure the interrupt doesn't interrupt this piece of code.
	// All variables that are used in interrupt should only be used with interrupt disabled.
	nrf_saadc_int_disable(NRF_SAADC_INT_END);

	if (_saadcBufferQueue.size() >= CS_ADC_NUM_SAADC_BUFFERS) {
		LOGAdcVerbose("SAADC queue full");
		nrf_saadc_int_enable(NRF_SAADC_INT_END);
		return ERR_NO_SPACE;
	}

	if (_saadcBufferQueue.find(bufIndex) != CS_CIRCULAR_BUFFER_INDEX_NOT_FOUND) {
		LOGAdcDebug("Buf already in SAADC queue");
		nrf_saadc_int_enable(NRF_SAADC_INT_END);
		return ERR_ALREADY_EXISTS;
	}

	nrf_saadc_value_t* buf = InterleavedBuffer::getInstance().getBuffer(bufIndex);

	switch (_saadcState) {
		case ADC_SAADC_STATE_BUSY: {
			LOGAdcDebug("queue buf");
			_saadcBufferQueue.push(bufIndex);
			{
				// Make sure to queue the next buffer only after the STARTED event
				// which should follow quickly after the START task.
				while (nrf_saadc_event_check(NRF_SAADC_EVENT_STARTED) == 0);
				nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
				nrf_saadc_buffer_init(buf, CS_ADC_BUF_SIZE);
			}
			nrf_saadc_int_enable(NRF_SAADC_INT_END);
			break;
		}
		case ADC_SAADC_STATE_IDLE: {
			LOGAdcDebug("add buf and start");
			_saadcState = ADC_SAADC_STATE_BUSY;
			_saadcBufferQueue.push(bufIndex);
			nrf_saadc_int_enable(NRF_SAADC_INT_END);
			nrf_saadc_buffer_init(buf, CS_ADC_BUF_SIZE);
			nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
			nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
			break;
		}
		default: {
			LOGAdcDebug("don't queue, wrong state: %u", _saadcState);
			nrf_saadc_int_enable(NRF_SAADC_INT_END);
			return ERR_WRONG_STATE;
		}
	}
	return ERR_SUCCESS;
}

void ADC::releaseBuffer(buffer_id_t bufIndex) {
	LOGAdcVerbose("Release buf %u", bufIndex);

#ifdef TEST_PIN_PROCESS
	nrf_gpio_pin_toggle(TEST_PIN_PROCESS);
#endif

	_inProgress[bufIndex] = false;

	_bufferQueue.pushUnique(bufIndex);

	switch (_state) {
		case ADC_STATE_WAITING_TO_START:
			// Try to start now.
			start();
			return;
		case ADC_STATE_BUSY:
			// Continue sampling, fill up the SAADC queue.
			fillSaadcQueue();
			return;
		case ADC_STATE_IDLE:
		case ADC_STATE_READY_TO_START:
		default:
			// We want to stop, so don't add buffers to SAADC queue.
			LOGAdcDebug("not running, don't fill saadc queue");
			return;
	}
}

void ADC::setZeroCrossingCallback(adc_zero_crossing_cb_t callback) {
	_zeroCrossingCallback = callback;
}

void ADC::enableZeroCrossingInterrupt(channel_id_t channel, int32_t zeroVal) {
	LOGv("enable zero chan=%u zero=%i", channel, zeroVal);
	_zeroValue = zeroVal;
	_zeroCrossingChannel = channel;
	_eventLimitLow = getLimitLowEvent(_zeroCrossingChannel);
	_eventLimitHigh = getLimitHighEvent(_zeroCrossingChannel);
	_zeroCrossingEnabled = true;
	setLimitUp();
}

cs_ret_code_t ADC::changeChannel(channel_id_t channel, adc_channel_config_t& config) {
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
	// NRF52_PAN_74
	nrf_saadc_disable();
	// Apply channel configs
	for (int i=0; i<_config.channelCount; ++i) {
		initChannel(i, _config.channels[i]);
	}
	nrf_saadc_enable();

	UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_ADC_CONFIG, (uint8_t*)(&_config), sizeof(_config));
	_changeConfig = false;
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

void ADC::stopTimeout() {
	nrf_timer_task_trigger(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_TASK_STOP);
}


void ADC::handleEvent(event_t & event) {
	switch(event.type) {
		default: {}
	}
}


void ADC::_restart() {
	LOGAdcDebug("restart");
	stop();
	start();
}

void ADC::_handleTimeout() {
	LOGAdcDebug("timeout");
	stop();
	start();
}

void ADC::_handleAdcDone(buffer_id_t bufIndex) {
#ifdef TEST_PIN_PROCESS
	nrf_gpio_pin_toggle(TEST_PIN_PROCESS);
#endif

	if (dataCallbackRegistered()) {
		_inProgress[bufIndex] = true;

		if (_firstBuffer) {
			LOGw("ADC restarted");
			event_t event(CS_TYPE::EVT_ADC_RESTARTED, NULL, 0);
			event.dispatch();
		}
		_firstBuffer = false;

		if (_changeConfig) {
			stop();
			applyConfig();
			start();
		}

		LOGAdcVerbose("process buf %u", bufIndex);
		_doneCallback(bufIndex);
	}
	else {
		// Skip the callback: release immediately.
		releaseBuffer(bufIndex);
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



void ADC::_handleTimeoutInterrupt() {
//#ifdef TEST_PIN_TIMEOUT
//	nrf_gpio_pin_toggle(TEST_PIN_TIMEOUT);
//#endif
	// Decouple timeout handling from interrupt handler.
	uint32_t errorCode = app_sched_event_put(NULL, 0, adc_timeout);
	APP_ERROR_CHECK(errorCode);
}



void ADC::_handleAdcInterrupt() {
	if (nrf_saadc_event_check(NRF_SAADC_EVENT_END)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

#ifdef TEST_PIN_INT_END
		nrf_gpio_pin_toggle(TEST_PIN_INT_END);
#endif

		LOGAdcInterrupt("NRF_SAADC_EVENT_END");
		if (_saadcState != ADC_SAADC_STATE_BUSY) {
			// Don't handle end events when state is not busy.
			// The end event also fires when we stop the saadc.
			LOGAdcInterrupt("Not busy: saadcState=%u", _saadcState);
			return;
		}

		if (_saadcBufferQueue.empty()) {
			// This shouldn't happen: a buffer is done, so it should be in the SAADC queue.
			LOGAdcDebug("No buffer");

			// Let's restart.
			_saadcState = ADC_SAADC_STATE_STOPPING;
			uint32_t errorCode = app_sched_event_put(NULL, 0, adc_restart);
			APP_ERROR_CHECK(errorCode);
			return;
		}

		// Decouple handling of buffer from adc interrupt handler, copy buffer index.
		buffer_id_t bufIndex = _saadcBufferQueue.pop();
		uint32_t errorCode = app_sched_event_put(&bufIndex, sizeof(bufIndex), adc_done);
		APP_ERROR_CHECK(errorCode);

		__attribute__((unused)) uint16_t bufSize = nrf_saadc_amount_get();
		LOGAdcInterrupt("Done bufIndex=%u queueSize=%u queued=%u", bufIndex, _saadcBufferQueue.size(), _saadcBufferQueue.peek());

		if (_saadcBufferQueue.empty()) {
			// There is no buffer queued in the SAADC peripheral, so it has no more buffers to fill.
			LOGAdcInterrupt("No buffer queued");

			// Let's restart.
			_saadcState = ADC_SAADC_STATE_STOPPING;
			uint32_t errorCode = app_sched_event_put(NULL, 0, adc_restart);
			APP_ERROR_CHECK(errorCode);
			return;
		}

		// SAADC will continue with sampling to the queued buffer.
		// The start task is called via PPI.
	}
	if (nrf_saadc_event_check(NRF_SAADC_EVENT_STOPPED)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_STOPPED);
		LOGAdcInterrupt("NRF_SAADC_EVENT_STOPPED");
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
}

// Timer interrupt handler
extern "C" void CS_ADC_TIMER_IRQ(void) {
}

extern "C" void CS_ADC_TIMEOUT_TIMER_IRQ(void) {
	ADC::getInstance()._handleTimeoutInterrupt();
	nrf_timer_event_clear(CS_ADC_TIMEOUT_TIMER, nrf_timer_compare_event_get(0));
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

nrf_saadc_event_t ADC::getLimitLowEvent(channel_id_t channel) {
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
	return NRF_SAADC_EVENT_CH7_LIMITL;
}

nrf_saadc_event_t ADC::getLimitHighEvent(channel_id_t channel) {
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
	return NRF_SAADC_EVENT_CH7_LIMITH;
}

nrf_gpiote_tasks_t ADC::getGpioteTaskOut(uint8_t index) {
	switch(index) {
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
	APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
	return NRF_GPIOTE_TASKS_OUT_0;
}
