/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */
#pragma once

extern "C" {
#include <nrf_drv_saadc.h>
}
#include <structs/cs_PowerSamples.h>
#include <structs/buffer/cs_CircularBuffer.h>

class PowerSampling {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static PowerSampling& getInstance() {
		static PowerSampling instance;
		return instance;
	}

	void init(uint8_t pinAinCurrent, uint8_t pinAinVoltage);

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

private:
	PowerSampling();

#if (NORDIC_SDK_VERSION >= 11)
//	app_timer_t              _powerSamplingStartTimerData;
//	app_timer_id_t           _powerSamplingStartTimerId;
	app_timer_t              _powerSamplingReadTimerData;
	app_timer_id_t           _powerSamplingSentDoneTimerId;
#else
//	uint32_t _powerSamplingStartTimerId;
	uint32_t _powerSamplingSentDoneTimerId;
#endif

	uint8_t _operationMode;

	buffer_ptr_t _powerSamplesBuffer; //! Buffer that holds the data for burst or continuous sampling

//	CircularBuffer<uint16_t> _currentSampleCircularBuf;
//	CircularBuffer<uint16_t> _voltageSampleCircularBuf;
	power_samples_cont_message_t* _powerSamplesContMsg;
//	uint16_t _powerSamplesCount;
//	uint16_t _lastPowerSample;
//	uint16_t _burstCount;

	PowerSamples _powerSamples;

	uint16_t _burstSamplingInterval;
	uint16_t _contSamplingInterval;
	float _voltageMultiplier;
	float _currentMultiplier;
	int32_t _voltageZero;
	int32_t _currentZero;
	int32_t _powerZero;
//	uint16_t _zeroAvgWindow; // No longer used

	bool _sendingSamples;

	uint16_t _avgZeroVoltageDiscount;
	uint16_t _avgZeroCurrentDiscount;
	uint16_t _avgPowerDiscount;
	int32_t _avgZeroVoltage; //! Used for storing and calculating the average zero voltage value
	int32_t _avgZeroCurrent; //! Used for storing and calculating the average zero current value
//	int64_t _avgPower; //! Used for storing and calculating the average power
	double _avgPower; //! Used for storing and calculating the average power
	int32_t _avgPowerMilliWatt; //! Used to send out the average power

	uint16_t _currentThreshold;
	uint16_t _currentThresholdPwm;

	/** Copies the adc samples to the power samples struct, to be sent over bluetooth
	 */
	void copyBufferToPowerSamples(nrf_saadc_value_t* buf, uint16_t length, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex);

	/** Function to be called when the power samples struct is ready to be sent over bluetooth
	 */
	void readyToSendPowerSamples();

	/** Initialize the moving averages
	 */
	void initAverages();

	/** Determine which index is actually the current index, this should not be necessary!
	 */
	uint16_t determineCurrentIndex(nrf_saadc_value_t* buf, uint16_t length, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex, uint32_t sampleIntervalUs, uint32_t acPeriodUs);

	/** Calculate the value of the zero line of the voltage samples
	 */
	void calculateVoltageZero(nrf_saadc_value_t* buf, uint16_t length, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex, uint32_t sampleIntervalUs, uint32_t acPeriodUs);

	/** Calculate the value of the zero line of the current samples
	 */
	void calculateCurrentZero(nrf_saadc_value_t* buf, uint16_t length, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex, uint32_t sampleIntervalUs, uint32_t acPeriodUs);

	/** Calculate the average power usage
	 */
	void calculatePower(nrf_saadc_value_t* buf, size_t length, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex, uint32_t sampleIntervalUs, uint32_t acPeriodUs);
};

