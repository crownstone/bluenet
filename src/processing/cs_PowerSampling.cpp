/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 19, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_PowerSampling.h"

#include "storage/cs_Settings.h"
#include "drivers/cs_Serial.h"
#include "drivers/cs_RTC.h"
#include "protocol/cs_StateTypes.h"
#include "events/cs_EventDispatcher.h"
#include "storage/cs_State.h"
#include "processing/cs_Switch.h"
#include "third/optmed.h"
#include "protocol/cs_UartProtocol.h"
#include "protocol/cs_UartMsgTypes.h"

#if BUILD_MESHING == 1
#include "mesh/cs_MeshControl.h"
#endif

#include <math.h>
#include "third/SortMedian.h"

#include "processing/cs_RecognizeSwitch.h"
#include "structs/buffer/cs_InterleavedBuffer.h"

// Define test pin to enable gpio debug.
//#define TEST_PIN 20

// Define to print power samples
#define PRINT_POWER_SAMPLES

#define VOLTAGE_CHANNEL_IDX 0
#define CURRENT_CHANNEL_IDX 1

PowerSampling::PowerSampling() :
		_isInitialized(false),
		_adc(NULL),
		_powerSamplingSentDoneTimerId(NULL),
		_powerSamplesBuffer(NULL),
		_consecutivePwmOvercurrent(0),
		_lastEnergyCalculationTicks(0),
		_energyUsedmicroJoule(0),
//		_lastSwitchState(0),
		_lastSwitchOffTicks(0),
		_lastSwitchOffTicksValid(false),
		_igbtFailureDetectionStarted(false)
{
	_powerSamplingReadTimerData = { {0} };
	_powerSamplingSentDoneTimerId = &_powerSamplingReadTimerData;
	_adc = &(ADC::getInstance());
	_powerMilliWattHist = new CircularBuffer<int32_t>(POWER_SAMPLING_RMS_WINDOW_SIZE);
	_currentRmsMilliAmpHist = new CircularBuffer<int32_t>(POWER_SAMPLING_RMS_WINDOW_SIZE);
	_voltageRmsMilliVoltHist = new CircularBuffer<int32_t>(POWER_SAMPLING_RMS_WINDOW_SIZE);
	_filteredCurrentRmsHistMA = new CircularBuffer<int32_t>(POWER_SAMPLING_RMS_WINDOW_SIZE);
	_logsEnabled.asInt = 0;
}

#if POWER_SAMPLING_RMS_WINDOW_SIZE == 7
#define opt_med(arr) opt_med7(arr)
#elif POWER_SAMPLING_RMS_WINDOW_SIZE == 9
#define opt_med(arr) opt_med9(arr)
#elif POWER_SAMPLING_RMS_WINDOW_SIZE == 25
#define opt_med(arr) opt_med25(arr)
#endif

#ifdef PRINT_POWER_SAMPLES
static int printPower = 0;
#endif

/*
 * At this moment in time is the function adc_done_callback already decoupled from the ADC interrupt. 
 */
