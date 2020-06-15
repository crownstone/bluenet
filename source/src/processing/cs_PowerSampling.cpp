/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 19, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_PowerSampling.h"

#include "common/cs_Types.h"
#include "drivers/cs_RTC.h"
#include "drivers/cs_Serial.h"
#include "events/cs_EventDispatcher.h"
#include "processing/cs_RecognizeSwitch.h"
#include "protocol/cs_UartMsgTypes.h"
#include "protocol/cs_UartProtocol.h"
#include "protocol/cs_Packets.h"
#include "storage/cs_State.h"
#include "structs/buffer/cs_InterleavedBuffer.h"
#include "third/SortMedian.h"
#include "third/optmed.h"
#include "time/cs_SystemTime.h"

#include <cmath>

// Define test pin to enable gpio debug.
//#define TEST_PIN 20

// Define to print power samples
#define PRINT_POWER_SAMPLES

#define VOLTAGE_CHANNEL_IDX 0
#define CURRENT_CHANNEL_IDX 1

PowerSampling::PowerSampling() :
		_isInitialized(false),
		_adc(NULL),
		_bufferQueue(CS_ADC_NUM_BUFFERS),
		_consecutivePwmOvercurrent(0),
		_lastEnergyCalculationTicks(0),
		_energyUsedmicroJoule(0),
		_lastSwitchOffTicks(0),
		_lastSwitchOffTicksValid(false),
		_igbtFailureDetectionStarted(false)
{
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
void adc_done_callback(buffer_id_t bufIndex) {
	PowerSampling::getInstance().powerSampleAdcDone(bufIndex);
}

void PowerSampling::init(const boards_config_t& boardConfig) {
	State& settings = State::getInstance();
	settings.get(CS_TYPE::CONFIG_VOLTAGE_MULTIPLIER, &_voltageMultiplier, sizeof(_voltageMultiplier));
	settings.get(CS_TYPE::CONFIG_CURRENT_MULTIPLIER, &_currentMultiplier, sizeof(_currentMultiplier));
	settings.get(CS_TYPE::CONFIG_VOLTAGE_ADC_ZERO, &_voltageZero, sizeof(_voltageZero));
	settings.get(CS_TYPE::CONFIG_CURRENT_ADC_ZERO, &_currentZero, sizeof(_currentZero));
	settings.get(CS_TYPE::CONFIG_POWER_ZERO, &_powerZero, sizeof(_powerZero));
	settings.get(CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD, &_currentMilliAmpThreshold, sizeof(_currentMilliAmpThreshold));
	settings.get(CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM, &_currentMilliAmpThresholdPwm, sizeof(_currentMilliAmpThresholdPwm));
	bool switchcraftEnabled = settings.isTrue(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED);

	RecognizeSwitch::getInstance().init();
	TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD) switchcraftThreshold;
	settings.get(CS_TYPE::CONFIG_SWITCHCRAFT_THRESHOLD, &switchcraftThreshold, sizeof(switchcraftThreshold));
	RecognizeSwitch::getInstance().configure(switchcraftThreshold);
	enableSwitchcraft(switchcraftEnabled);

	_bufferQueue.init();

	initAverages();
	_recalibrateZeroVoltage = true;
	_recalibrateZeroCurrent = true;
//	_zeroVoltageInitialized = false;
//	_zeroCurrentInitialized = false;
	_zeroVoltageCount = 0;
	_zeroCurrentCount = 0;
	_avgZeroVoltageDiscount = VOLTAGE_ZERO_EXP_AVG_DISCOUNT;
	_avgZeroCurrentDiscount = CURRENT_ZERO_EXP_AVG_DISCOUNT;
	_avgPowerDiscount = POWER_EXP_AVG_DISCOUNT;
	_boardPowerZero = boardConfig.powerZero;

	LOGi(FMT_INIT, "buffers");
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

	TYPIFY(STATE_OPERATION_MODE) mode;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	_operationMode = getOperationMode(mode);

	_adc->start();
}

