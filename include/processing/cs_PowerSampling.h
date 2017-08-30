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

typedef void (*ps_zero_crossing_cb_t) ();

class PowerSampling {
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

	uint16_t _currentThreshold; //! Current threshold from settings.
	uint16_t _currentThresholdPwm; //! Current threshold when using dimmer from settings.

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

	/** Calculate the average power usage
	 */
	void calculatePower(power_t power);

	/**
	 * If current goes beyond predefined threshold levels, take action!
	 */
	void checkSoftfuse(int64_t powerMilliWatt);
};