void adc_done_callback(cs_adc_buffer_id_t bufIndex) {
	PowerSampling::getInstance().powerSampleAdcDone(bufIndex);
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
	settings.get(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD, &_currentMilliAmpThreshold);
	settings.get(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM, &_currentMilliAmpThresholdPwm);
	bool switchcraftEnabled = settings.isSet(CONFIG_SWITCHCRAFT_ENABLED);

	RecognizeSwitch::getInstance().init();
	float switchcraftThreshold;
	settings.get(CONFIG_SWITCHCRAFT_THRESHOLD, &switchcraftThreshold);
	RecognizeSwitch::getInstance().configure(switchcraftThreshold);
	enableSwitchcraft(switchcraftEnabled);

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
	LOGd("power sample buffer=%u size=%u", _powerSamplesBuffer, size);

	_powerSamples.assign(_powerSamplesBuffer, size);
	_powerMilliWattHist->init(); // Allocates buffer
	_currentRmsMilliAmpHist->init(); // Allocates buffer
	_voltageRmsMilliVoltHist->init(); // Allocates buffer
	_filteredCurrentRmsHistMA->init(); // Allocates buffer

	// Init moving median filter
	unsigned halfWindowSize = POWER_SAMPLING_CURVE_HALF_WINDOW_SIZE;
//	unsigned halfWindowSize = 5;  // Takes 0.74ms
//	unsigned halfWindowSize = 16; // Takes 0.93ms
	unsigned windowSize = halfWindowSize * 2 + 1;
	uint16_t bufSize = CS_ADC_BUF_SIZE / 2;
	unsigned blockCount = (bufSize + halfWindowSize*2) / windowSize; // Shouldn't have a remainder!
	_filterParams = new MedianFilter(halfWindowSize, blockCount);
	_inputSamples = new PowerVector(bufSize + halfWindowSize*2);
	_outputSamples = new PowerVector(bufSize);

	LOGd(FMT_INIT, "ADC");
	adc_config_t adcConfig;
	adcConfig.channelCount = 2;
	adcConfig.channels[VOLTAGE_CHANNEL_IDX].pin = boardConfig.pinAinVoltage;
	adcConfig.channels[VOLTAGE_CHANNEL_IDX].rangeMilliVolt = boardConfig.voltageRange;
	adcConfig.channels[VOLTAGE_CHANNEL_IDX].referencePin = boardConfig.flags.hasAdcZeroRef ? boardConfig.pinAinZeroRef : CS_ADC_REF_PIN_NOT_AVAILABLE;
//	adcConfig.channels[CURRENT_CHANNEL_IDX].pin = boardConfig.pinAinCurrentGainMed;
	adcConfig.channels[CURRENT_CHANNEL_IDX].pin = boardConfig.pinAinCurrentGainLow;
	adcConfig.channels[CURRENT_CHANNEL_IDX].rangeMilliVolt = boardConfig.currentRange;
	adcConfig.channels[CURRENT_CHANNEL_IDX].referencePin = boardConfig.flags.hasAdcZeroRef ? boardConfig.pinAinZeroRef : CS_ADC_REF_PIN_NOT_AVAILABLE;
	adcConfig.samplingPeriodUs = CS_ADC_SAMPLE_INTERVAL_US;
	_adc->init(adcConfig);

	_adc->setDoneCallback(adc_done_callback);

	// init the adc config
	_adcConfig.rangeMilliVolt[VOLTAGE_CHANNEL_IDX] = boardConfig.voltageRange;
	_adcConfig.rangeMilliVolt[CURRENT_CHANNEL_IDX] = boardConfig.currentRange;
	_adcConfig.currentPinGainHigh = boardConfig.pinAinCurrentGainHigh;
	_adcConfig.currentPinGainMed  = boardConfig.pinAinCurrentGainMed;
	_adcConfig.currentPinGainLow  = boardConfig.pinAinCurrentGainLow;
	_adcConfig.voltagePin = boardConfig.pinAinVoltage;
	_adcConfig.zeroReferencePin = adcConfig.channels[CURRENT_CHANNEL_IDX].referencePin;
	_adcConfig.voltageChannelPin = _adcConfig.voltagePin;
	_adcConfig.voltageChannelUsedAs = 0;
	_adcConfig.currentDifferential = true;
	_adcConfig.voltageDifferential = true;

	EventDispatcher::getInstance().addListener(this);

#ifdef TEST_PIN
	nrf_gpio_cfg_output(TEST_PIN);
#endif

	_isInitialized = true;
}

void PowerSampling::startSampling() {
	LOGi(FMT_START, "power sample");
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

void PowerSampling::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_ENABLE_LOG_POWER:
		_logsEnabled.flags.power = *(uint8_t*)p_data;
		break;
	case EVT_ENABLE_LOG_CURRENT:
		_logsEnabled.flags.current = *(uint8_t*)p_data;
		break;
	case EVT_ENABLE_LOG_VOLTAGE:
		_logsEnabled.flags.voltage = *(uint8_t*)p_data;
		break;
	case EVT_ENABLE_LOG_FILTERED_CURRENT:
		_logsEnabled.flags.filteredCurrent = *(uint8_t*)p_data;
		break;
	case EVT_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN:
		toggleVoltageChannelInput();
		break;
	case EVT_ENABLE_ADC_DIFFERENTIAL_CURRENT:
		enableDifferentialModeCurrent(*(uint8_t*)p_data);
		break;
	case EVT_ENABLE_ADC_DIFFERENTIAL_VOLTAGE:
		enableDifferentialModeVoltage(*(uint8_t*)p_data);
		break;
	case EVT_INC_VOLTAGE_RANGE:
		changeRange(VOLTAGE_CHANNEL_IDX, 600);
		break;
	case EVT_DEC_VOLTAGE_RANGE:
		changeRange(VOLTAGE_CHANNEL_IDX, -600);
		break;
	case EVT_INC_CURRENT_RANGE:
		changeRange(CURRENT_CHANNEL_IDX, 600);
		break;
	case EVT_DEC_CURRENT_RANGE:
		changeRange(CURRENT_CHANNEL_IDX, -600);
		break;
	case EVT_SWITCHCRAFT_ENABLED:
		enableSwitchcraft(*(bool*)p_data);
		break;
	case EVT_ADC_RESTARTED:
//		uint32_t timestamp = RTC::getCount();
//		UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_ADC_RESTART, (uint8_t*)(&timestamp), sizeof(timestamp));
		UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_ADC_RESTART, NULL, 0);
		RecognizeSwitch::getInstance().skip(2);
		break;
	case CONFIG_SWITCHCRAFT_THRESHOLD:
		RecognizeSwitch::getInstance().configure(*(float*)p_data);
		break;
	}
}

