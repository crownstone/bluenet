/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
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
#include "structs/buffer/cs_InterleavedBuffer.h"
#include "events/cs_EventDispatcher.h"

// Define to print adc restarts
//#define PRINT_DEBUG

// Define to print buffers being queued etc.
//#define PRINT_DEBUG_VERBOSE


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
	cs_adc_buffer_id_t* bufIndex = (cs_adc_buffer_id_t*)p_event_data;
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
		_bufferIndex(BUFFER_INDEX_NONE),
		_queuedBufferIndex(BUFFER_INDEX_NONE),
		_numBuffersQueued(0),
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
		InterleavedBuffer::getInstance().setBuffer(0, NULL);
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
cs_adc_error_t ADC::init(const adc_config_t & config) {
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
	LOGd("ticks=%u", ticks);
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
//	nrf_timer_frequency_set(CS_ADC_TIMER, CS_ADC_TIMEOUT_TIMER_FREQ);
	uint32_t timeoutCount = CS_ADC_BUF_SIZE / _config.channelCount - 2; // Timeout 2 samples before END event
	LOGd("timeoutCount=%u", timeoutCount);
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
	err_code = initAdc(config);
	APP_ERROR_CHECK(err_code);

	for (int i=0; i<_config.channelCount; ++i) {
		initChannel(i, _config.channels[i]);
	}

	// Allocate buffers
	for (int i=0; i<CS_ADC_NUM_BUFFERS; i++) {
		nrf_saadc_value_t* buf = new nrf_saadc_value_t[CS_ADC_BUF_SIZE];
		LOGd("Allocate buffer %i = %p", i, buf);
		InterleavedBuffer::getInstance().setBuffer(i, buf);
		_inProgress[i] = false;
	}
	initQueue();

	EventDispatcher::getInstance().addListener(this);

	return 0;
}


cs_adc_error_t ADC::initAdc(const adc_config_t & config) {
	nrf_saadc_resolution_set(CS_ADC_RESOLUTION);
	nrf_saadc_oversample_set(NRF_SAADC_OVERSAMPLE_DISABLED); // Oversample can only be used with 1 channel.

	nrf_saadc_int_disable(NRF_SAADC_INT_ALL);
	nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

	// Enable interrupts
	sd_nvic_SetPriority(SAADC_IRQn, CS_ADC_IRQ_PRIORITY);
	sd_nvic_ClearPendingIRQ(SAADC_IRQn);
	sd_nvic_EnableIRQ(SAADC_IRQn);
	nrf_saadc_int_enable(NRF_SAADC_INT_END);

	nrf_saadc_enable();

	return 0;
}

void ADC::initQueue() {
	_bufferIndex = 0;
	_queuedBufferIndex = 1;
}

/** Configure an ADC channel.
 *
 *   - set acquire time to 10 micro seconds
 *   - use the internal VGB reference of 0.6V (not the external one, so no need to use its multiplexer either)
 *   - set the prescaler for the input voltage (the input, not the input supply)
 *   - set either differential mode (pin - ref_pin), or single ended mode (pin - 0)
 */
cs_adc_error_t ADC::initChannel(cs_adc_channel_id_t channel, adc_channel_config_t& config) {
	LOGi("init channel %u on ain%u, range=%umV, ref=ain%u", channel, config.pin, config.rangeMilliVolt, config.referencePin);
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

	return 0;
}

void ADC::setDoneCallback(adc_done_cb_t callback) {
	_doneCallback = callback;
}

void ADC::stop() {
#ifdef PRINT_DEBUG
	LOGd("stop");
#endif
	switch (_state) {
	case ADC_STATE_IDLE:
		LOGw("already stopped");
		return;
	default:
		break;
	}
	nrf_saadc_int_disable(NRF_SAADC_INT_END); // Make sure the interrupt doesn't interrupt this piece of code.
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
#ifdef PRINT_DEBUG_VERBOSE
		LOGd("stop saadc");
#endif
		_saadcState = ADC_SAADC_STATE_STOPPING;

		nrf_saadc_event_clear(NRF_SAADC_EVENT_STOPPED);
		nrf_saadc_int_enable(NRF_SAADC_INT_STOPPED);
		// The stop task triggers an END interrupt.
		nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);
		// Wait for ADC being stopped.
		while (_saadcState != ADC_SAADC_STATE_IDLE);
	}
	_numBuffersQueued = 0;
	nrf_saadc_event_clear(NRF_SAADC_EVENT_END); // Clear any pending saadc END event.
	nrf_saadc_int_enable(NRF_SAADC_INT_END); // Re-enable end interrupt.
#ifdef TEST_PIN_STOP
	nrf_gpio_pin_toggle(TEST_PIN_STOP);
