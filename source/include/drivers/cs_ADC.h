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
#include <structs/buffer/cs_AdcBuffer.h>
#include <structs/buffer/cs_CircularBuffer.h>

enum adc_gain_t {
	CS_ADC_GAIN4   = NRF_SAADC_GAIN4,    // gain is 4/1, maps [0, 0.15] to [0, 0.6]
	CS_ADC_GAIN2   = NRF_SAADC_GAIN2,    // gain is 2/1, maps [0, 0.3]  to [0, 0.6]
	CS_ADC_GAIN1   = NRF_SAADC_GAIN1,    // gain is 1/1, maps [0, 0.6]  to [0, 0.6]
	CS_ADC_GAIN1_2 = NRF_SAADC_GAIN1_2,  // gain is 1/2, maps [0, 1.2]  to [0, 0.6]
	CS_ADC_GAIN1_3 = NRF_SAADC_GAIN1_3,  // gain is 1/3, maps [0, 1.8]  to [0, 0.6]
	CS_ADC_GAIN1_4 = NRF_SAADC_GAIN1_4,  // gain is 1/4, maps [0, 2.4]  to [0, 0.6]
	CS_ADC_GAIN1_5 = NRF_SAADC_GAIN1_5,  // gain is 1/5, maps [0, 3.0]  to [0, 0.6]
	CS_ADC_GAIN1_6 = NRF_SAADC_GAIN1_6,  // gain is 1/6, maps [0, 3.6]  to [0, 0.6]
};

/**
 * The typedef adc_done_cb_t is a function pointer to a function with the buffer index as argument.
 * This function pointer can be set via ADC::setDoneCallback.
 * Currently, ADC::setDoneCallback is called from cs_PowerSampling.cpp.
 */
typedef void (*adc_done_cb_t)(adc_buffer_id_t bufIndex);

typedef void (*adc_zero_crossing_cb_t)();

// State of this class
enum adc_state_t {
	ADC_STATE_IDLE,
	ADC_STATE_BUSY,
	ADC_STATE_WAITING_TO_START,  // Wait for buffers to be released.
	ADC_STATE_READY_TO_START  // This state is a dummy, to work around the state check in start(). It is set just before
							  // calling start() again.
};

// State of the SAADC
enum adc_saadc_state_t {
	ADC_SAADC_STATE_IDLE,     // When saadc is idle.
	ADC_SAADC_STATE_BUSY,     // When saadc is busy sampling.
	ADC_SAADC_STATE_STOPPING  // When saadc is or will be commanded to stop.
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

	/** Set the callback which is called when a buffer is filled.
	 *
	 * @param[in] callback             Function to be called when a buffer is filled with samples.
	 */
	void setDoneCallback(adc_done_cb_t callback);

	/** Set the callback which is called on a zero crossing interrupt.
	 *
	 * Currently only called when going from below to above the zero.
	 *
	 * @param[in] callback             Function to be called on a zero crossing event. This function will run at
	 * interrupt level!
	 */
	void setZeroCrossingCallback(adc_zero_crossing_cb_t callback);

	/** Enable zero crossing detection on given channel, generating interrupts.
	 *
	 * @param[in] channel              The channel on which to check for zero crossings.
	 * @param[in] zeroVal              The value of zero.
	 */
	void enableZeroCrossingInterrupt(adc_channel_id_t channel, int32_t zeroVal);

	/**
	 * Change channel config.
	 *
	 * Channel config will not change immediately.
	 *
	 * @param[in] channel              Channel number.
	 * @param[in] config               The channel config.
	 * @return                         Return code.
	 */
	cs_ret_code_t changeChannel(adc_channel_id_t channel, adc_channel_config_t& config);

	// Handle events as EventListener.
	void handleEvent(event_t& event);

	/** Restart
	 */
	void _restart();

	/** Handle buffer, called in main thread.
	 */
	void _handleAdcDone(adc_buffer_id_t bufIndex);

	/** Called when the sampled value is above upper limit, or below lower limit.
	 *
	 */
	void _handleAdcLimitInterrupt(nrf_saadc_limit_t type);

	/**
	 * Handles the ADC interrupt.
	 *
	 * End:
	 * - Restarts SAADC when no buffers are queued in SAADC,
	 * - Fills up SAADC buffer.
	 */
	void _handleAdcInterrupt();

private:
	/** Constructor
	 */
	ADC();

	// This class is singleton, deny implementation
	ADC(ADC const&);

	// This class is singleton, deny implementation
	void operator      =(ADC const&);

	/**
	 * Whether or not the config should be changed.
	 *
	 * == Used in interrupt! ==
	 */
	bool _changeConfig = false;

	/**
	 * Configuration of this class.
	 *
	 * == Used in interrupt! ==
	 */
	adc_config_t _config;

	/**
	 * Resulting configuration of this class.
	 *
	 * == Used in interrupt! ==
	 */
	adc_channel_config_result_t _channelResultConfigs[CS_ADC_NUM_CHANNELS];

	/**
	 * Buffer sequence number.
	 *
	 * Increased each time a buffer is fully sampled.
	 */
	adc_buffer_seq_nr_t _bufSeqNr = 1;