/**
 * After ADC has finished, calculate power consumption, copy data if required, and release buffer.
 * Only when in normal operation mode (e.g. not in setup mode) sent the information to a BLE characteristic.
 * Responsibilities of this function:
 *
 * 1. This function fills first a power struct with information about the number of channels, which is the voltage and
 * which the current channel, the current buffer, the buffer size, the sample interval and the AC period in 
 * microseconds.
 * 2. The function filters the current curve.
 * 3. The zero-crossings of voltage and current are calculated.
 * 4. Power and energy are calculated.
 * 5. The data is send over Bluetooth in normal mode.
 * 6. It tries to recognize a physical switch event using the voltage curve.
 * 7. It releases the buffer to the ADC.
 *
 * @param[in] buf                                Buffer populated by the ADC peripheral.
 * @param[in] size                               Size of the buffer.
 * @param[in] bufIndex                           The buffer index, can be used in InterleavedBuffer.
 */
void PowerSampling::powerSampleAdcDone(cs_adc_buffer_id_t bufIndex) {
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif
	power_t power;
	power.buf = InterleavedBuffer::getInstance().getBuffer(bufIndex);
	power.bufSize = CS_ADC_BUF_SIZE;
	power.voltageIndex = VOLTAGE_CHANNEL_IDX;
	power.currentIndex = CURRENT_CHANNEL_IDX;
	power.numChannels = 2;
	power.sampleIntervalUs = CS_ADC_SAMPLE_INTERVAL_US;
	power.acPeriodUs = 20000;
	
	cs_adc_buffer_id_t prevIndex = InterleavedBuffer::getInstance().getPrevious(bufIndex);

#ifdef DELAY_FILTERING_VOLTAGE
	// Filter the current immediately, filter the voltage in the buffer at time t-1 (so raw values are available)
	filter(bufIndex, power.currentIndex);
	filter(prevIndex, power.voltageIndex);
#else
	// Filter both immediately, no raw values available
	filter(bufIndex, power.voltageIndex);
	filter(bufIndex, power.currentIndex);
#endif

#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	if (_recalibrateZeroVoltage) {
		calculateVoltageZero(power);
	}
	if (_recalibrateZeroCurrent) {
		calculateCurrentZero(power);
	}

#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	calculatePower(power);
	calculateEnergy();

	if (_operationMode == OPERATION_MODE_NORMAL) {
		// Note, sendingSamples is a mutex. If we are already sending smples, we will not wait and just skip.
		if (!_sendingSamples) {
			copyBufferToPowerSamples(power);
		}
		// TODO: use State.set() for this.
		EventDispatcher::getInstance().dispatch(STATE_POWER_USAGE, &_avgPowerMilliWatt, sizeof(_avgPowerMilliWatt));
		EventDispatcher::getInstance().dispatch(STATE_ACCUMULATED_ENERGY, &_energyUsedmicroJoule, sizeof(_energyUsedmicroJoule));
//		EventDispatcher::getInstance().dispatch(STATE_ACCUMULATED_ENERGY, &_avgVoltageRmsMilliVolt, sizeof(_avgVoltageRmsMilliVolt));
//		EventDispatcher::getInstance().dispatch(STATE_ACCUMULATED_ENERGY, &_avgCurrentRmsMilliAmp, sizeof(_avgCurrentRmsMilliAmp));
//		EventDispatcher::getInstance().dispatch(STATE_ACCUMULATED_ENERGY, &_avgZeroCurrent, sizeof(_avgZeroCurrent)); // Send NTC voltage
	}

#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	bool switch_detected = RecognizeSwitch::getInstance().detect(prevIndex, power.voltageIndex);
	if (switch_detected) {
		LOGd("Switch event detected!");
		EventDispatcher::getInstance().dispatch(EVT_POWER_TOGGLE);
	}

	_adc->releaseBuffer(bufIndex);
}

void PowerSampling::getBuffer(buffer_ptr_t& buffer, uint16_t& size) {
	_powerSamples.getBuffer(buffer, size);
}

/**
 * Fill the number of samples in the Bluetooth Low Energy characteristic. This clears the old samples actively. A
 * warning is dispatched beforehand, EVT_POWER_SAMPLES_START so other listeners know (which ones) they should 
 * invalidate the current buffer.
 *
 * @param[in] power                              Reference to buffer, current and voltage channel
 */
void PowerSampling::copyBufferToPowerSamples(power_t power) {
	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_START);
	_powerSamples.clear();
	uint32_t startTime = RTC::getCount(); // Not really the start time

	_powerSamples.setTimestamp(startTime);
	for (int i = 0; i < power.bufSize; i += power.numChannels) {
		if (_powerSamples.getCurrentSamplesBuffer()->full() || _powerSamples.getVoltageSamplesBuffer()->full()) {
			readyToSendPowerSamples();
			return;
		}
		_powerSamples.getCurrentSamplesBuffer()->push(power.buf[i+power.currentIndex]);
		_powerSamples.getVoltageSamplesBuffer()->push(power.buf[i+power.voltageIndex]);
	}
	// TODO: are we actually ready here?
	readyToSendPowerSamples();
}

