/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */

#include <processing/cs_PowerSampling.h>

#include <storage/cs_Settings.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_RTC.h>
#include <protocol/cs_StateTypes.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_State.h>
#include <processing/cs_Switch.h>
#include <third/optmed.h>

#if BUILD_MESHING == 1
#include <mesh/cs_MeshControl.h>
#endif

#include <math.h>

#define PRINT_POWERSAMPLING_VERBOSE

#define VOLTAGE_CHANNEL_IDX 0
#define CURRENT_CHANNEL_IDX 1

PowerSampling::PowerSampling() :
		_isInitialized(false),
		_adc(NULL),
		_powerSamplingSentDoneTimerId(NULL),
		_powerSamplesBuffer(NULL)
{
	_powerSamplingReadTimerData = { {0} };
	_powerSamplingSentDoneTimerId = &_powerSamplingReadTimerData;
	_adc = &(ADC::getInstance());
	_powerMilliWattHist = new CircularBuffer<int32_t>(POWER_SAMPLING_WINDOW_SIZE);
	_currentRmsMilliAmpHist = new CircularBuffer<int32_t>(POWER_SAMPLING_WINDOW_SIZE);
	_voltageRmsMilliVoltHist = new CircularBuffer<int32_t>(POWER_SAMPLING_WINDOW_SIZE);
}

// adc done callback is already decoupled from adc interrupt
void adc_done_callback(nrf_saadc_value_t* buf, uint16_t size, uint8_t bufNum) {
	PowerSampling::getInstance().powerSampleAdcDone(buf, size, bufNum);
}

void PowerSampling::init(const boards_config_t& boardConfig) {
//	memcpy(&_config, &config, sizeof(power_sampling_config_t));

	Timer::getInstance().createSingleShot(_powerSamplingSentDoneTimerId, (app_timer_timeout_handler_t)PowerSampling::staticPowerSampleRead);

	Settings& settings = Settings::getInstance();
	settings.get(CONFIG_VOLTAGE_MULTIPLIER, &_voltageMultiplier);
	settings.get(CONFIG_CURRENT_MULTIPLIER, &_currentMultiplier);
	settings.get(CONFIG_VOLTAGE_ZERO, &_voltageZero);
	settings.get(CONFIG_CURRENT_ZERO, &_currentZero);
	settings.get(CONFIG_POWER_ZERO, &_powerZero);
	settings.get(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD, &_currentThreshold);
	settings.get(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM, &_currentThresholdPwm);

	initAverages();
	_recalibrateZeroVoltage = true;
	_recalibrateZeroCurrent = true;
	_avgZeroVoltageDiscount = VOLTAGE_ZERO_EXP_AVG_DISCOUNT;
	_avgZeroCurrentDiscount = CURRENT_ZERO_EXP_AVG_DISCOUNT;
	_avgPowerDiscount = POWER_EXP_AVG_DISCOUNT;
	_sendingSamples = false;

	LOGi(FMT_INIT, "buffers");
	uint16_t burstSize = _powerSamples.getMaxLength();

	size_t size = burstSize;
	_powerSamplesBuffer = (buffer_ptr_t) calloc(size, sizeof(uint8_t));
#ifdef PRINT_POWERSAMPLING_VERBOSE
	LOGd("power sample buffer=%u size=%u", _powerSamplesBuffer, size);
#endif

	_powerSamples.assign(_powerSamplesBuffer, size);
	_powerMilliWattHist->init(); // Allocates buffer
	_currentRmsMilliAmpHist->init(); // Allocates buffer
	_voltageRmsMilliVoltHist->init(); // Allocates buffer

	LOGd(FMT_INIT, "ADC");
	adc_config_t adcConfig;
	adcConfig.channelCount = 2;
	adcConfig.channels[VOLTAGE_CHANNEL_IDX].pin = boardConfig.pinAinVoltage;
	adcConfig.channels[VOLTAGE_CHANNEL_IDX].rangeMilliVolt = boardConfig.voltageRange;
	adcConfig.channels[VOLTAGE_CHANNEL_IDX].referencePin = boardConfig.flags.hasAdcZeroRef ? boardConfig.pinAinZeroRef : CS_ADC_REF_PIN_NOT_AVAILABLE;
	adcConfig.channels[CURRENT_CHANNEL_IDX].pin = boardConfig.pinAinCurrent;
	adcConfig.channels[CURRENT_CHANNEL_IDX].rangeMilliVolt = boardConfig.currentRange;
	adcConfig.channels[CURRENT_CHANNEL_IDX].referencePin = boardConfig.flags.hasAdcZeroRef ? boardConfig.pinAinZeroRef : CS_ADC_REF_PIN_NOT_AVAILABLE;
	adcConfig.samplingPeriodUs = CS_ADC_SAMPLE_INTERVAL_US;
	_adc->init(adcConfig);

	_adc->setDoneCallback(adc_done_callback);

	_isInitialized = true;
}