void PowerSampling::enableZeroCrossingInterrupt(ps_zero_crossing_cb_t callback) {
	_adc->setZeroCrossingCallback(callback);
	// Simply use the zero from the board config, that should be accurate enough for this purpose.
	_adc->enableZeroCrossingInterrupt(VOLTAGE_CHANNEL_IDX, _voltageZero);
}

void PowerSampling::handleEvent(event_t & event) {
	switch(event.type) {
	case CS_TYPE::CMD_ENABLE_LOG_POWER:
		_logsEnabled.flags.power = *(TYPIFY(CMD_ENABLE_LOG_POWER)*)event.data;
		break;
	case CS_TYPE::CMD_ENABLE_LOG_CURRENT:
		_logsEnabled.flags.current = *(TYPIFY(CMD_ENABLE_LOG_CURRENT)*)event.data;
		break;
	case CS_TYPE::CMD_ENABLE_LOG_VOLTAGE:
		_logsEnabled.flags.voltage = *(TYPIFY(CMD_ENABLE_LOG_VOLTAGE)*)event.data;
		break;
	case CS_TYPE::CMD_ENABLE_LOG_FILTERED_CURRENT:
		_logsEnabled.flags.filteredCurrent = *(TYPIFY(CMD_ENABLE_LOG_FILTERED_CURRENT)*)event.data;
		break;
	case CS_TYPE::CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN:
		toggleVoltageChannelInput();
		break;
	case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT:
		enableDifferentialModeCurrent(*(TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT)*)event.data);
		break;
	case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE:
		enableDifferentialModeVoltage(*(TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE)*)event.data);
		break;
	case CS_TYPE::CMD_INC_VOLTAGE_RANGE:
		changeRange(VOLTAGE_CHANNEL_IDX, 600);
		break;
	case CS_TYPE::CMD_DEC_VOLTAGE_RANGE:
		changeRange(VOLTAGE_CHANNEL_IDX, -600);
		break;
	case CS_TYPE::CMD_INC_CURRENT_RANGE:
		changeRange(CURRENT_CHANNEL_IDX, 600);
		break;
	case CS_TYPE::CMD_DEC_CURRENT_RANGE:
		changeRange(CURRENT_CHANNEL_IDX, -600);
		break;
	case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED:
		enableSwitchcraft(*(TYPIFY(CONFIG_SWITCHCRAFT_ENABLED)*)event.data);
		break;
	case CS_TYPE::CMD_GET_POWER_SAMPLES: {
		cs_power_samples_request_t* cmd = (TYPIFY(CMD_GET_POWER_SAMPLES)*)event.data;
		handleGetPowerSamples((PowerSamplesType)cmd->type, cmd->index, event.result);
		break;
	}
	case CS_TYPE::CMD_GET_ADC_RESTARTS: {
		if (event.result.buf.len < sizeof(_adcRestarts)) {
			event.result.returnCode = ERR_BUFFER_TOO_SMALL;
			return;
		}
		memcpy(event.result.buf.data, &_adcRestarts, sizeof(_adcRestarts));
		event.result.dataSize = sizeof(_adcRestarts);
		event.result.returnCode = ERR_SUCCESS;
		break;
	}
	case CS_TYPE::EVT_ADC_RESTARTED:
		_adcRestarts.count++;
		_adcRestarts.lastTimestamp = SystemTime::posix();
		_skipSwapDetection = 1;
		while (!_bufferQueue.empty()) {
			ADC::getInstance().releaseBuffer(_bufferQueue.pop());
		}
		UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_ADC_RESTART, NULL, 0);
		RecognizeSwitch::getInstance().skip(2);
		break;
	case CS_TYPE::CONFIG_SWITCHCRAFT_THRESHOLD:
		RecognizeSwitch::getInstance().configure(*(TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD)*)event.data);
		break;
	case CS_TYPE::EVT_TICK:
		if (_calibratePowerZeroCountDown) {
			--_calibratePowerZeroCountDown;
		}
		break;
	default: {}
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
void PowerSampling::powerSampleAdcDone(buffer_id_t bufIndex) {
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	buffer_id_t filteredBufIndex;
	if (_bufferQueue.empty()) {
		// Filter current buffer to current buffer.
		filteredBufIndex = bufIndex;
	}
	else {
		// Filter current buffer to last buffer in queue (which is the last unfiltered buffer).
		filteredBufIndex = _bufferQueue[_bufferQueue.size() - 1];
	}
	LOGnone("filter %u to %u", bufIndex, filteredBufIndex);
	_lastBufIndex = bufIndex;
	_lastFilteredBufIndex = filteredBufIndex;

	_bufferQueue.push(bufIndex);

	power_t power;
	power.buf = InterleavedBuffer::getInstance().getBuffer(filteredBufIndex);
	power.bufSize = CS_ADC_BUF_SIZE;
	power.voltageIndex = VOLTAGE_CHANNEL_IDX;
	power.currentIndex = CURRENT_CHANNEL_IDX;
	power.numChannels = 2;
	power.sampleIntervalUs = CS_ADC_SAMPLE_INTERVAL_US;
	power.acPeriodUs = 20000;

	// Filter current buffer to the previous unfiltered buffer.
	filter(bufIndex, filteredBufIndex, power.voltageIndex);
	filter(bufIndex, filteredBufIndex, power.currentIndex);


	if (_bufferQueue.size() >= 3) {
		buffer_id_t prevIndex = _bufferQueue[_bufferQueue.size() - 3]; // Previous filtered buffer.
		sample_value_t* prevBuf = InterleavedBuffer::getInstance().getBuffer(prevIndex);
		if (isVoltageAndCurrentSwapped(power, prevBuf)) {
			LOGw("Swap detected: restart ADC.");
			_adc->stop();
			printBuf(power);
			_adc->releaseBuffer(bufIndex);
			_adc->start();
			return;
		}
	}

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

	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {
		State::getInstance().set(CS_TYPE::STATE_POWER_USAGE, &_avgPowerMilliWatt, sizeof(_avgPowerMilliWatt));
		State::getInstance().set(CS_TYPE::STATE_ACCUMULATED_ENERGY, &_energyUsedmicroJoule, sizeof(_energyUsedmicroJoule));
	}

#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	bool switch_detected = RecognizeSwitch::getInstance().detect(_bufferQueue, power.voltageIndex);
	if (switch_detected) {
		LOGd("Switch event detected!");
		event_t event(CS_TYPE::CMD_SWITCH_TOGGLE, nullptr, 0, cmd_source_t(CS_CMD_SOURCE_SWITCHCRAFT));
		EventDispatcher::getInstance().dispatch(event);
	}

	// We want to keep 4 buffers: 1 unfiltered, 3 filtered.
	if (_bufferQueue.size() > 4) {
		buffer_id_t bufIndexToRelease = _bufferQueue.pop();
		_adc->releaseBuffer(bufIndexToRelease);
	}
}