#endif
}

void ADC::start() {
#ifdef PRINT_DEBUG
	LOGd("start");
#endif
	switch (_state) {
	case ADC_STATE_WAITING_TO_START:
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
		_state = ADC_STATE_WAITING_TO_START;
#ifdef PRINT_DEBUG_VERBOSE
		LOGd("wait to start");
#endif
		return;
	}
	_state = ADC_STATE_BUSY;

	if (_bufferIndex != BUFFER_INDEX_NONE) {
		// Resume from previous index.
		if (_queuedBufferIndex != BUFFER_INDEX_NONE) {
			// Store queued buffer index, as it will be overwritten by addBufferToSampleQueue()
			cs_adc_buffer_id_t queuedBufferIndex = _queuedBufferIndex;
			addBufferToSampleQueue(_bufferIndex);
			addBufferToSampleQueue(queuedBufferIndex);
		}
		else {
			LOGw("no queued buffer?");
			addBufferToSampleQueue(_bufferIndex);
			addBufferToSampleQueue((_bufferIndex + 1) % CS_ADC_NUM_BUFFERS);
		}
	}
	else {
		LOGe("no buffer set");
		APP_ERROR_CHECK(NRF_ERROR_INVALID_STATE);
	}
	_firstBuffer = true;

	nrf_ppi_channel_enable(_ppiChannelStart); // Start saadc on end event
	nrf_timer_task_trigger(CS_ADC_TIMEOUT_TIMER, NRF_TIMER_TASK_CLEAR); // Clear timeout counter
	nrf_ppi_channel_enable(_ppiTimeoutStart); // Start timeout on saadc end event
	nrf_timer_task_trigger(CS_ADC_TIMER, NRF_TIMER_TASK_START); // Start sample timer
#ifdef TEST_PIN_START
	nrf_gpio_pin_toggle(TEST_PIN_START);
#endif
}

void ADC::addBufferToSampleQueue(cs_adc_buffer_id_t bufIndex) {
	if (_inProgress[bufIndex]) {
		LOGe("Buffer %u in progress", bufIndex);
		APP_ERROR_CHECK(NRF_ERROR_BUSY);
		return;
	}

	nrf_saadc_int_disable(NRF_SAADC_INT_END); // Make sure the interrupt doesn't interrupt this piece of code.
#ifdef PRINT_DEBUG_VERBOSE
	LOGd("Queued: %u", _numBuffersQueued);
	LOGd("Queue buf %u", bufIndex);
#endif
	if (_numBuffersQueued > 1) { // TODO: check _queuedBufferIndex instead?
		nrf_saadc_int_enable(NRF_SAADC_INT_END);
		LOGe("Too many buffers queued %u", _numBuffersQueued);
		APP_ERROR_CHECK(NRF_ERROR_RESOURCES);
		return;
	}

	nrf_saadc_value_t* buf = InterleavedBuffer::getInstance().getBuffer(bufIndex);

	switch (_saadcState) {
	case ADC_SAADC_STATE_BUSY: {
		if (_queuedBufferIndex != BUFFER_INDEX_NONE) {
			nrf_saadc_int_enable(NRF_SAADC_INT_END);
			LOGe("Second buffer already queued");
			APP_ERROR_CHECK(NRF_ERROR_BUSY);
			return;
		}
		_queuedBufferIndex = bufIndex;
		_numBuffersQueued++;
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
		_saadcState = ADC_SAADC_STATE_BUSY;
		_bufferIndex = bufIndex;
		_queuedBufferIndex = BUFFER_INDEX_NONE;
		_numBuffersQueued++;
		nrf_saadc_int_enable(NRF_SAADC_INT_END);
		nrf_saadc_buffer_init(buf, CS_ADC_BUF_SIZE);
		nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
		nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
		break;
	}
	default: {
#ifdef PRINT_DEBUG_VERBOSE
		LOGw("don't queue");
#endif
	}
	}
}

bool ADC::releaseBuffer(cs_adc_buffer_id_t bufIndex) {
#ifdef TEST_PIN_PROCESS
	nrf_gpio_pin_toggle(TEST_PIN_PROCESS);
#endif
#ifdef PRINT_DEBUG_VERBOSE
	LOGd("Release buf %u", bufIndex);
#endif
	_inProgress[bufIndex] = false;

	if (_state == ADC_STATE_WAITING_TO_START) {
		_state = ADC_STATE_READY_TO_START;
		start();
		return true;
	}

	if (_state != ADC_STATE_BUSY) {
		LOGd("not running, don't queue buf");
		return true;
	}

	cs_adc_buffer_id_t nextIndex = (bufIndex + 2) % CS_ADC_NUM_BUFFERS; // TODO: 2 should be (CS_ADC_NUM_BUFFERS - saadc queue size)?
	addBufferToSampleQueue(nextIndex);
	stopTimeout();
	return true;
}

