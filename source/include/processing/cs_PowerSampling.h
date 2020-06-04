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
#include <structs/buffer/cs_CircularBuffer.h>
#include <third/Median.h>

typedef void (*ps_zero_crossing_cb_t) ();

typedef uint8_t channel_id_t;

class PowerSampling : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static PowerSampling& getInstance() {
		static PowerSampling instance;
		return instance;
	}

	void init(const boards_config_t& boardConfig);

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
	void powerSampleAdcDone(buffer_id_t bufIndex);

	/** Fill up the current curve and send it out over bluetooth
	 * @type specifies over which characteristic the current curve should be sent.
	 */
	void sampleCurrentDone(uint8_t type);

	/** Enable zero crossing detection on given channel, generating interrupts.
	 *
	 * @param[in] callback             Function to be called on a zero crossing event. This function will run at interrupt level!
	 */
	void enableZeroCrossingInterrupt(ps_zero_crossing_cb_t callback);

	/** handle (crownstone) events
	 */
	void handleEvent(event_t & event);

	/**
	 * Struct that defines the buffer received from the ADC sampler in scanning mode.
	 */
	typedef struct {
		nrf_saadc_value_t* buf;
		uint16_t bufSize;
		uint16_t numChannels;
		uint16_t voltageIndex;
		uint16_t currentIndex;
		uint32_t sampleIntervalUs;
		uint32_t acPeriodUs;
	} power_t;


private:
	PowerSampling();

	//! Variable to keep up whether power sampling is initialized.
	bool _isInitialized;

	//! Reference to the ADC instance
	ADC* _adc;

	//! Operation mode of this device.
	OperationMode _operationMode;

	TYPIFY(CONFIG_VOLTAGE_MULTIPLIER) _voltageMultiplier; //! Voltage multiplier from settings.
	TYPIFY(CONFIG_CURRENT_MULTIPLIER) _currentMultiplier; //! Current multiplier from settings.
	TYPIFY(CONFIG_VOLTAGE_ADC_ZERO) _voltageZero; //! Voltage zero from settings.
	TYPIFY(CONFIG_CURRENT_ADC_ZERO) _currentZero; //! Current zero from settings.
	TYPIFY(CONFIG_POWER_ZERO) _powerZero; //! Power zero from settings.

	uint16_t _avgZeroCurrentDiscount;
	uint16_t _avgZeroVoltageDiscount;
	uint16_t _avgPowerDiscount;

	int32_t _boardPowerZero; //! Measured power when there is no load for this board (mW).
	int32_t _avgZeroVoltage; //! Used for storing and calculating the average zero voltage value (times 1000).
	int32_t _avgZeroCurrent; //! Used for storing and calculating the average zero current value (times 1000).
	bool _recalibrateZeroVoltage; //! Whether or not the zero voltage value should be recalculated.
	bool _recalibrateZeroCurrent; //! Whether or not the zero current value should be recalculated.