void PowerSampling::readyToSendPowerSamples() {
	// Mark that the power samples are being sent now
	_sendingSamples = true;

	// Dispatch event that samples are now filled and ready to be sent
	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_END, _powerSamplesBuffer, _powerSamples.getDataLength());

	// Simply use an amount of time for sending, should be event based or polling based
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
	int32_t zeroVoltage = sum * 1000 / numSamples;
	
	// Exponential moving average
	int64_t avgZeroVoltageDiscount = _avgZeroVoltageDiscount; // Make sure calculations are in int64_t
	_avgZeroVoltage = ((1000 - avgZeroVoltageDiscount) * _avgZeroVoltage + avgZeroVoltageDiscount * zeroVoltage) / 1000;
}

/**
 * The same as for the voltage curve, but for the current.
 */
void PowerSampling::calculateCurrentZero(power_t power) {
	uint16_t numSamples = power.acPeriodUs / power.sampleIntervalUs; 

	int64_t sum = 0;
	// Use filtered samples to calculate the zero.
	for (int i = 0; i < numSamples; ++i) {
		sum += _outputSamples->at(i);
	}
	int32_t zeroCurrent = sum * 1000 / numSamples;

	// Exponential moving average
	int64_t avgZeroCurrentDiscount = _avgZeroCurrentDiscount; // Make sure calculations are in int64_t
	_avgZeroCurrent = ((1000 - avgZeroCurrentDiscount) * _avgZeroCurrent + avgZeroCurrentDiscount * zeroCurrent) / 1000;
	
}

/*
 * The median filter uses its own data structure to optimally filter the data. A median filter does filter outliers by
 * a particular smoothing operation. It sorts a sequence of values and picks the one in the center: the median. To
 * optimize this operation it makes sense that if we slide the sliding window with sorted values we do not sort these
 * values again and again. When moving the window we insert the new value from the right into the sorted array. For
 * that it is logical to use a linked list. This means that the MedianFilter object we use requires us copying the
 * data into it. An expensive operation.
 *
 * TODO: Do not work with "half window sizes". Just work with a window size and enforce its size to be odd. That 
 * to pick the median you only need to sort half of the values (plus one) is an implementation detail. 
 *
 * TODO: Create an in-place version of a median filter. It is possible to have a linked list (of half the window size
 * plus one) that contains a list of indices into the array with values. For example, let us assume a full window size
 * of 5 and ignore the border starting at index 2. Now let's sort the first 5 values [4 2 0 1 3]. Hence, the value at
 * index 4 is the smallest. We replace value at index 2 with the middle one (which is at index 0). Now we shift to the
 * right, hence index 0 falls out [4 2 1 3], and index 5 shifts in. Let's assume 5 is a value between 4 and 2, then we
 * have [4 5 2 1 3] and replace the value at index 3 with the value at index 2. Note that the value at index 0 is 
 * copied into 2 and then into 3. If we do not want this, we need to have a separate array the size of the sliding
 * window and only copy the value when it is shifted out. We will have a copy assignment operation for each value,
 * however still our RAM use is much less because we only need place for values at the window. Note that we can 
 * actually sort up to the half so the examples should be [4 2 0 ? ?], then [4 2 ? ?], then [4 5 2 ? ?]. In the 
 * particular case where 5 > 2 we need an additional value [4 2 1 ?] or we do not know if it should be [4 2 5 ? ?] or
 * [4 2 1 ? ?]. So in this case we would need a min(.) operation on half (minus one) of the remaining array.
 *
 * TODO: Keep the newest buffer at t=0 "raw" and only filter the t=-1. If the operation is done in-place we have also
 * filtered buffers for t=-2 and t=-3 (assuming four buffers). We can use the buffer at t=0 and t=-2 for padding the
 * buffer at t=-1. This means we do not pad with copies of values but with real values. All operations that require
 * smoothed values will have a delay of one sine wave (20ms on 50Hz).
 */

/**
 * This function performs a median filter with respect to the given channel.
 */
