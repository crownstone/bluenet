/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 6 Nov., 2014
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <cfg/cs_Config.h>
#include <events/cs_EventListener.h>
#include <structs/buffer/cs_CircularBuffer.h>
#include <structs/buffer/cs_InterleavedBuffer.h>

// Numeric reference to a pin
typedef uint8_t cs_adc_pin_id_t;


enum adc_gain_t {
	CS_ADC_GAIN4   = NRF_SAADC_GAIN4,   // gain is 4/1, maps [0, 0.15] to [0, 0.6]
	CS_ADC_GAIN2   = NRF_SAADC_GAIN2,   // gain is 2/1, maps [0, 0.3]  to [0, 0.6]
	CS_ADC_GAIN1   = NRF_SAADC_GAIN1,   // gain is 1/1, maps [0, 0.6]  to [0, 0.6]
	CS_ADC_GAIN1_2 = NRF_SAADC_GAIN1_2, // gain is 1/2, maps [0, 1.2]  to [0, 0.6]
	CS_ADC_GAIN1_3 = NRF_SAADC_GAIN1_3, // gain is 1/3, maps [0, 1.8]  to [0, 0.6]
	CS_ADC_GAIN1_4 = NRF_SAADC_GAIN1_4, // gain is 1/4, maps [0, 2.4]  to [0, 0.6]
	CS_ADC_GAIN1_5 = NRF_SAADC_GAIN1_5, // gain is 1/5, maps [0, 3.0]  to [0, 0.6]
	CS_ADC_GAIN1_6 = NRF_SAADC_GAIN1_6, // gain is 1/6, maps [0, 3.6]  to [0, 0.6]
};

#define CS_ADC_REF_PIN_NOT_AVAILABLE 255
#define CS_ADC_PIN_VDD 100

/** Struct to configure an ADC channel.
 *
 * pin:                 The AIN pin to sample.
 * rangeMilliVolt:      The range in mV of the pin.
 * referencePin:        The AIN pin to be subtracted from the measured voltage.
 */
struct __attribute__((packed)) adc_channel_config_t {
	cs_adc_pin_id_t pin;
	uint32_t rangeMilliVolt;
	cs_adc_pin_id_t referencePin;
};

/** Struct to configure the ADC.
 *
 * channelCount:        The amount of channels to sample.
 * channels:            The channel configs.
 * samplingPeriodUs:    The sampling period in μs (each period, all channels are sampled once).
 */
struct __attribute__((packed)) adc_config_t {
	channel_id_t channelCount;
	adc_channel_config_t channels[CS_ADC_MAX_PINS];
	uint32_t samplingPeriodUs;
};


/**
 * The typedef adc_done_cb_t is a function pointer to a function with the buffer index as argument.
 * This function pointer can be set via ADC::setDoneCallback.
 * Currently, ADC::setDoneCallback is called from cs_PowerSampling.cpp.
 */
typedef void (*adc_done_cb_t) (buffer_id_t bufIndex);

typedef void (*adc_zero_crossing_cb_t) ();

// State of this class
enum adc_state_t {
	ADC_STATE_IDLE,
	ADC_STATE_BUSY,
	ADC_STATE_WAITING_TO_START, // Wait for buffers to be released.
	ADC_STATE_READY_TO_START    // This state is a dummy, to work around the state check in start(). It is set just before calling start() again.
};

// State of the SAADC
enum adc_saadc_state_t {
	ADC_SAADC_STATE_IDLE,    // When saadc is idle.
	ADC_SAADC_STATE_BUSY,    // When saadc is busy sampling.
	ADC_SAADC_STATE_STOPPING // When saadc is or will be commanded to stop.
};

// Max number of buffers in the SAADC peripheral.
#define CS_ADC_NUM_SAADC_BUFFERS 2


/** Analog-Digital conversion.
 *
 * The ADC class can be used to convert an analog signal to a digital value on multiple pins.
 * It's currently used for power measurement (measure voltage and current).
 *
 * There is only one SAADC hardware peripheral, hence this class conforms to the singleton design pattern.
 *
 * The constructor does NOT have arguments you might expect, such as the number of buffers to use, or the size of the
 * buffers. These are all set through preprocessor macros. The ADC class requires access to the following macros
 * (see below).
 *
 * The timer used to get new samples on regular time intervals. The timer should be able to run on at least 2kHz to
 * be able to get the fine-grained current and voltage data we want.
 *
 *   - CS_ADC_TIMER                    The timer peripheral to use for the ADC peripheral.
 *   - CS_ADC_INSTANCE_INDEX           The instance id of the timer, used by nrf_drv_config.
 *   - CS_ADC_TIMER_ID                 Number of the timer.
 *
 * The buffers to be used internally. To have these buffers in the form of macros means that we can check at compile
 * time if they are small enough with respect to the nRF52 memory limitations.
 *
 *   - CS_ADC_NUM_BUFFERS              The number of ADC buffers to use, should be at least 2.
 *   - CS_ADC_BUF_SIZE                 The size of the buffer (first time used in init()).
 *
 * The pins:
 *
 *   - CS_ADC_MAX_PINS                 The maximum number of pins available for ADC.
 *
 * The sequence of events can be found in the uml diagrams under docs.
 */