void ADC::setZeroCrossingCallback(adc_zero_crossing_cb_t callback) {
	_zeroCrossingCallback = callback;
}

void ADC::enableZeroCrossingInterrupt(cs_adc_channel_id_t channel, int32_t zeroVal) {
	LOGd("enable zero chan=%u zero=%i", channel, zeroVal);
	_zeroValue = zeroVal;
	_zeroCrossingChannel = channel;
	_eventLimitLow = getLimitLowEvent(_zeroCrossingChannel);
	_eventLimitHigh = getLimitHighEvent(_zeroCrossingChannel);
	_zeroCrossingEnabled = true;
	setLimitUp();
}

cs_adc_error_t ADC::changeChannel(cs_adc_channel_id_t channel, adc_channel_config_t& config) {
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
	// Apply channel configs
	for (int i=0; i<_config.channelCount; ++i) {
		initChannel(i, _config.channels[i]);
	}

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
		case CS_TYPE::EVT_STORAGE_WRITE: {
			//		stop();
			break;
		}
		case CS_TYPE::EVT_STORAGE_WRITE_DONE: {
			//		start();
			break;
		}
		default: {}
	}
}


void ADC::_restart() {
#ifdef PRINT_DEBUG
	LOGi("restart");
#endif
	stop();
	start();
}

void ADC::_handleTimeout() {
#ifdef PRINT_DEBUG
	LOGw("timeout");
#endif
	stop();
	start();
}

void ADC::_handleAdcDone(cs_adc_buffer_id_t bufIndex) {
#ifdef TEST_PIN_PROCESS
	nrf_gpio_pin_toggle(TEST_PIN_PROCESS);
#endif


	if (dataCallbackRegistered()) {
		_inProgress[bufIndex] = true;

		if (_firstBuffer) {
#ifdef PRINT_DEBUG
			LOGd("ADC restarted");
#endif
			event_t event(CS_TYPE::EVT_ADC_RESTARTED, NULL, 0);
			EventDispatcher::getInstance().dispatch(event);
		}
		_firstBuffer = false;

		if (_changeConfig) {
			stop();
			applyConfig();
			start();
		}

#ifdef PRINT_DEBUG_VERBOSE
		LOGd("process buf %u", bufIndex);
#endif
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

		cs_adc_buffer_id_t bufIndex = _bufferIndex;
		__attribute__((unused)) uint16_t bufSize = nrf_saadc_amount_get();


#ifdef PRINT_DEBUG_VERBOSE
		LOGd("Done %u q=%u ind=%u amount=%u", bufIndex, _numBuffersQueued, _bufferIndex, bufSize);
#endif
		if (_saadcState != ADC_SAADC_STATE_BUSY) {
			// Don't handle end events when state is not busy.
			// The end event also fires when we stop the saadc.
			return;
		}

		--_numBuffersQueued;
		if (_queuedBufferIndex == BUFFER_INDEX_NONE) {
			_saadcState = ADC_SAADC_STATE_STOPPING;
#ifdef PRINT_DEBUG_VERBOSE
			LOGw("No buffer queued");
#endif
			_bufferIndex = bufIndex;
//			_queuedBufferIndex = (bufIndex + 1) % CS_ADC_NUM_BUFFERS;
			uint32_t errorCode = app_sched_event_put(NULL, 0, adc_restart);
			APP_ERROR_CHECK(errorCode);
		}
		else {
			// Start next buffer
			// - set buffer: this is already done in addBufferToSample()
			// - call START task: this is already done via PPI

			// TODO: Maybe put outside the else? We can maybe still process this buffer, no? But it does delay the restart?
			_bufferIndex = _queuedBufferIndex;
			_queuedBufferIndex = BUFFER_INDEX_NONE;
			// Decouple handling of buffer from adc interrupt handler, copy buffer index.
			uint32_t errorCode = app_sched_event_put(&bufIndex, sizeof(bufIndex), adc_done);
			APP_ERROR_CHECK(errorCode);
		}
	}
	if (nrf_saadc_event_check(NRF_SAADC_EVENT_STOPPED)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_STOPPED);
#ifdef PRINT_DEBUG_VERBOSE
		LOGi("stopped");
#endif
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

nrf_saadc_event_t ADC::getLimitLowEvent(cs_adc_channel_id_t channel) {
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

nrf_saadc_event_t ADC::getLimitHighEvent(cs_adc_channel_id_t channel) {
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