void PowerSampling::filter(cs_adc_buffer_id_t buffer_id, channel_id_t channel_id) {
	
	// Pad the start of the input vector with the first sample in the buffer
	uint16_t j = 0;
	value_t padded_value = InterleavedBuffer::getInstance().getValue(buffer_id, channel_id, 0);
	for (uint16_t i = 0; i < _filterParams->half; ++i, ++j) {
		_inputSamples->at(j) = padded_value;
	}
	// Copy samples from buffer to input vector
	uint8_t channel_length = InterleavedBuffer::getInstance().getChannelLength();
	for (int i = 0; i < channel_length - 1; ++i, ++j) {
		_inputSamples->at(j) = InterleavedBuffer::getInstance().getValue(buffer_id, channel_id, i);
	}
	// Pad the end of the buffer with the last sample in the buffer
	padded_value = InterleavedBuffer::getInstance().getValue(buffer_id, channel_id, channel_length - 1);
	for (uint16_t i = 0; i < _filterParams->half; ++i, ++j) {
		_inputSamples->at(j) = padded_value;
	}

	// Filter the data
	sort_median(*_filterParams, *_inputSamples, *_outputSamples);

	// TODO: Note that this is a lot of copying back and forth. Filtering should actually be done in-place.
	
	// Copy the result back into the buffer
	for (int i = 0; i < InterleavedBuffer::getInstance().getChannelLength() - 1; ++i) {
		InterleavedBuffer::getInstance().setValue(buffer_id, channel_id, i, _outputSamples->at(i));
	}
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

	//////////////////////////////////////////////////
	// Calculatate power, Irms, and Vrms
	//////////////////////////////////////////////////

	int64_t pSum = 0;
	int64_t cSquareSum = 0;
	int64_t vSquareSum = 0;
	int64_t current;
	int64_t voltage;
	for (uint16_t i = 0; i < numSamples * power.numChannels; i += power.numChannels) {
		current = (int64_t)power.buf[i+power.currentIndex]*1000 - _avgZeroCurrent;
		voltage = (int64_t)power.buf[i+power.voltageIndex]*1000 - _avgZeroVoltage;
		cSquareSum += (current * current) / (1000*1000);
		vSquareSum += (voltage * voltage) / (1000*1000);
		pSum +=       (current * voltage) / (1000*1000);
	}
	pSum *= -1;
	int32_t powerMilliWattReal = pSum * _currentMultiplier * _voltageMultiplier * 1000 / numSamples - _powerZero;
	int32_t currentRmsMA =  sqrt((double)cSquareSum * _currentMultiplier * _currentMultiplier / numSamples) * 1000;
	int32_t voltageRmsMilliVolt = sqrt((double)vSquareSum * _voltageMultiplier * _voltageMultiplier / numSamples) * 1000;



	////////////////////////////////////////////////////////////////////////////////
	// Calculate Irms of median filtered samples, and filter over multiple periods
	////////////////////////////////////////////////////////////////////////////////

	// Calculate Irms again, but now with the filtered current samples
	cSquareSum = 0;
	for (uint16_t i=0; i<numSamples; ++i) {
		current = (int64_t)_outputSamples->at(i)*1000 - _avgZeroCurrent;
		cSquareSum += (current * current) / (1000*1000);
	}
	int32_t filteredCurrentRmsMA =  sqrt((double)cSquareSum * _currentMultiplier * _currentMultiplier / numSamples) * 1000;

	// Calculate median when there are enough values in history, else calculate the average.
	_filteredCurrentRmsHistMA->push(filteredCurrentRmsMA);
	int32_t filteredCurrentRmsMedianMA;
	if (_filteredCurrentRmsHistMA->full()) {
		memcpy(_histCopy, _filteredCurrentRmsHistMA->getBuffer(), _filteredCurrentRmsHistMA->getMaxByteSize());
		filteredCurrentRmsMedianMA = opt_med(_histCopy);
	}
	else {
		int64_t currentRmsSumMA = 0;
		for (uint16_t i=0; i<_filteredCurrentRmsHistMA->size(); ++i) {
			currentRmsSumMA += _filteredCurrentRmsHistMA->operator [](i);
		}
		filteredCurrentRmsMedianMA = currentRmsSumMA / _filteredCurrentRmsHistMA->size();
	}

	// Now that Irms is known: first check the soft fuse.
	checkSoftfuse(filteredCurrentRmsMedianMA, filteredCurrentRmsMedianMA);



	/////////////////////////////////////////////////////////
	// Filter Irms, Vrms, and Power (over multiple periods)
	/////////////////////////////////////////////////////////

	// Calculate median when there are enough values in history, else calculate the average.
	_currentRmsMilliAmpHist->push(currentRmsMA);
	int32_t currentRmsMedianMA;
	if (_currentRmsMilliAmpHist->full()) {
		memcpy(_histCopy, _currentRmsMilliAmpHist->getBuffer(), _currentRmsMilliAmpHist->getMaxByteSize());
		currentRmsMedianMA = opt_med(_histCopy);
	}
	else {
		int64_t currentRmsMilliAmpSum = 0;
		for (uint16_t i=0; i<_currentRmsMilliAmpHist->size(); ++i) {
			currentRmsMilliAmpSum += _currentRmsMilliAmpHist->operator [](i);
		}
		currentRmsMedianMA = currentRmsMilliAmpSum / _currentRmsMilliAmpHist->size();
	}

//	// Exponential moving average of the median
//	int64_t discountCurrent = 200;
//	_avgCurrentRmsMilliAmp = ((1000-discountCurrent) * _avgCurrentRmsMilliAmp + discountCurrent * medianCurrentRmsMilliAmp) / 1000;
	// Use median as average
	_avgCurrentRmsMilliAmp = currentRmsMedianMA;

	// Calculate median when there are enough values in history, else calculate the average.
	_voltageRmsMilliVoltHist->push(voltageRmsMilliVolt);
	if (_voltageRmsMilliVoltHist->full()) {
		memcpy(_histCopy, _voltageRmsMilliVoltHist->getBuffer(), _voltageRmsMilliVoltHist->getMaxByteSize());
		_avgVoltageRmsMilliVolt = opt_med(_histCopy);
	}
	else {
		int64_t voltageRmsMilliVoltSum = 0;
		for (uint16_t i=0; i<_voltageRmsMilliVoltHist->size(); ++i) {
			voltageRmsMilliVoltSum += _voltageRmsMilliVoltHist->operator [](i);
		}
		_avgVoltageRmsMilliVolt = voltageRmsMilliVoltSum / _voltageRmsMilliVoltHist->size();
	}

	// Calculate apparent power: current_rms * voltage_rms
	__attribute__((unused)) uint32_t powerMilliWattApparent = (int64_t)_avgCurrentRmsMilliAmp * _avgVoltageRmsMilliVolt / 1000;

	// Exponential moving average
	int64_t avgPowerDiscount = _avgPowerDiscount;
	_avgPowerMilliWatt = ((1000-avgPowerDiscount) * _avgPowerMilliWatt + avgPowerDiscount * powerMilliWattReal) / 1000;

	/////////////////////////////////////////////////////////
	// Debug prints
	/////////////////////////////////////////////////////////
//	for (int i = power.voltageIndex; i < numSamples * power.numChannels; i += power.numChannels) {
//		if (power.buf[i] < 100) {
//			APP_ERROR_CHECK(1);
//		}
//	}
//	LOGd("I=%u %u %u %u P=%i %i %i", currentRmsMA, currentRmsMedianMA, filteredCurrentRmsMA, filteredCurrentRmsMedianMA, _avgPowerMilliWatt, powerMilliWattReal, powerMilliWattApparent);

#ifdef PRINT_POWER_SAMPLES
	if (printPower % 1 == 0) {
//	if (printPower % 500 == 0) {
//	if (printPower % 500 == 0 || currentRmsMedianMA > _currentMilliAmpThresholdPwm || currentRmsMA > _currentMilliAmpThresholdPwm) {
		uint32_t rtcCount = RTC::getCount();
		if (_logsEnabled.flags.power) {
			// Calculated values
			uart_msg_power_t powerMsg;
			powerMsg.timestamp = rtcCount;
//			powerMsg.timestamp = RTC::getCount();
			powerMsg.currentRmsMA = currentRmsMA;
			powerMsg.currentRmsMedianMA = currentRmsMedianMA;
			powerMsg.filteredCurrentRmsMA = filteredCurrentRmsMA;
			powerMsg.filteredCurrentRmsMedianMA = filteredCurrentRmsMedianMA;
			powerMsg.avgZeroVoltage = _avgZeroVoltage;
			powerMsg.avgZeroCurrent = _avgZeroCurrent;
			powerMsg.powerMilliWattApparent = powerMilliWattApparent;
			powerMsg.powerMilliWattReal = powerMilliWattReal;
			powerMsg.avgPowerMilliWattReal = _avgPowerMilliWatt;

			UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_POWER_LOG_POWER, (uint8_t*)&powerMsg, sizeof(powerMsg));
		}

		if (_logsEnabled.flags.current) {
			// Write uart_msg_current_t without allocating a buffer.
			UartProtocol::getInstance().writeMsgStart(UART_OPCODE_TX_POWER_LOG_CURRENT, sizeof(uart_msg_current_t));
			UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_CURRENT,(uint8_t*)&(rtcCount), sizeof(rtcCount));
			for (int i = power.currentIndex; i < numSamples * power.numChannels; i += power.numChannels) {
				UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_CURRENT, (uint8_t*)&(power.buf[i]), sizeof(nrf_saadc_value_t));
			}
			UartProtocol::getInstance().writeMsgEnd(UART_OPCODE_TX_POWER_LOG_CURRENT);
		}

		if (_logsEnabled.flags.filteredCurrent) {
			// Write uart_msg_current_t without allocating a buffer.
			UartProtocol::getInstance().writeMsgStart(UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT, sizeof(uart_msg_current_t));
			UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT, (uint8_t*)&(rtcCount), sizeof(rtcCount));
			int16_t val;
			for (int i = 0; i < numSamples; ++i) {
				val = _outputSamples->at(i);
				UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT, (uint8_t*)&val, sizeof(val));
			}
			UartProtocol::getInstance().writeMsgEnd(UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT);
		}

		if (_logsEnabled.flags.voltage) {
			// Write uart_msg_voltage_t without allocating a buffer.
			UartProtocol::getInstance().writeMsgStart(UART_OPCODE_TX_POWER_LOG_VOLTAGE, sizeof(uart_msg_voltage_t));
			UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_VOLTAGE,(uint8_t*)&(rtcCount), sizeof(rtcCount));
			for (int i = power.voltageIndex; i < numSamples * power.numChannels; i += power.numChannels) {
				UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_VOLTAGE,(uint8_t*)&(power.buf[i]), sizeof(nrf_saadc_value_t));
			}
			UartProtocol::getInstance().writeMsgEnd(UART_OPCODE_TX_POWER_LOG_VOLTAGE);
		}
	}
	++printPower;