void PowerSampling::startSampling() {
#ifdef PRINT_POWERSAMPLING_VERBOSE
	LOGd(FMT_START, "power sample");
#endif
	// Get operation mode
	State::getInstance().get(STATE_OPERATION_MODE, _operationMode);

	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_START);
	_powerSamples.clear();

	_adc->start();
}

void PowerSampling::stopSampling() {
	// todo:
}

void PowerSampling::sentDone() {
	_sendingSamples = false;
}

void PowerSampling::enableZeroCrossingInterrupt(ps_zero_crossing_cb_t callback) {
	_adc->setZeroCrossingCallback(callback);
	// Simply use the zero from the board config, that should be accurate enough for this purpose.
	_adc->enableZeroCrossingInterrupt(VOLTAGE_CHANNEL_IDX, _voltageZero);
}
	
/**
 * After ADC has finished, calculate power consumption, copy data if required, and release buffer.
 *
 * Only when in normal operation mode (e.g. not in setup mode) sent the information to a BLE characteristic.
 */
void PowerSampling::powerSampleAdcDone(nrf_saadc_value_t* buf, uint16_t size, uint8_t bufNum) {
	power_t power;
	power.buf = buf;
	power.bufSize = size;
	power.voltageIndex = VOLTAGE_CHANNEL_IDX;
	power.currentIndex = CURRENT_CHANNEL_IDX;
	power.numChannels = 2;
	power.sampleIntervalUs = CS_ADC_SAMPLE_INTERVAL_US;
	power.acPeriodUs = 20000;

	if (_recalibrateZeroVoltage) {
		calculateVoltageZero(power);
//		_recalibrateZeroVoltage = false;
	}
	if (_recalibrateZeroCurrent) {
		calculateCurrentZero(power);
//		_recalibrateZeroCurrent = false;
	}
	calculatePower(power);

	if (_operationMode == OPERATION_MODE_NORMAL) {
		if (!_sendingSamples) {
			copyBufferToPowerSamples(power);
		}
		EventDispatcher::getInstance().dispatch(STATE_POWER_USAGE, &_avgPowerMilliWatt, sizeof(_avgPowerMilliWatt)); 
	}

	_adc->releaseBuffer(buf);
}

void PowerSampling::getBuffer(buffer_ptr_t& buffer, uint16_t& size) {
	_powerSamples.getBuffer(buffer, size);
}

/**
 * Fill the number of samples in the characteristic.
 */