class ADC : EventListener {

public:
	//! Use static variant of singleton, no dynamic memory allocation
	static ADC& getInstance() {
		static ADC instance;
		return instance;
	}

	/** Initialize ADC.
	 *
	 * @param[in] config               Config struct.
	 * @return                         Return code.
	 */
	cs_ret_code_t init(const adc_config_t& config);

	/** Start the ADC sampling
	 */
	void start();

	/** Stop the ADC sampling
	 *  Only to be called from main thread.
	 */
	void stop();

	/** Release a buffer, so it can be used again by the ADC.
	 *
	 * @param[in] buf                  Pointer to the buffer, as received in the done callback.
	 */
	void releaseBuffer(buffer_id_t bufIndex);

	/** Set the callback which is called when a buffer is filled.
	 *
	 * @param[in] callback             Function to be called when a buffer is filled with samples.
	 */
	void setDoneCallback(adc_done_cb_t callback);

	/** Set the callback which is called on a zero crossing interrupt.
	 *
	 * Currently only called when going from below to above the zero.
	 *
	 * @param[in] callback             Function to be called on a zero crossing event. This function will run at interrupt level!
	 */
	void setZeroCrossingCallback(adc_zero_crossing_cb_t callback);

	/** Enable zero crossing detection on given channel, generating interrupts.
	 *
	 * @param[in] channel              The channel on which to check for zero crossings.
	 * @param[in] zeroVal              The value of zero.
	 */
	void enableZeroCrossingInterrupt(channel_id_t channel, int32_t zeroVal);

	/** Change channel config.
	 *  Currently this config is applied immediately.
	 *  TODO: Apply config after a buffer has been filled.
	 *
	 * @param[in] channel              Channel number.
	 * @param[in] config               Config struct.
	 * @return                         Return code.
	 */
	cs_ret_code_t changeChannel(channel_id_t channel, adc_channel_config_t& config);

	// Handle events as EventListener.
	void handleEvent(event_t & event);

	/** Restart
	 */
	void _restart();

	/** Handle buffer, called in main thread.
	 */
	void _handleAdcDone(buffer_id_t bufIndex);

	/** Handles timeout
	 */
	void _handleTimeout();

	/** Called when the sampled value is above upper limit, or below lower limit.
	 *
	 */
	void _handleAdcLimitInterrupt(nrf_saadc_limit_t type);

	/** Handles the ADC interrupt.
	 */
	void _handleAdcInterrupt();

	/** Called when the timeout timer triggered.
	 */
	void _handleTimeoutInterrupt();

private:
	/** Constructor
	 */
	ADC();

	// This class is singleton, deny implementation
	ADC(ADC const&);

	// This class is singleton, deny implementation
	void operator=(ADC const &);

	// Whether or not the config should be changed.
	bool _changeConfig;

	// Configuration of this class
	// **Used in interrupt!**
	adc_config_t _config;

	/**
	 * PPI channel to sample each tick, and count these.
	 *
	 * Sample timer event COMPARE -> SAADC task SAMPLE
	 *                            -> Timeout counter task COUNT
	 */
	nrf_ppi_channel_t _ppiChannelSample;

	/**
	 * PPI channel used to start the SAADC on end event.
	 *
	 * SAADC event END -> SAADC task START
	 */
	nrf_ppi_channel_t _ppiChannelStart;

	/**
	 * PPI channel used to clear and start the timeout counter.
	 *
	 * SAADC event END -> Timeout counter task CLEAR.
	 *                 -> Timeout counter task START.
	 */
	nrf_ppi_channel_t _ppiTimeoutStart;

	/**
	 * PPI channel used to stop on timeout (when no buffer is queued in time).
	 *
	 * Timeout counter event COMPARE -> Sample timer task STOP
	 */
	nrf_ppi_channel_t _ppiTimeout;

	nrf_ppi_channel_t _ppiDebug;

	nrf_ppi_channel_group_t _ppiTimeoutGroup;

	/**
	 * Queue of buffers that are free to be added to the SAADC queue.
	 *
	 * Used in interrupt!
	 */
	CircularBuffer<buffer_id_t> _bufferQueue;

	/**
	 * Keeps up which buffers that are queued in the SAADC peripheral.
	 *
	 * First buffer in queue is the one currently being (or going to be) filled with samples.
	 *
	 * Used in interrupt!
	 */
	CircularBuffer<buffer_id_t> _saadcBufferQueue;