#endif
}

/**
 * TODO: What does this do?
 */
void PowerSampling::calculateEnergy() {
	uint32_t rtcCount = RTC::getCount();
	uint32_t diffTicks = RTC::difference(rtcCount, _lastEnergyCalculationTicks);
	// TODO: using ms introduces more error (due to rounding to ms), maybe use ticks directly?
//	_energyUsedmicroJoule += (int64_t)_avgPowerMilliWatt * RTC::ticksToMs(diffTicks);
//	_energyUsedmicroJoule += (int64_t)_avgPowerMilliWatt * diffTicks * (NRF_RTC0->PRESCALER + 1) * 1000 / RTC_CLOCK_FREQ;

	// In order to keep more precision: multiply ticks by some number, then divide the result by the same number.
	_energyUsedmicroJoule += (int64_t)_avgPowerMilliWatt * RTC::ticksToMs(1024*diffTicks) / 1024;
	_lastEnergyCalculationTicks = rtcCount;
}

/**
 * TODO: What does this do?
 */
void PowerSampling::checkSoftfuse(int32_t currentRmsMA, int32_t currentRmsFilteredMA) {

	//! Get the current state errors
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, stateErrors.asInt);

	// Get the current switch state before we dispatch any event (as that may change the switch).
	switch_state_t switchState;

	// TODO: implement this differently
	if (RTC::getCount() > RTC::msToTicks(5000)) {
		startIgbtFailureDetection();
	}


	// ---------- TODO: this should be kept up in the state ---------
	switch_state_t prevSwitchState = _lastSwitchState;
	State::getInstance().get(STATE_SWITCH_STATE, &switchState, sizeof(switch_state_t));
	_lastSwitchState = switchState;

	if (switchState.relay_state == 0 && switchState.pwm_state == 0 && (prevSwitchState.relay_state || prevSwitchState.pwm_state)) {
		// switch has been turned off
		_lastSwitchOffTicksValid = true;
		_lastSwitchOffTicks = RTC::getCount();
	}

	bool justSwitchedOff = false;
	if (_lastSwitchOffTicksValid) {
		uint32_t tickDiff = RTC::difference(RTC::getCount(), _lastSwitchOffTicks);
//		write("%u\r\n", tickDiff);
		if (tickDiff < RTC::msToTicks(1000)) {
			justSwitchedOff = true;
		}
		else {
			// Timed out
			_lastSwitchOffTicksValid = false;
		}
	}
	// ---------------------- end of to do --------------------------


	// Check if the filtered Irms is above threshold.
	if ((currentRmsFilteredMA > _currentMilliAmpThreshold) && (!stateErrors.errors.overCurrent)) {
		LOGw("current above threshold");
		EventDispatcher::getInstance().dispatch(EVT_CURRENT_USAGE_ABOVE_THRESHOLD);
		State::getInstance().set(STATE_ERROR_OVER_CURRENT, (uint8_t)1);
		return;
	}

	// When the dimmer is on: check if the filtered Irms is above threshold, or if the unfiltered Irms is way above threshold.
