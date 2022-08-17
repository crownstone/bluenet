/**
 * Authors: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 19, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Boards.h>
#include <drivers/cs_ADC.h>
#include <events/cs_EventListener.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_AdcBuffer.h>
#include <structs/buffer/cs_CircularBuffer.h>
#include <third/Median.h>

#include <cstdint>

typedef void (*ps_zero_crossing_cb_t)();

class PowerSampling : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static PowerSampling& getInstance() {
		static PowerSampling instance;
		return instance;
	}

	/**
	 * Init the class.
	 *
	 * The board config must remain in memory.
	 */
	void init(const boards_config_t* boardConfig);

	/** Initializes and starts the ADC, also starts interval timer.
	 */
	void powerSampleFirstStart();

	/** Starts a new power sample burst.
	 *  Called at a low interval.
	 */
	void startSampling();

	/** Called when the sample burst is finished.
	 *  Calculates the power usage, updates the state.
	 *  Sends the samples if the central is subscribed for that.
	 */
	void powerSampleAdcDone(adc_buffer_id_t bufIndex);

	/** Fill up the current curve and send it out over bluetooth
	 * @type specifies over which characteristic the current curve should be sent.
	 */
	void sampleCurrentDone(uint8_t type);

	/** Enable zero crossing detection on given channel, generating interrupts.
	 *
	 * @param[in] callback             Function to be called on a zero crossing event. This function will run at
	 * interrupt level!
	 */
	void enableZeroCrossingInterrupt(ps_zero_crossing_cb_t callback);

	/** handle (crownstone) events
	 */
	void handleEvent(event_t& event);

	/**
	 * Struct that defines the buffer received from the ADC sampler in scanning mode.
	 */
	typedef struct {
		adc_sample_value_t* buf;
		uint16_t bufSize;
		uint16_t numChannels;
		uint16_t voltageIndex;
		uint16_t currentIndex;
		uint32_t sampleIntervalUs;
		uint32_t acPeriodUs;
	} power_t;