void PowerSampling::initAverages() {
	_avgZeroVoltage = _voltageZero * 1024;
	_avgZeroCurrent = _currentZero * 1024;
	_avgPower = 0.0;
}

/**
 * This just returns the given currentIndex.
 */
uint16_t PowerSampling::determineCurrentIndex(power_t & power) {
	return power.currentIndex;
}

/*
 * Other idea:
 * - compare Vrms[t-1] with Vrms[t] and Irms[t]. If Vrms[t-1] is more similar to Irms[t] than to Vrms[t], then swapped.
 * Problems:
 * - Vzero and Izero can be different and affect Vrm and Irms.
 */
bool PowerSampling::isVoltageAndCurrentSwapped(power_t & power, sample_value_t* prevBuf) {
	// Problem: switchcraft makes this function falsely detect a swap.
	return false;
	if (_skipSwapDetection != 0) {
		_skipSwapDetection--;
		return false;
	}
	int16_t prev, sameIndex, differentIndex;

	uint32_t sumSameIndex = 0;
	uint32_t sumDifferentIndex = 0;

	int16_t maxPrev =           INT16_MIN;
	int16_t maxSameIndex =      INT16_MIN;
	int16_t maxDifferentIndex = INT16_MIN;

	int16_t minPrev =           INT16_MAX;
	int16_t minSameIndex =      INT16_MAX;
	int16_t minDifferentIndex = INT16_MAX;

	for (int i = 0; i < power.bufSize / power.numChannels; i += power.numChannels) {
		prev =             prevBuf[power.voltageIndex + i];
		sameIndex =      power.buf[power.voltageIndex + i];
		differentIndex = power.buf[power.currentIndex + i];
//		_log(SERIAL_DEBUG, "%i %i %i\r\n", prev, sameIndex, differentIndex);

		sumSameIndex +=      std::abs(sameIndex - prev);
		sumDifferentIndex += std::abs(differentIndex - prev);

		if (prev > maxPrev) { maxPrev = prev; }
		if (prev < minPrev) { minPrev = prev; }
		if (sameIndex > maxSameIndex) { maxSameIndex = sameIndex; }
		if (sameIndex < minSameIndex) { minSameIndex = sameIndex; }
		if (differentIndex > maxDifferentIndex) { maxDifferentIndex = differentIndex; }
		if (differentIndex < minDifferentIndex) { minDifferentIndex = differentIndex; }
	}

	int16_t minMaxSameIndex = std::abs(maxPrev - maxSameIndex) + std::abs(minPrev - minSameIndex);
	int16_t minMaxDifferentIndex = std::abs(maxPrev - maxDifferentIndex) + std::abs(minPrev - minDifferentIndex);
	if (minMaxSameIndex > minMaxDifferentIndex) {
		LOGd("minMaxSameIndex=%i minMaxDifferentIndex=%i", minMaxSameIndex, minMaxDifferentIndex);
		LOGd("prev=[%i %i] same=[%i %i] different=[%i %i]", minPrev, maxPrev, minSameIndex, maxSameIndex, minDifferentIndex, maxDifferentIndex);
	}

//	if (sumSameIndex > sumDifferentIndex || sumSameIndex > 1000) {
	if (sumSameIndex > sumDifferentIndex) {
		LOGd("sumSameIndex=%u sumDifferentIndex=%u", sumSameIndex, sumDifferentIndex);
	}
	return (sumDifferentIndex < sumSameIndex);
//	return (minMaxSameIndex > minMaxDifferentIndex && sumSameIndex > sumDifferentIndex);
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
void PowerSampling::calculateVoltageZero(power_t & power) {
	uint16_t numSamples = power.acPeriodUs / power.sampleIntervalUs;

	int64_t sum = 0;
	for (int i = power.voltageIndex; i < numSamples * power.numChannels; i += power.numChannels) {
		sum += power.buf[i];
	}
	int32_t zeroVoltage = sum * 1024 / numSamples;

//	if (!_zeroVoltageInitialized) {
//		_avgZeroVoltage = zeroVoltage;
//		_zeroVoltageInitialized = true;
//	}
	if (!_zeroVoltageCount) {
		_avgZeroVoltage = zeroVoltage;
	}
	else {
		// Exponential moving average
		int64_t avgZeroVoltageDiscount = _avgZeroVoltageDiscount; // Make sure calculations are in int64_t
		_avgZeroVoltage = ((1000 - avgZeroVoltageDiscount) * _avgZeroVoltage + avgZeroVoltageDiscount * zeroVoltage) / 1000;
	}
	if (_zeroVoltageCount < 65535) {
		++_zeroVoltageCount;
	}
}

/**
 * The same as for the voltage curve, but for the current.
 */
void PowerSampling::calculateCurrentZero(power_t & power) {
	uint16_t numSamples = power.acPeriodUs / power.sampleIntervalUs;

	int64_t sum = 0;
	// Use filtered samples to calculate the zero.
	for (int i = 0; i < numSamples; ++i) {
		sum += _outputSamples->at(i);
	}
	int32_t zeroCurrent = sum * 1024 / numSamples;

//	if (!_zeroCurrentInitialized) {
//		_avgZeroCurrent = zeroCurrent;
//		_zeroCurrentInitialized = true;
//	}
	if (!_zeroCurrentCount) {
		_avgZeroCurrent = zeroCurrent;
	}
	else {
		// Exponential moving average
		int64_t avgZeroCurrentDiscount = _avgZeroCurrentDiscount; // Make sure calculations are in int64_t
		_avgZeroCurrent = ((1000 - avgZeroCurrentDiscount) * _avgZeroCurrent + avgZeroCurrentDiscount * zeroCurrent) / 1000;
	}
	if (_zeroCurrentCount < 65535) {
		++_zeroCurrentCount;
	}
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
void PowerSampling::filter(buffer_id_t bufIndexIn, buffer_id_t bufIndexOut, channel_id_t channel_id) {

	// Pad the start of the input vector with the first sample in the buffer
	uint16_t j = 0;
	sample_value_t padded_value = InterleavedBuffer::getInstance().getValue(bufIndexIn, channel_id, 0);
	for (uint16_t i = 0; i < _filterParams->half; ++i, ++j) {
		_inputSamples->at(j) = padded_value;
	}
	// Copy samples from buffer to input vector
	uint8_t channel_length = InterleavedBuffer::getInstance().getChannelLength();
	for (int i = 0; i < channel_length - 1; ++i, ++j) {
		_inputSamples->at(j) = InterleavedBuffer::getInstance().getValue(bufIndexIn, channel_id, i);
	}
	// Pad the end of the buffer with the last sample in the buffer
	padded_value = InterleavedBuffer::getInstance().getValue(bufIndexIn, channel_id, channel_length - 1);
	for (uint16_t i = 0; i < _filterParams->half; ++i, ++j) {
		_inputSamples->at(j) = padded_value;
	}

	// Filter the data
	sort_median(*_filterParams, *_inputSamples, *_outputSamples);

	// Copy the result back into the buffer
	for (int i = 0; i < InterleavedBuffer::getInstance().getChannelLength() - 1; ++i) {
		InterleavedBuffer::getInstance().setValue(bufIndexOut, channel_id, i, _outputSamples->at(i));
	}
}

/**
 * Calculate power.
 *
 * The int64_t sum is large enough: 2^63 / (2^12 * 1000 * 2^12 * 1000) = 5*10^5. Many more samples than the 100 we use.
 */
void PowerSampling::calculatePower(power_t & power) {

	uint16_t numSamples = power.acPeriodUs / power.sampleIntervalUs;

	// TODO: Make this an assert.
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
		current = (int64_t)power.buf[i+power.currentIndex]*1024 - _avgZeroCurrent;
		voltage = (int64_t)power.buf[i+power.voltageIndex]*1024 - _avgZeroVoltage;
		cSquareSum += (current * current) / (1024*1024);
		vSquareSum += (voltage * voltage) / (1024*1024);
		pSum +=       (current * voltage) / (1024*1024);
	}
	int32_t powerMilliWattReal = pSum * _currentMultiplier * _voltageMultiplier * 1000 / numSamples;
	int32_t currentRmsMA = sqrt((double)cSquareSum * _currentMultiplier * _currentMultiplier / numSamples) * 1000;
	int32_t voltageRmsMilliVolt = sqrt((double)vSquareSum * _voltageMultiplier * _voltageMultiplier / numSamples) * 1000;



	////////////////////////////////////////////////////////////////////////////////
	// Calculate Irms of median filtered samples, and filter over multiple periods
	////////////////////////////////////////////////////////////////////////////////

//	// Calculate Irms again, but now with the filtered current samples
//	cSquareSum = 0;
//	for (uint16_t i=0; i<numSamples; ++i) {
//		current = (int64_t)_outputSamples->at(i)*1000 - _avgZeroCurrent;
//		cSquareSum += (current * current) / (1000*1000);
//	}
//	int32_t filteredCurrentRmsMA =  sqrt((double)cSquareSum * _currentMultiplier * _currentMultiplier / numSamples) * 1000;

	// power.buf is already median filtered
	int32_t filteredCurrentRmsMA = currentRmsMA;

//	if (isVoltageAndCurrentSwapped(filteredCurrentRmsMA, voltageRmsMilliVolt)) {
//		LOGw("Swap detected: restart ADC.");
//		_adc->stop();
//		_adc->start();
//		return;
//	}

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
			currentRmsSumMA += _filteredCurrentRmsHistMA->operator[](i);
		}
		filteredCurrentRmsMedianMA = currentRmsSumMA / _filteredCurrentRmsHistMA->size();
	}

	// Now that Irms is known: first check the soft fuse.