//	if ((currentRmsFilteredMA > _currentMilliAmpThresholdPwm || currentRmsMA > (int32_t)_currentMilliAmpThresholdPwm*5) && (!stateErrors.errors.overCurrentPwm)) {
//	if ((currentRmsFilteredMA > _currentMilliAmpThresholdPwm) && (!stateErrors.errors.overCurrentPwm)) {
	if (currentRmsMA > _currentMilliAmpThresholdPwm) {
		++_consecutivePwmOvercurrent;
//		_logSerial(SERIAL_DEBUG, "mA=%u cnt=%u\r\n", currentRmsMA, _consecutivePwmOvercurrent);
	}
	else {
		_consecutivePwmOvercurrent = 0;
	}
	if ((_consecutivePwmOvercurrent > 20) && (!stateErrors.errors.overCurrentPwm)) {
		// Get the current pwm state before we dispatch the event (as that may change the pwm).
		switch_state_t switchState;
		State::getInstance().get(STATE_SWITCH_STATE, &switchState, sizeof(switch_state_t));
		if (switchState.pwm_state != 0) {
			// If the pwm was on:
			LOGw("current above pwm threshold");
			// Dispatch the event that will turn off the pwm
			EventDispatcher::getInstance().dispatch(EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM);
			// Set overcurrent error.
			State::getInstance().set(STATE_ERROR_OVER_CURRENT_PWM, (uint8_t)1);
		}
		else if (switchState.relay_state == 0 && !justSwitchedOff && _igbtFailureDetectionStarted) {
			// If there is current flowing, but relay and dimmer are both off, then the dimmer is probably broken.
			LOGe("IGBT failure detected");
			EventDispatcher::getInstance().dispatch(EVT_DIMMER_ON_FAILURE_DETECTED);
			State::getInstance().set(STATE_ERROR_DIMMER_ON_FAILURE, (uint8_t)1);
		}
	}
}