void PowerSampling::copyBufferToPowerSamples(power_t power) {
	// First clear the old samples
	// Dispatch event that samples are will be cleared
	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_START);
	_powerSamples.clear();
	// Use dummy timestamps for now, for backward compatibility
	uint32_t startTime = RTC::getCount(); // Not really the start time
	uint32_t dT = CS_ADC_SAMPLE_INTERVAL_US * RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000 / 1000;

	for (int i = 0; i < power.bufSize; i += power.numChannels) {
		if (_powerSamples.getCurrentSamplesBuffer()->full() || _powerSamples.getVoltageSamplesBuffer()->full()) {
			readyToSendPowerSamples();
			return;
		}
		_powerSamples.getCurrentSamplesBuffer()->push(power.buf[i+power.currentIndex]);
		_powerSamples.getVoltageSamplesBuffer()->push(power.buf[i+power.voltageIndex]);
		if ((!_powerSamples.getCurrentTimestampsBuffer()->push(startTime + (i/power.numChannels) * dT)) || 
				(!_powerSamples.getVoltageTimestampsBuffer()->push(startTime + (i/power.numChannels) * dT))) {
			_powerSamples.getCurrentSamplesBuffer()->clear();
			_powerSamples.getVoltageSamplesBuffer()->clear();
			return;
		}
	}
	//! TODO: are we actually ready here?
	readyToSendPowerSamples();
}

void PowerSampling::readyToSendPowerSamples() {
	//! Mark that the power samples are being sent now
	_sendingSamples = true;
	//! Dispatch event that samples are now filled and ready to be sent
	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_END, _powerSamplesBuffer, _powerSamples.getDataLength());
	//! Simply use an amount of time for sending, should be event based or polling based
	Timer::getInstance().start(_powerSamplingSentDoneTimerId, MS_TO_TICKS(3000), this);
}

void PowerSampling::initAverages() {
	_avgZeroVoltage = _voltageZero * 1000;
	_avgZeroCurrent = _currentZero * 1000;
	_avgPower = 0.0;
}

/**
 * This just returns the given currentIndex. 
 */
uint16_t PowerSampling::determineCurrentIndex(power_t power) {
	return power.currentIndex;
}

/**
 * The voltage curve is a distorted sinusoid. We calculate the zero(-crossing) by averaging over the buffer over 
 * exactly one cycle (positive and negative) of the sinusoid. The cycle does not start at a particular known phase.
 *
 * @param buf                                    Series of samples for voltage and current (we skip every numChannel).
 * @param bufSize                                Not used.
 * @param numChannels                            By default 2 channels, voltage and current.
 * @param voltageIndex                           Offset into the array.
 * @param currentIndex                           Offset into the array for current (irrelevant).
 * @param sampleIntervalUs                       CS_ADC_SAMPLE_INTERVAL_US (default 200).
 * @param acPeriodUs                             20000 (at 50Hz this is 20.000 microseconds, this means: 100 samples).
 *
 * We iterate over the buffer. The number of samples within a single buffer (either voltage or current) depends on the
 * period in microseconds and the sample interval also in microseconds.
 */
void PowerSampling::calculateVoltageZero(power_t power) {
	uint16_t numSamples = power.acPeriodUs / power.sampleIntervalUs; 

	int64_t sum = 0;
	for (int i = power.voltageIndex; i < numSamples * power.numChannels; i += power.numChannels) {
		sum += power.buf[i];
	}
//	_avgZeroVoltage = sum * 1000 / numSamples;
	
	//! Exponential moving average
	_avgZeroVoltage = ((1000 - _avgZeroVoltageDiscount) * _avgZeroVoltage + _avgZeroVoltageDiscount * sum * 1000 / numSamples) / 1000;
}

/**
 * The same as for the voltage curve, but for the current.
 */
void PowerSampling::calculateCurrentZero(power_t power) {
	uint16_t numSamples = power.acPeriodUs / power.sampleIntervalUs; 

	int64_t sum = 0;
	for (int i = power.currentIndex; i < numSamples * power.numChannels; i += power.numChannels) {
		sum += power.buf[i];
	}
//	_avgZeroCurrent = sum * 1000 / numSamples;


	//! Exponential moving average
	_avgZeroCurrent = ((1000 - _avgZeroCurrentDiscount) * _avgZeroCurrent + _avgZeroCurrentDiscount * sum * 1000 / numSamples) / 1000;
//	write("sum=%lli numSamples=%u czero=%i \r\n", sum, numSamples, _avgZeroCurrent);
}