	/**
	 * PPI channel to sample each tick, and count these.
	 *
	 * Sample timer event COMPARE -> SAADC task SAMPLE
	 *                            -> Toggle sample test pin
	 */
	nrf_ppi_channel_t _ppiChannelSample;

	/**
	 * PPI channel used to start the SAADC on end event.
	 *
	 * SAADC event END -> SAADC task START
	 */
	nrf_ppi_channel_t _ppiChannelStart;

	/**
	 * Queue of buffers that are free to be added to the SAADC queue.
	 *
	 * == Used in interrupt! ==
	 */
	CircularBuffer<adc_buffer_id_t> _bufferQueue;

	/**
	 * Keeps up which buffers that are queued in the SAADC peripheral.
	 *
	 * First buffer in queue is the one currently being (or going to be) filled with samples.
	 *
	 * == Used in interrupt! ==
	 */
	CircularBuffer<adc_buffer_id_t> _saadcBufferQueue;

	// True when next buffer is the first after start.
	bool _firstBuffer                      = true;

	// State of this class.
	adc_state_t _state                     = ADC_STATE_IDLE;

	/**
	 * Sate of the SAADC peripheral.
	 *
	 * == Used in interrupt! ==
	 */
	volatile adc_saadc_state_t _saadcState = ADC_SAADC_STATE_IDLE;

	// Callback function
	adc_done_cb_t _doneCallback            = nullptr;

	inline bool dataCallbackRegistered() { return (_doneCallback != nullptr); }

	/**
	 * The zero crossing callback.
	 *
	 * == Used in interrupt! ==
	 */
	adc_zero_crossing_cb_t _zeroCrossingCallback = nullptr;

	/**
	 * The channel which is checked for zero crossings.
	 *
	 * == Used in interrupt! ==
	 */
	adc_channel_id_t _zeroCrossingChannel        = 0;

	/**
	 * Cache limit event.
	 *
	 * == Used in interrupt! ==
	 */
	nrf_saadc_event_t _eventLimitLow;

	/**
	 * Cache limit event.
	 *
	 * == Used in interrupt! ==
	 */
	nrf_saadc_event_t _eventLimitHigh;

	/**
	 * Keep up whether zero crossing is enabled.
	 *
	 * == Used in interrupt! ==
	 */
	bool _zeroCrossingEnabled     = false;

	/**
	 * Store the RTC timestamp of the last upwards zero crossing.
	 *
	 * == Used in interrupt! ==
	 */
	uint32_t _lastZeroCrossUpTime = 0;

	/**
	 * Store the zero value used to detect zero crossings.
	 *
	 * == Used in interrupt! ==
	 */
	int32_t _zeroValue            = 0;

	/**
	 * Keep up number of nested critical regions entered.
	 */
	int _criticalRegionEntered    = 0;

	cs_ret_code_t initSaadc();

	/**
	 * Configure a channel.
	 *
	 * @param[in] channel              Channel number.
	 * @param[in] config               The channel config.
	 * @return                         Return code.
	 */
	cs_ret_code_t initChannel(adc_channel_id_t channel, adc_channel_config_t& config);

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
	 *
	 * @param[in] fromInterrupt        Whether this is called from an interrupt.
	 *
	 * == Called from interrupt! ==
	 */
	cs_ret_code_t _fillSaadcQueue(bool fromInterrupt);

	/**
	 * Puts a buffer in the SAADC queue.
	 *
	 * Starts the SAADC when it's idle.
	 * Waits (blocking) for the SAADC to be started to queue another buffer.
	 *
	 * @return
	 * - ERR_SUCCESS when the buffer is queued in the SAADC.
	 * - ERR_NO_SPACE when there are already enough buffers queued in the SAADC, so the given buffer is not queued.
	 * - Other return codes for failures, the given buffer is not queued.
	 */
	cs_ret_code_t addBufferToSaadcQueue(adc_buffer_id_t bufIndex);

	/**
	 * Same as addBufferToSaadcQueue(), but without critical region enter/exit.
	 *
	 * == Called from interrupt! ==
	 */
	cs_ret_code_t _addBufferToSaadcQueue(adc_buffer_id_t bufIndex);

	/**
	 * Enter a region of code where variables are used that are also used in SAADC interrupts.
	 * Make sure you exit once for each time you call enter.
	 */
	void enterCriticalRegion();

	/**
	 * Exit a region of code where variables are used that are also used in SAADC interrupts.
	 */
	void exitCriticalRegion();

	void printQueues();

	// Function to apply a new config. Should only be called when SAADC has stopped.
	void applyConfig();

	// Helper function that returns the adc pin number, given the AIN number.
	static nrf_saadc_input_t getAdcPin(adc_pin_id_t pinNum);

	// Helper function to get the ppi channel, given the index.
	static nrf_ppi_channel_t getPpiChannel(uint8_t index);

	// Helper function to get the limit event, given the channel.
	static nrf_saadc_event_t getLimitLowEvent(adc_channel_id_t channel);

	// Helper function to get the limit event, given the channel.
	static nrf_saadc_event_t getLimitHighEvent(adc_channel_id_t channel);

	// Helper function to get the gpiote task out, given the index.
	static nrf_gpiote_tasks_t getGpioteTaskOut(uint8_t index);
};