/**
 * TODO: What does this do?
 */
void PowerSampling::startIgbtFailureDetection() {
	_igbtFailureDetectionStarted = true;
}

/**
 * TODO: What does this do?
 */
void PowerSampling::toggleVoltageChannelInput() {
	_adcConfig.voltageChannelUsedAs = (_adcConfig.voltageChannelUsedAs + 1) % 5;

	switch (_adcConfig.voltageChannelUsedAs) {
	case 0: // voltage pin
		_adcConfig.voltageChannelPin = _adcConfig.voltagePin;
		break;
	case 1: // zero reference pin
		if (_adcConfig.zeroReferencePin == CS_ADC_REF_PIN_NOT_AVAILABLE) {
			// Skip this pin
			_adcConfig.voltageChannelUsedAs = (_adcConfig.voltageChannelUsedAs + 1) % 5;
			// No break here.
		}
		else {
			_adcConfig.voltageChannelPin = _adcConfig.zeroReferencePin;
			break;
		}
	case 2: // VDD
		_adcConfig.voltageChannelPin = CS_ADC_PIN_VDD;
		break;
	case 3: // Current high gain
		_adcConfig.voltageChannelPin  = _adcConfig.currentPinGainHigh;
		break;
	case 4: // Current low gain
		_adcConfig.voltageChannelPin  = _adcConfig.currentPinGainLow;
		break;
	}

	adc_channel_config_t channelConfig;
	channelConfig.pin = _adcConfig.voltageChannelPin;
	channelConfig.rangeMilliVolt = _adcConfig.rangeMilliVolt[VOLTAGE_CHANNEL_IDX];
	channelConfig.referencePin = _adcConfig.voltageDifferential ? _adcConfig.zeroReferencePin : CS_ADC_REF_PIN_NOT_AVAILABLE;
	_adc->changeChannel(VOLTAGE_CHANNEL_IDX, channelConfig);
}

/**
 * TODO: What does this do?
 */
void PowerSampling::enableDifferentialModeCurrent(bool enable) {
	_adcConfig.currentDifferential = enable;
	adc_channel_config_t channelConfig;
	channelConfig.pin = _adcConfig.currentPinGainMed;
	channelConfig.rangeMilliVolt = _adcConfig.rangeMilliVolt[CURRENT_CHANNEL_IDX];
	channelConfig.referencePin = _adcConfig.currentDifferential ? _adcConfig.zeroReferencePin : CS_ADC_REF_PIN_NOT_AVAILABLE;
	_adc->changeChannel(CURRENT_CHANNEL_IDX, channelConfig);
}

/**
 * TODO: What does this do?
 */
void PowerSampling::enableDifferentialModeVoltage(bool enable) {
	_adcConfig.voltageDifferential = enable;
	adc_channel_config_t channelConfig;
	channelConfig.pin = _adcConfig.voltageChannelPin;
	channelConfig.rangeMilliVolt = _adcConfig.rangeMilliVolt[VOLTAGE_CHANNEL_IDX];
	channelConfig.referencePin = _adcConfig.voltageDifferential ? _adcConfig.zeroReferencePin : CS_ADC_REF_PIN_NOT_AVAILABLE;
	_adc->changeChannel(VOLTAGE_CHANNEL_IDX, channelConfig);
}

/**
 * TODO: What does this do?
 */
void PowerSampling::changeRange(uint8_t channel, int32_t amount) {
	uint16_t newRange = _adcConfig.rangeMilliVolt[channel] + amount;
	switch (_adcConfig.rangeMilliVolt[channel]) {
	case 600:
		if (amount < 0) {
			newRange = 300;
		}
		break;
	case 300:
		if (amount < 0) {
			newRange = 150;
		}
		else {
			newRange = 600;
		}
		break;
	case 150:
		if (amount > 0) {
			newRange = 300;
		}
		break;
	}

	if (newRange < 150 || newRange > 3600) {
		return;
	}
	_adcConfig.rangeMilliVolt[channel] = newRange;

	adc_channel_config_t channelConfig;
	if (channel == VOLTAGE_CHANNEL_IDX) {
		channelConfig.pin = _adcConfig.voltageChannelPin;
		channelConfig.referencePin = _adcConfig.voltageDifferential ? _adcConfig.zeroReferencePin : CS_ADC_REF_PIN_NOT_AVAILABLE;
	}
	else {
		channelConfig.pin = _adcConfig.currentPinGainMed;
		channelConfig.referencePin = _adcConfig.currentDifferential ? _adcConfig.zeroReferencePin : CS_ADC_REF_PIN_NOT_AVAILABLE;
	}
	channelConfig.rangeMilliVolt = _adcConfig.rangeMilliVolt[channel];
	_adc->changeChannel(channel, channelConfig);
}

void PowerSampling::enableSwitchcraft(bool enable) {
	if (enable) {
		RecognizeSwitch::getInstance().start();
	}
	else {
		RecognizeSwitch::getInstance().stop();
	}
}