/**
 * Calculate power.
 *
 * The int64_t sum is large enough: 2^63 / (2^12 * 1000 * 2^12 * 1000) = 5*10^5. Many more samples than the 100 we use.
 */
void PowerSampling::calculatePower(power_t power) {

	uint16_t numSamples = power.acPeriodUs / power.sampleIntervalUs; 

	if ((int)power.bufSize < numSamples * power.numChannels) {
		LOGe("Should have at least a whole period in a buffer!");
		return;
	}

	int64_t pSum = 0;
	int64_t cSquareSum = 0;
	int64_t vSquareSum = 0;
	int64_t current;
	int64_t voltage;
	for (uint16_t i = 0; i < numSamples * power.numChannels; i += power.numChannels) {
		current = ((int64_t)power.buf[i+power.currentIndex]*1000 - _avgZeroCurrent);
		voltage = ((int64_t)power.buf[i+power.voltageIndex]*1000 - _avgZeroVoltage);
		cSquareSum += (current * current) / (1000*1000);
		vSquareSum += (voltage * voltage) / (1000*1000);
		pSum +=       (current * voltage) / (1000*1000);
	}
	int32_t powerMilliWatt = pSum * _currentMultiplier * _voltageMultiplier * 1000 / numSamples - _powerZero;
	int32_t currentRmsMilliAmp =  sqrt((double)cSquareSum * _currentMultiplier * _currentMultiplier / numSamples) * 1000;
	int32_t voltageRmsMilliVolt = sqrt((double)vSquareSum * _voltageMultiplier * _voltageMultiplier / numSamples) * 1000;

//	// Exponential moving average
//	int64_t discountCurrent = 50;
//	_avgCurrentRmsMilliAmp = ((1000-discountCurrent) * _avgCurrentRmsMilliAmp + discountCurrent * currentRmsMilliAmp) / 1000;

	// Calculate median when there are enough values in history, else calculate the average.
	_currentRmsMilliAmpHist->push(currentRmsMilliAmp);
	if (_currentRmsMilliAmpHist->full()) {
		memcpy(_histCopy, _currentRmsMilliAmpHist->getBuffer(), _currentRmsMilliAmpHist->getMaxByteSize());
		_avgCurrentRmsMilliAmp = opt_med9(_histCopy);
	}
	else {
		int64_t currentRmsMilliAmpSum = 0;
		for (uint16_t i=0; i<_currentRmsMilliAmpHist->size(); ++i) {
			currentRmsMilliAmpSum += _currentRmsMilliAmpHist->operator [](i);
		}
		_avgCurrentRmsMilliAmp = currentRmsMilliAmpSum / _currentRmsMilliAmpHist->size();
	}


	// TODO: use _avgCurrentRmsMilliAmp instead.
	checkSoftfuse(powerMilliWatt);


	// Calculate median when there are enough values in history, else calculate the average.
	_voltageRmsMilliVoltHist->push(voltageRmsMilliVolt);
	if (_voltageRmsMilliVoltHist->full()) {
		memcpy(_histCopy, _voltageRmsMilliVoltHist->getBuffer(), _voltageRmsMilliVoltHist->getMaxByteSize());
		_avgVoltageRmsMilliVolt = opt_med9(_histCopy);
	}
	else {
		int64_t voltageRmsMilliVoltSum = 0;
		for (uint16_t i=0; i<_voltageRmsMilliVoltHist->size(); ++i) {
			voltageRmsMilliVoltSum += _voltageRmsMilliVoltHist->operator [](i);
		}
		_avgVoltageRmsMilliVolt = voltageRmsMilliVoltSum / _voltageRmsMilliVoltHist->size();
	}

	// Instead of real power, calculate apparent power: current_rms * voltage_rms
	powerMilliWatt = (int64_t)_avgCurrentRmsMilliAmp * _avgVoltageRmsMilliVolt / 1000;

//	// Calculate median when there are enough values in history, else calculate the average.
//	_powerMilliWattHist->push(powerMilliWatt);
//	if (_powerMilliWattHist->full()) {
//		memcpy(_histCopy, _powerMilliWattHist->getBuffer(), _powerMilliWattHist->getMaxByteSize());
//		_avgPowerMilliWatt = opt_med9(_histCopy);
//	}
//	else {
//		int64_t powerMilliWattSum = 0;
//		for (uint16_t i=0; i<_powerMilliWattHist->size(); ++i) {
//			powerMilliWattSum += _powerMilliWattHist->operator [](i);
//		}
//		_avgPowerMilliWatt = powerMilliWattSum / _powerMilliWattHist->size();
//	}

	// Exponential moving average
	int64_t avgPowerDiscount = _avgPowerDiscount;
	_avgPowerMilliWatt = ((1000-avgPowerDiscount) * _avgPowerMilliWatt + avgPowerDiscount * powerMilliWatt) / 1000;
//	_avgPowerMilliWatt = powerMilliWatt;


//	static int printPower = 0;
//	if (printPower % 1 == 0) {
//		write("%i %i\r\n", currentRmsMilliAmp, _avgCurrentRmsMilliAmp);
//		write("%i %i\r\n", voltageRmsMilliVolt, _avgVoltageRmsMilliVolt);
//		write("%i %i\r\n", powerMilliWatt, _avgPowerMilliWatt);
//	}
//	printPower++;

//	static int printPower = 0;
//	if (printPower % 100 == 0) {
//		write("vZero=%i cZero=%i\r\n", _avgZeroVoltage, _avgZeroCurrent);
//		write("pSum=%lld \r\n", pSum);
//		write("power=%lld avg=%d\r\n",powerMilliWatt, _avgPowerMilliWatt);
////		printPower = 0;
//		// current
//		write("Current: ");
//		write("\r\n");
//		for (int i = power.currentIndex; i < numSamples * power.numChannels; i += power.numChannels) {
//			write("%4d ", power.buf[i]);
//			if (i % 40 == 40 - 1) {
//				write("\r\n");
//			}
//		}
//		write("\r\n");
//		write("Voltage: ");
//		write("\r\n");
//		for (int i = power.voltageIndex; i < numSamples * power.numChannels; i += power.numChannels) {
//			write("%4d ", power.buf[i]);
//			if (i % 40 == 40 - 2) {
//				write("\r\n");
//			}
//		}
//		write("\r\n");
//	}
//	printPower++;
}