	// True when next buffer is the first after start.
	bool _firstBuffer;

	// State of this class.
	adc_state_t _state;

	/**
	 * Sate of the SAADC peripheral.
	 *
	 * Used in interrupt!
	 */
	volatile adc_saadc_state_t _saadcState;

	/**
	 * Keep up which buffers are being processed by callback.
	 * This only accounts for the time between calling the doneCallback and releaseBuffer.
	 * Really only used for testing.
	 *
	 * Used in interrupt!
	 */
	bool _inProgress[CS_ADC_NUM_BUFFERS] = { false };

	// Callback function
	adc_done_cb_t _doneCallback;

	inline bool dataCallbackRegistered() {
//		return (_doneCallbackData.callback != NULL);
		return (_doneCallback != NULL);
	}

	// The zero crossing callback.
	// **Used in interrupt!**
	adc_zero_crossing_cb_t _zeroCrossingCallback;

	// The channel which is checked for zero crossings.
	// **Used in interrupt!**
	channel_id_t _zeroCrossingChannel;

	// Cache limit event.
	// **Used in interrupt!**
	nrf_saadc_event_t _eventLimitLow;

	// Cache limit event.
	// **Used in interrupt!**
	nrf_saadc_event_t _eventLimitHigh;

	// Keep up whether zero crossing is enabled.
	// **Used in interrupt!**
	bool _zeroCrossingEnabled;

	// Store the timestamp of the last upwards zero crossing.
	// **Used in interrupt!**
	uint32_t _lastZeroCrossUpTime;

	// Store the zero value used to detect zero crossings.
	// **Used in interrupt!**
	int32_t _zeroValue;

	/**
	 * Keep up number of nested critical regions entered.
	 */
	int _criticalRegionEntered = 0;

	cs_ret_code_t initSaadc(const adc_config_t & config);

	// Function to initialize the adc channels.
	cs_ret_code_t initChannel(channel_id_t channel, adc_channel_config_t& config);

	// Set the adc limit such that it triggers when going above zero
	void setLimitUp();

	// Set the adc limit such that it triggers when going below zero
	void setLimitDown();

	// Initialize buffer queue
	cs_ret_code_t initBufferQueue();

	/**
	 * Try to add all queued buffers to the SAADC queue.
	 * If the SAADC queue is now filled, stop the timeout timer.
	 *
	 * @return ERR_SUCCESS when SAADC queue is full,           and timeout timer has been stopped.
	 * @return ERR_WRONG_STATE when SAADC queue is not filled, and timeout timer has been stopped.
	 * @return ERR_NOT_FOUND when the SAADC queue is not full, and timeout timer has not been stopped.
	 * @return Other return codes on failure.
	 */
	cs_ret_code_t fillSaadcQueue();

	/**
	 * Same as fillSaadcQueue(), but without critical region enter/exit.
	 */
	cs_ret_code_t _fillSaadcQueue();

	/**
	 * Puts a buffer in the SAADC queue.
	 *
	 * Starts the SAADC when it's idle.
	 * Waits for the SAADC to be started to queue another buffer.
	 *
	 * @return
	 * - ERR_SUCCESS when the buffer is queued in the SAADC.
	 * - ERR_NO_SPACE when there are already enough buffers queued in the SAADC, so the given buffer is not queued.
	 * - Other return codes for failures, the given buffer is not queued.
	 */
	cs_ret_code_t addBufferToSaadcQueue(buffer_id_t bufIndex);

	/**
	 * Same as addBufferToSaadcQueue(), but without critical region enter/exit.
	 */
	cs_ret_code_t _addBufferToSaadcQueue(buffer_id_t bufIndex);

	/**
	 * Enter a region of code where variables are used that are also used in SAADC interrupts.
	 * Make sure you exit once for each time you call enter.
	 */
	void enterCriticalRegion();

	/**
	 * Exit a region of code where variables are used that are also used in SAADC interrupts.
	 */
	void exitCriticalRegion();

	void enableInterrupt();

	void printQueues();

	// Function to apply a new config. Should be called when no buffers are are queued, nor being processed.
	void applyConfig();

	// Function to stop the timeout timer.
	void stopTimeout();

	// Helper function that returns the adc pin number, given the AIN number.
	nrf_saadc_input_t getAdcPin(cs_adc_pin_id_t pinNum);

	// Helper function to get the ppi channel, given the index.
	nrf_ppi_channel_t getPpiChannel(uint8_t index);

	// Helper function to get the limit event, given the channel.
	nrf_saadc_event_t getLimitLowEvent(channel_id_t channel);

	// Helper function to get the limit event, given the channel.
	nrf_saadc_event_t getLimitHighEvent(channel_id_t channel);

	// Helper function to get the gpiote task out, given the index.
	nrf_gpiote_tasks_t getGpioteTaskOut(uint8_t index);
};
