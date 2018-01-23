/**
 * Authors: Crownstone Team
 * Copyright: Crownstone B.V.
 * Date: May 19, 2016
 * License: LGPLv3+
 */
#pragma once

extern "C" {
#include <nrf_drv_saadc.h>
}
#include "structs/cs_PowerSamples.h"
#include "structs/buffer/cs_CircularBuffer.h"
#include "cfg/cs_Boards.h"
#include "drivers/cs_ADC.h"
#include "third/Median.h"
#include "events/cs_EventListener.h"

typedef void (*ps_zero_crossing_cb_t) ();

class PowerSampling : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static PowerSampling& getInstance() {
		static PowerSampling instance;
		return instance;
	}

	void init(const boards_config_t& boardConfig);

	void stopSampling();

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
	void powerSampleAdcDone(nrf_saadc_value_t* buf, uint16_t size, uint8_t bufNum);

	/** Called at a short interval.
	 *  Reads out the buffer.
	 *  Sends the samples via notifications and/or mesh.
	 */
	void sentDone();
	static void staticPowerSampleRead(PowerSampling *ptr) {
		ptr->sentDone();
	}

	/** Fill up the current curve and send it out over bluetooth
	 * @type specifies over which characteristic the current curve should be sent.
	 */
	void sampleCurrentDone(uint8_t type);

	void getBuffer(buffer_ptr_t& buffer, uint16_t& size);

	/** Enable zero crossing detection on given channel, generating interrupts.
	 *
	 * @param[in] callback             Function to be called on a zero crossing event. This function will run at interrupt level!
	 */
	void enableZeroCrossingInterrupt(ps_zero_crossing_cb_t callback);

	/** handle (crownstone) events
	 */
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

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

	//! Timer instance
	app_timer_t              _powerSamplingReadTimerData;
	app_timer_id_t           _powerSamplingSentDoneTimerId;

	//! Operation mode of this device.
	uint8_t _operationMode;

	//! Buffer that holds the data of the power samples.
	buffer_ptr_t _powerSamplesBuffer;

	//! Power samples to be sent via characteristic.
	PowerSamples _powerSamples;

	float _voltageMultiplier; //! Voltage multiplier from settings.
	float _currentMultiplier; //! Current multiplier from settings.
	int32_t _voltageZero; //! Voltage zero from settings.
	int32_t _currentZero; //! Current zero from settings.
	int32_t _powerZero; //! Power zero from settings.

	bool _sendingSamples; //! Whether or not currently sending power samples.

	uint16_t _avgZeroCurrentDiscount;
	uint16_t _avgZeroVoltageDiscount;
	uint16_t _avgPowerDiscount;

	int32_t _avgZeroVoltage; //! Used for storing and calculating the average zero voltage value (times 1000).
	int32_t _avgZeroCurrent; //! Used for storing and calculating the average zero current value (times 1000).
	bool _recalibrateZeroVoltage; //! Whether or not the zero voltage value should be recalculated.
	bool _recalibrateZeroCurrent; //! Whether or not the zero current value should be recalculated.
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


	uint16_t _currentMilliAmpThreshold;    //! Current threshold from settings.
	uint16_t _currentMilliAmpThresholdPwm; //! Current threshold when using dimmer from settings.

	uint32_t _lastEnergyCalculationTicks; //! Ticks of RTC when last energy calculation was performed.
	int64_t _energyUsedmicroJoule; //! Energy used in micro joule

	//! Store the adc config, so that the actual adc config can be changed.
	struct __attribute__((packed)) {
		uint16_t rangeMilliVolt[2];       // For both channels
		uint8_t currentPinGainHigh;       // Stores the current pin
		uint8_t currentPinGainMed;        // Stores the current pin with medium gain
		uint8_t currentPinGainLow;        // Stores the current pin with lowest gain
		uint8_t voltagePin;               // Stores the voltage pin
		uint8_t zeroReferencePin;         // Stores the zero reference pin
		uint8_t voltageChannelPin;        // Stores which pin is currently set on the voltage channel.
		uint8_t voltageChannelUsedAs : 4; // 0 for voltage, 1 for reference, 2 for VDD, 3 for current1, 4 for current2.
		bool currentDifferential     : 1; // True when differential mode is used for current channel (if possible).
		bool voltageDifferential     : 1; // True when differential mode is used for voltage channel (if possible).
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

	/** Copies the adc samples to the power samples struct, to be sent over bluetooth
	 */
	void copyBufferToPowerSamples(power_t power);

	/** Function to be called when the power samples struct is ready to be sent over bluetooth
	 */
	void readyToSendPowerSamples();

	/** Initialize the moving averages
	 */
	void initAverages();
	
	/** Determine which index is actually the current index, this should not be necessary!
	 */
	uint16_t determineCurrentIndex(power_t power);

	/** Calculate the value of the zero line of the voltage samples
	 */
	void calculateVoltageZero(power_t power);

	/** Calculate the value of the zero line of the current samples
	 */
	void calculateCurrentZero(power_t power);

	/** Filter the samples
	 */
	void filter(power_t power);

	/** Calculate the average power usage
	 */
	void calculatePower(power_t power);

	/** Calculate the energy used
	 */
	void calculateEnergy();

	/**
	 * If current goes beyond predefined threshold levels, take action!
	 */
	void checkSoftfuse(int32_t currentRmsMilliAmp, int32_t currentRmsMilliAmpFiltered);

	void toggleVoltageChannelInput();

	void toggleDifferentialModeCurrent();

	void toggleDifferentialModeVoltage();

	void changeRange(uint8_t channel, int32_t amount);
};