private:
	PowerSampling();

	// Number of filtered buffers for processing.
	// Should be at least as large as number of buffers required for recognize switch.
	const static uint8_t numFilteredBuffersForProcessing = 4;

	// Currently hard coded at 1.
	const static uint8_t numUnfilteredBuffers            = 1;

	// Number of switch states to keep up.
	const static uint8_t switchHistSize                  = 3;

	//! Variable to keep up whether power sampling is initialized.
	bool _isInitialized                                  = false;

	//! Reference to the ADC instance
	ADC* _adc;

	//! Operation mode of this device.
	OperationMode _operationMode;

	const boards_config_t* _boardConfig = nullptr;

	/**
	 * Queue of buffers we can use for processing.
	 *
	 * If queue size == 1:
	 * - buffer[0] = last filtered.
	 * If queue size > 1:
	 * - buffer[size] = last unfiltered.
	 * - buffer[size-1] = last filtered.
	 * - buffer[size-2] = previous filtered.
	 */
	CircularBuffer<adc_buffer_id_t> _bufferQueue;

	cs_power_samples_header_t _lastSoftfuse;
	adc_sample_value_t _lastSoftfuseSamples[AdcBuffer::getChannelLength()] = {0};

	CircularBuffer<switch_state_t> _switchHist;
	cs_power_samples_header_t _lastSwitchSamplesHeader;

	const static uint8_t numSwitchSamplesBuffers = 6;  // 3 voltage and 3 current buffers.
	adc_sample_value_t _lastSwitchSamples[numSwitchSamplesBuffers * AdcBuffer::getChannelLength()] = {0};

	TYPIFY(CONFIG_VOLTAGE_MULTIPLIER) _voltageMultiplier;  //! Voltage multiplier from settings.
	TYPIFY(CONFIG_CURRENT_MULTIPLIER) _currentMultiplier;  //! Current multiplier from settings.
	TYPIFY(CONFIG_VOLTAGE_ADC_ZERO) _voltageZero;          //! Voltage zero from settings.
	TYPIFY(CONFIG_CURRENT_ADC_ZERO) _currentZero;          //! Current zero from settings.
	TYPIFY(CONFIG_POWER_ZERO) _powerZero;                  //! Power zero from settings.

	uint16_t _avgZeroCurrentDiscount;
	uint16_t _avgZeroVoltageDiscount;
	uint16_t _avgPowerDiscount;

	// Slow averaging of power
	float _slowAvgPowerDiscount;
	float _slowAvgPowerMilliWatt = 0.0f;
	uint16_t _slowAvgPowerCount;  // Number of values that have been used for slow averaging.
	const uint16_t slowAvgPowerConvergedCount = 1000;
	float _powerDiffThresholdPart;  // When difference is 10% larger or smaller, consider it a significant change.
	float _powerDiffThresholdMinMilliWatt;   // But the difference must also be at least so many Watts.
	float _negativePowerThresholdMilliWatt;  // Only if power is below threshold, it may be negative.

	int32_t _boardPowerZero;       //! Measured power when there is no load for this board (mW).
	int32_t _avgZeroVoltage;       //! Used for storing and calculating the average zero voltage value (times 1024).
	int32_t _avgZeroCurrent;       //! Used for storing and calculating the average zero current value (times 1024).
	bool _recalibrateZeroVoltage;  //! Whether or not the zero voltage value should be recalculated.
	bool _recalibrateZeroCurrent;  //! Whether or not the zero current value should be recalculated.

	uint16_t _zeroVoltageCount;  //! Number of times the zero voltage has been calculated.
	uint16_t _zeroCurrentCount;  //! Number of times the zero current has been calculated.

	int32_t _avgPowerMilliWatt;       //! Used to send out the average power (in mW).
	int32_t _avgCurrentRmsMilliAmp;   //! Used for storing the average rms current (in mA).
	int32_t _avgVoltageRmsMilliVolt;  //! Used for storing the average rms voltage (in mV).

	PowerVector* _inputSamples;   //! Used for storing the samples to be filtered.
	PowerVector* _outputSamples;  //! Used for storing the filtered samples.
	MedianFilter* _filterParams;  //! Stores the parameters for the moving median filter.

	CircularBuffer<int32_t>* _powerMilliWattHist;        //! Used to store a history of the power
	CircularBuffer<int32_t>* _currentRmsMilliAmpHist;    //! Used to store a history of the current_rms
	CircularBuffer<int32_t>* _filteredCurrentRmsHistMA;  //! Used to store a history of the filtered current_rms
	CircularBuffer<int32_t>* _voltageRmsMilliVoltHist;   //! Used to store a history of the voltage_rms
	int32_t _histCopy[POWER_SAMPLING_RMS_WINDOW_SIZE];   //! Used to copy a history to (so it can be used to calculate
														 //! the median)
	uint16_t _consecutiveDimmerOvercurrent = 0;
	uint16_t _consecutiveOvercurrent       = 0;

	TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD) _currentMilliAmpThreshold;  //! Current threshold from settings.
	TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_DIMMER)
	_currentMilliAmpThresholdDimmer;  //! Current threshold when using dimmer from settings.

	int64_t _energyUsedmicroJoule = 0;  //! Energy used in micro joule

	switch_state_t _lastSwitchState;                //! Stores the last seen switch state.
	uint32_t _lastSwitchOffTicks;                   //! RTC ticks when the switch was last turned off.
	bool _lastSwitchOffTicksValid         = false;  //! Keep up whether the last switch off time is valid.
	bool _dimmerFailureDetectionStarted   = false;  //! Keep up whether the IGBT failure detection has started yet.
	uint32_t _calibratePowerZeroCountDown = 4000 / TICK_INTERVAL_MS;

	// Store the adc config, so that the actual adc config can be changed.
	struct __attribute__((packed)) {
		adc_channel_config_t config;

		// Iterate over the different pins for this channel.
		uint8_t pinIndex = 0;

		// Number of pins we can iterate over.
		uint8_t pinCount = 0;
	} _adcConfig[2];

	union {
		struct __attribute__((packed)) {
			bool power : 1;
			bool current : 1;
			bool voltage : 1;
			bool filteredCurrent : 1;
		} flags;
		uint32_t asInt;
	} _logsEnabled;

	adc_buffer_seq_nr_t _lastBufSeqNr     = 0;
	adc_buffer_id_t _lastBufIndex         = 0;
	adc_buffer_id_t _lastFilteredBufIndex = 0;

	cs_adc_restarts_t _adcRestarts;
	cs_adc_channel_swaps_t _adcChannelSwaps;

	/** Initialize the moving averages
	 */
	void initAverages();

	/**
	 * Whether the given buffer is valid.
	 *
	 * This can change at any moment (set in interrupt).
	 */
	bool isValidBuf(adc_buffer_id_t bufIndex);

	/**
	 * Whether the given sequence nr follows directly after the previous sequence nr.
	 *
	 * This can change at any moment (set in interrupt).
	 */
	bool isConsecutiveBuf(adc_buffer_seq_nr_t seqNr, adc_buffer_seq_nr_t prevSeqNr);

	/**
	 * Remove all buffers from queue that are older than the newest invalid buffer.
	 *
	 * What remains is a queue of consecutive valid buffers.
	 */
	void removeInvalidBufs();

	/**
	 * Calculate the value of the zero line of the voltage samples (the offset).
	 */
	void calculateVoltageZero(adc_buffer_id_t bufIndex);

	/** Calculate the value of the zero line of the current samples
	 */
	void calculateCurrentZero(adc_buffer_id_t bufIndex);

	/** Filter the samples
	 */
	void filter(adc_buffer_id_t bufIndexIn, adc_buffer_id_t bufIndexOut, adc_channel_id_t channel_id);

	/**
	 * Checks if voltage and current index are swapped.
	 *
	 * Checks if previous voltage samples look more like this buffer voltage samples or current samples.
	 * Assumes previous buffer is valid, and of same size as this buffer.
	 */
	bool isVoltageAndCurrentSwapped(adc_buffer_id_t bufIndex, adc_buffer_id_t prevBufIndex);

	/**
	 * Calculate the average power usage
	 *
	 * @return true when calculation was successful.
	 */
	bool calculatePower(adc_buffer_id_t bufIndex);

	void calculateSlowAveragePower(float powerMilliWatt, float fastAvgPowerMilliWatt);

	/**
	 * Determines measured power usage with no load.
	 *
	 * When successful, sets the value in state.
	 *
	 * Careful: make sure this doesn't interfere with dimmer on failure detection.
	 */
	void calibratePowerZero(int32_t powerMilliWatt);

	/** Calculate the energy used
	 */
	void calculateEnergy();

	/**
	 * Check if the current goes above a threshold (for long enough).
	 *
	 * Emits an event when a softfuse triggers.
	 * Stores the current buffer of the last buffer that's above threshold, before the softfuse triggered.
	 *
	 * @param[in] currentRmsMilliAmp             RMS current in mA of the last AC period.
	 * @param[in] currentRmsMilliAmpFiltered     Filtered (averaged or so) RMS current in mA.
	 * @param[in] voltageRmsMilliVolt            RMS voltage in mV of the last AC period.
	 * @param[in] power                          Struct that holds the buffers.
	 */
	void checkSoftfuse(
			int32_t currentRmsMilliAmp,
			int32_t currentRmsMilliAmpFiltered,
			int32_t voltageRmsMilliVolt,
			adc_buffer_id_t bufIndex);

	void handleGetPowerSamples(PowerSamplesType type, uint8_t index, cs_result_t& result);

	void selectNextPin(adc_channel_id_t channel);

	void enableDifferentialModeCurrent(bool enable);

	void enableDifferentialModeVoltage(bool enable);

	void changeRange(uint8_t channel, int32_t amount);

	void applyAdcConfig(adc_channel_id_t channelIndex);

	void enableSwitchcraft(bool enable);

	void printBuf(adc_buffer_id_t bufIndex);
};