//	if (_zeroCurrentInitialized && _zeroVoltageInitialized) {
	if (_zeroVoltageCount > 200 && _zeroCurrentCount > 200) { // Wait some time, for the measurement to converge.. why does this have to take so long?
		checkSoftfuse(filteredCurrentRmsMedianMA, filteredCurrentRmsMedianMA, voltageRmsMilliVolt, power);
	}



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
			currentRmsMilliAmpSum += _currentRmsMilliAmpHist->operator[](i);
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
			voltageRmsMilliVoltSum += _voltageRmsMilliVoltHist->operator[](i);
		}
		_avgVoltageRmsMilliVolt = voltageRmsMilliVoltSum / _voltageRmsMilliVoltHist->size();
	}

	// Calculate apparent power: current_rms * voltage_rms
	__attribute__((unused)) uint32_t powerMilliWattApparent = (int64_t)_avgCurrentRmsMilliAmp * _avgVoltageRmsMilliVolt / 1000;

	if (_powerZero == CONFIG_POWER_ZERO_INVALID) {
		calibratePowerZero(_avgPowerMilliWatt);
	}
	else {
		powerMilliWattReal -= _powerZero;
	}

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
//	if (printPower % 10 == 0) {
//		LOGd("I=%u %u %u %u P=%i %i %i", currentRmsMA, currentRmsMedianMA, filteredCurrentRmsMA, filteredCurrentRmsMedianMA, _avgPowerMilliWatt, powerMilliWattReal, powerMilliWattApparent);
//	}

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
				UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_CURRENT, (uint8_t*)&(power.buf[i]), sizeof(sample_value_t));
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
				UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_VOLTAGE,(uint8_t*)&(power.buf[i]), sizeof(sample_value_t));
			}
			UartProtocol::getInstance().writeMsgEnd(UART_OPCODE_TX_POWER_LOG_VOLTAGE);
		}
	}
	++printPower;