//	bool _zeroVoltageInitialized; //! True when zero of voltage has been initialized.
//	bool _zeroCurrentInitialized; //! True when zero of current has been initialized.
	uint16_t _zeroVoltageCount; //! Number of times the zero voltage has been calculated.
	uint16_t _zeroCurrentCount; //! Number of times the zero current has been calculated.
	double _avgPower; //! Used for storing and calculating the average power (in mW).
	int32_t _avgPowerMilliWatt; //! Used to send out the average power (in mW).
	int32_t _avgCurrentRmsMilliAmp; //! Used for storing the average rms current (in mA).
	int32_t _avgVoltageRmsMilliVolt; //! Used for storing the average rms voltage (in mV).

	PowerVector* _inputSamples;  //! Used for storing the samples to be filtered.
	PowerVector* _outputSamples; //! Used for storing the filtered samples.
	MedianFilter* _filterParams;  //! Stores the parameters for the moving median filter.

	CircularBuffer<int32_t>* _powerMilliWattHist;      //! Used to store a history of the power
	CircularBuffer<int32_t>* _currentRmsMilliAmpHist;  //! Used to store a history of the current_rms
	CircularBuffer<int32_t>* _filteredCurrentRmsHistMA; //! Used to store a history of the filtered current_rms
	CircularBuffer<int32_t>* _voltageRmsMilliVoltHist; //! Used to store a history of the voltage_rms
	int32_t _histCopy[POWER_SAMPLING_RMS_WINDOW_SIZE];     //! Used to copy a history to (so it can be used to calculate the median)
	uint16_t _consecutivePwmOvercurrent;


	TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD) _currentMilliAmpThreshold;    //! Current threshold from settings.
	TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM) _currentMilliAmpThresholdPwm; //! Current threshold when using dimmer from settings.

	uint32_t _lastEnergyCalculationTicks; //! Ticks of RTC when last energy calculation was performed.
	int64_t _energyUsedmicroJoule; //! Energy used in micro joule

	uint8_t _skipSwapDetection = 1; //! Number of buffers to skip until we start detecting swaps.

	switch_state_t _lastSwitchState; //! Stores the last seen switch state.
	uint32_t _lastSwitchOffTicks;    //! RTC ticks when the switch was last turned off.
	bool _lastSwitchOffTicksValid;   //! Keep up whether the last switch off time is valid.
	bool _igbtFailureDetectionStarted; //! Keep up whether the IGBT failure detection has started yet.
	uint32_t _calibratePowerZeroCountDown = 4000 / TICK_INTERVAL_MS;

	//! Store the adc config, so that the actual adc config can be changed.
	struct __attribute__((packed)) {
		uint16_t rangeMilliVolt[2];       //! For both channels
		uint8_t currentPinGainHigh;       //! Stores the current pin
		uint8_t currentPinGainMed;        //! Stores the current pin with medium gain
		uint8_t currentPinGainLow;        //! Stores the current pin with lowest gain
		uint8_t voltagePin;               //! Stores the voltage pin
		uint8_t zeroReferencePin;         //! Stores the zero reference pin
		uint8_t voltageChannelPin;        //! Stores which pin is currently set on the voltage channel.
		uint8_t voltageChannelUsedAs : 4; //! 0 for voltage, 1 for reference, 2 for VDD, 3 for current1, 4 for current2.
		bool currentDifferential     : 1; //! True when differential mode is used for current channel (if possible).
		bool voltageDifferential     : 1; //! True when differential mode is used for voltage channel (if possible).
	} _adcConfig;

	union {
		struct __attribute__((packed)) {
			bool power : 1;
			bool current : 1;
			bool voltage : 1;
			bool filteredCurrent : 1;
		} flags;
		uint32_t asInt;
	} _logsEnabled;

	cs_adc_restarts_t _adcRestarts;

	/** Initialize the moving averages
	 */
	void initAverages();

	/** Determine which index is actually the current index, this should not be necessary!
	 */
	uint16_t determineCurrentIndex(power_t & power);

	/** Calculate the value of the zero line of the voltage samples
	 */
	void calculateVoltageZero(power_t & power);

	/** Calculate the value of the zero line of the current samples
	 */
	void calculateCurrentZero(power_t & power);

	/** Filter the samples
	 */
	void filter(buffer_id_t buffer_id, channel_id_t channel_id);

	/**
	 * Checks if voltage and current index are swapped.
	 *
	 * Checks if previous voltage samples look more like this buffer voltage samples or current samples.
	 * Assumes previous buffer is valid, and of same size as this buffer.
	 */
	bool isVoltageAndCurrentSwapped(power_t & power, nrf_saadc_value_t* prevBuf);

	/** Calculate the average power usage
	 */
	void calculatePower(power_t & power);

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

	/** If current goes beyond predefined threshold levels, take action!
	 */
	void checkSoftfuse(int32_t currentRmsMilliAmp, int32_t currentRmsMilliAmpFiltered, int32_t voltageRmsMilliVolt, power_t & power);

	/** Start IGBT failure detection
	 */
	void startIgbtFailureDetection();

	void toggleVoltageChannelInput();

	void enableDifferentialModeCurrent(bool enable);

	void enableDifferentialModeVoltage(bool enable);

	void changeRange(uint8_t channel, int32_t amount);

	void enableSwitchcraft(bool enable);

	void printBuf(power_t & power);
};