void PowerSampling::checkSoftfuse(int64_t powerMilliWatt) {
	//! Get the current state errors
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, &stateErrors, sizeof(state_errors_t));

	// TODO: don't use hardcoded 220V here
	if ((powerMilliWatt > _currentThreshold * 220) && (!stateErrors.errors.overCurrent)) {
		LOGd("current above threshold");
		EventDispatcher::getInstance().dispatch(EVT_CURRENT_USAGE_ABOVE_THRESHOLD);
		State::getInstance().set(STATE_ERROR_OVER_CURRENT, (uint8_t)1);
	}
	else if ((powerMilliWatt > _currentThresholdPwm * 220) && (!stateErrors.errors.overCurrentPwm)) {
		//! Get the current pwm state before we dispatch the event (as that may change the pwm).
		switch_state_t switchState;
		State::getInstance().get(STATE_SWITCH_STATE, &switchState, sizeof(switch_state_t));
		if (switchState.pwm_state != 0) {
			//! If the pwm was on:
			LOGd("current above pwm threshold");
			//! Dispatch the event that will turn off the pwm
			EventDispatcher::getInstance().dispatch(EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM);
			//! Set overcurrent error.
			State::getInstance().set(STATE_ERROR_OVER_CURRENT_PWM, (uint8_t)1);
		}
	}
}