#endif
}

void PowerSampling::calibratePowerZero(int32_t powerMilliWatt) {
	TYPIFY(STATE_SWITCH_STATE) switchState;
	State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));
	if (_calibratePowerZeroCountDown != 0 || switchState.asInt != 0) {
		return;
	}
	if (powerMilliWatt < (_boardPowerZero - 10000) || powerMilliWatt > (_boardPowerZero + 10000)) {
		// Measured power without load shouldn't be more than 10W different from the board default.
		// It might be a dimmer on failure instead.
		return;
	}
	LOGi("Power zero calibrated: %i", powerMilliWatt);
	_powerZero = powerMilliWatt;
	State::getInstance().set(CS_TYPE::CONFIG_POWER_ZERO, &_powerZero, sizeof(_powerZero));
}

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

void PowerSampling::checkSoftfuse(int32_t currentRmsMA, int32_t currentRmsFilteredMA, int32_t voltageRmsMilliVolt, power_t & power) {

	// Get the current state errors
	TYPIFY(STATE_ERRORS) stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors.asInt, sizeof(stateErrors));

	// TODO: implement this differently
	if (!_igbtFailureDetectionStarted && (RTC::getCount() > RTC::msToTicks(5000))) {
		startIgbtFailureDetection();
		LOGi("startIgbtFailureDetection");
		printBuf(power);
	}

	// TODO Bart 2019-12-12 Use event handler to check for switch changes. Don't poll, as this uses processing power for nothing.
	switch_state_t prevSwitchState = _lastSwitchState;
	// Get the current switch state before we dispatch any event (as that may change the switch).
	TYPIFY(STATE_SWITCH_STATE) switchState;
	State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));
	_lastSwitchState = switchState;

	if (switchState.state.relay == 0 && switchState.state.dimmer == 0 && (prevSwitchState.state.relay || prevSwitchState.state.dimmer)) {
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

	// Check if data makes sense: RMS voltage should be about 230V.
	if (voltageRmsMilliVolt != 0 && (voltageRmsMilliVolt < 200*1000 || 250*1000 < voltageRmsMilliVolt)) {
		LOGw("voltageRmsMilliVolt=%u", voltageRmsMilliVolt);
		return;
	}

	// Check if the filtered Irms is above threshold.
	if ((currentRmsFilteredMA > _currentMilliAmpThreshold) && (!stateErrors.errors.overCurrent)) {
		LOGw("Overcurrent: Irms=%i mA V=[%i %i ..] C=[%i %i ..]",
				currentRmsFilteredMA,
				power.buf[power.voltageIndex],
				power.buf[power.voltageIndex + power.numChannels],
				power.buf[power.currentIndex],
				power.buf[power.currentIndex + power.numChannels]);
		printBuf(power);

		event_t event(CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD);
		EventDispatcher::getInstance().dispatch(event);
		stateErrors.errors.overCurrent = true;
		State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
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
	if ((_consecutivePwmOvercurrent > 20) && (!stateErrors.errors.overCurrentDimmer)) {
//		// Get the current pwm state before we dispatch the event (as that may change the pwm).
//		TYPIFY(STATE_SWITCH_STATE) switchState;
//		State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState);
		if (switchState.state.dimmer != 0) {
			// If the pwm was on:
			LOGw("Dimmer overcurrent: Irms=%i mA V=[%i %i ..] C=[%i %i ..]",
				currentRmsMA,
				power.buf[power.voltageIndex],
				power.buf[power.voltageIndex + power.numChannels],
				power.buf[power.currentIndex],
				power.buf[power.currentIndex + power.numChannels]);
			printBuf(power);
			// Dispatch the event that will turn off the pwm
			event_t event(CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER);
			EventDispatcher::getInstance().dispatch(event);

			stateErrors.errors.overCurrentDimmer = true;
			State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
		}
		else if (switchState.state.relay == 0 && !justSwitchedOff && _igbtFailureDetectionStarted) {
			// If there is current flowing, but relay and dimmer are both off, then the dimmer is probably broken.
			LOGw("Dimmer failure detected: Irms=%i mA V=[%i %i ..] C=[%i %i ..]",
				currentRmsMA,
				power.buf[power.voltageIndex],
				power.buf[power.voltageIndex + power.numChannels],
				power.buf[power.currentIndex],
				power.buf[power.currentIndex + power.numChannels]);
			printBuf(power);
			event_t event(CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED);
			EventDispatcher::getInstance().dispatch(event);

			stateErrors.errors.dimmerOn = true;
			State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
		}
	}
}

void PowerSampling::startIgbtFailureDetection() {
	_igbtFailureDetectionStarted = true;
}

void PowerSampling::handleGetPowerSamples(PowerSamplesType type, uint8_t index, cs_result_t& result) {
	LOGi("handleGetPowerSamples type=%u index=%u", type, index);
	switch (type) {
		case POWER_SAMPLES_TYPE_SWITCHCRAFT:
		case POWER_SAMPLES_TYPE_SWITCHCRAFT_NON_TRIGGERED:
			RecognizeSwitch::getInstance().getLastDetection(type, index, result);
			break;
		case POWER_SAMPLES_TYPE_NOW_FILTERED:
		case POWER_SAMPLES_TYPE_NOW_UNFILTERED: {
			// Check index
			if (index >= 2) {
				LOGw("index=%u", index);
				result.returnCode = ERR_WRONG_PARAMETER;
				return;
			}

			// Check size
			cs_power_samples_header_t* header = (cs_power_samples_header_t*)result.buf.data;
			uint16_t numSamples = InterleavedBuffer::getInstance().getChannelLength();
			uint16_t samplesSize = numSamples * sizeof(sample_value_t);
			size16_t requiredSize = sizeof(*header) + samplesSize;
			if (result.buf.len < requiredSize) {
				LOGw("size=%u required=%u", result.buf.len, requiredSize);
				result.returnCode = ERR_BUFFER_TOO_SMALL;
				return;
			}

			// Set header fields.
			header->type = type;
			header->index = index;
			header->count = numSamples;
			header->unixTimestamp = SystemTime::posix();
			header->delayUs = 0;
			header->sampleIntervalUs = CS_ADC_SAMPLE_INTERVAL_US;
			if (index == VOLTAGE_CHANNEL_IDX) {
				header->offset = _avgZeroVoltage / 1024;
				header->multiplier = _voltageMultiplier;
			}
			else {
				header->offset = _avgZeroCurrent / 1024;
				header->multiplier = _currentMultiplier;
			}

			// Copy samples
			buffer_id_t bufIndex = (type == POWER_SAMPLES_TYPE_NOW_FILTERED) ? _lastFilteredBufIndex : _lastBufIndex;
			sample_value_t* samples = (sample_value_t*)(result.buf.data + sizeof(*header));
			for (sample_value_id_t i = 0; i < numSamples; ++i) {
				samples[i] = InterleavedBuffer::getInstance().getValue(bufIndex, index, i);
			}

			result.dataSize = requiredSize;
			result.returnCode = ERR_SUCCESS;
			break;
		}
		default:
			break;
	}
}

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

void PowerSampling::enableDifferentialModeCurrent(bool enable) {
	_adcConfig.currentDifferential = enable;
	adc_channel_config_t channelConfig;
	channelConfig.pin = _adcConfig.currentPinGainMed;
	channelConfig.rangeMilliVolt = _adcConfig.rangeMilliVolt[CURRENT_CHANNEL_IDX];
	channelConfig.referencePin = _adcConfig.currentDifferential ? _adcConfig.zeroReferencePin : CS_ADC_REF_PIN_NOT_AVAILABLE;
	_adc->changeChannel(CURRENT_CHANNEL_IDX, channelConfig);
}

void PowerSampling::enableDifferentialModeVoltage(bool enable) {
	_adcConfig.voltageDifferential = enable;
	adc_channel_config_t channelConfig;
	channelConfig.pin = _adcConfig.voltageChannelPin;
	channelConfig.rangeMilliVolt = _adcConfig.rangeMilliVolt[VOLTAGE_CHANNEL_IDX];
	channelConfig.referencePin = _adcConfig.voltageDifferential ? _adcConfig.zeroReferencePin : CS_ADC_REF_PIN_NOT_AVAILABLE;
	_adc->changeChannel(VOLTAGE_CHANNEL_IDX, channelConfig);
}

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

void PowerSampling::printBuf(power_t & power) {
	LOGd("PowerBuf:");
	for (uint16_t i = 0; i < power.bufSize; ++i) {
		_log(SERIAL_DEBUG, " %i", power.buf[i]);
		if ((i+1) % 10 == 0) {
			_log(SERIAL_DEBUG, SERIAL_CRLF);
		}
	}
	_log(SERIAL_DEBUG, SERIAL_CRLF);
}
