/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 19, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_PowerSampling.h"

#include <logging/cs_Logger.h>

#include <cmath>

#include "common/cs_Types.h"
#include "drivers/cs_RTC.h"
#include "events/cs_EventDispatcher.h"
#include "processing/cs_RecognizeSwitch.h"
#include "protocol/cs_Packets.h"
#include "protocol/cs_UartMsgTypes.h"
#include "storage/cs_State.h"
#include "structs/buffer/cs_AdcBuffer.h"
#include "third/SortMedian.h"
#include "third/optmed.h"
#include "time/cs_SystemTime.h"
#include "uart/cs_UartHandler.h"

#define LOGPowerSamplingWarn LOGw
#define LOGPowerSamplingDebug LOGnone
#define LOGPowerSamplingVerbose LOGnone

// Define test pin to enable gpio debug.
//#define PS_TEST_PIN 20

// Define to print power samples
//#define PRINT_POWER_SAMPLES

#define VOLTAGE_CHANNEL_IDX 0
#define CURRENT_CHANNEL_IDX 1
#define AC_PERIOD_US 20000

#ifdef PS_TEST_PIN
#ifdef DEBUG
#pragma message("Power sampling test pin enabled")
#else
#warning "Power sampling test pin enabled"
#endif
#define PS_TEST_PIN_INIT nrf_gpio_cfg_output(PS_TEST_PIN);
#define PS_TEST_PIN_TOGGLE nrf_gpio_pin_toggle(PS_TEST_PIN);
#else
#define PS_TEST_PIN_INIT
#define PS_TEST_PIN_TOGGLE
#endif

PowerSampling::PowerSampling() : _bufferQueue(CS_ADC_NUM_BUFFERS), _switchHist(switchHistSize) {
	_adc                      = &(ADC::getInstance());
	_powerMilliWattHist       = new CircularBuffer<int32_t>(POWER_SAMPLING_RMS_WINDOW_SIZE);
	_currentRmsMilliAmpHist   = new CircularBuffer<int32_t>(POWER_SAMPLING_RMS_WINDOW_SIZE);
	_voltageRmsMilliVoltHist  = new CircularBuffer<int32_t>(POWER_SAMPLING_RMS_WINDOW_SIZE);
	_filteredCurrentRmsHistMA = new CircularBuffer<int32_t>(POWER_SAMPLING_RMS_WINDOW_SIZE);
	_logsEnabled.asInt        = 0;
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
void adc_done_callback(adc_buffer_id_t bufIndex) {
	PowerSampling::getInstance().powerSampleAdcDone(bufIndex);
}

void PowerSampling::init(const boards_config_t* boardConfig) {
	State& settings = State::getInstance();
	settings.get(CS_TYPE::CONFIG_VOLTAGE_MULTIPLIER, &_voltageMultiplier, sizeof(_voltageMultiplier));
	settings.get(CS_TYPE::CONFIG_CURRENT_MULTIPLIER, &_currentMultiplier, sizeof(_currentMultiplier));
	settings.get(CS_TYPE::CONFIG_VOLTAGE_ADC_ZERO, &_voltageZero, sizeof(_voltageZero));
	settings.get(CS_TYPE::CONFIG_CURRENT_ADC_ZERO, &_currentZero, sizeof(_currentZero));
	settings.get(CS_TYPE::CONFIG_POWER_ZERO, &_powerZero, sizeof(_powerZero));
	settings.get(
			CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD, &_currentMilliAmpThreshold, sizeof(_currentMilliAmpThreshold));
	settings.get(
			CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_DIMMER,
			&_currentMilliAmpThresholdDimmer,
			sizeof(_currentMilliAmpThresholdDimmer));
	bool switchcraftEnabled = settings.isTrue(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED);

	if (boardConfig->flags.hasAccuratePowerMeasurement) {
		_powerDiffThresholdPart          = POWER_DIFF_THRESHOLD_PART;
		_powerDiffThresholdMinMilliWatt  = POWER_DIFF_THRESHOLD_MIN_WATT * 1000.0f;
		_negativePowerThresholdMilliWatt = NEGATIVE_POWER_THRESHOLD_WATT * 1000.0f;
	}
	else {
		_powerDiffThresholdPart          = POWER_DIFF_THRESHOLD_PART_CS_ZERO;
		_powerDiffThresholdMinMilliWatt  = POWER_DIFF_THRESHOLD_MIN_WATT_CS_ZERO * 1000.0f;
		_negativePowerThresholdMilliWatt = NEGATIVE_POWER_THRESHOLD_WATT_CS_ZERO * 1000.0f;
	}
	LOGd("_powerDiffThresholdPart=%i _powerDiffThresholdMinMilliWatt=%i _negativePowerThresholdMilliWatt=%i",
		 (int32_t)_powerDiffThresholdPart,
		 (int32_t)_powerDiffThresholdMinMilliWatt,
		 (int32_t)_negativePowerThresholdMilliWatt);

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
	_zeroVoltageCount       = 0;
	_zeroCurrentCount       = 0;
	_avgZeroVoltageDiscount = VOLTAGE_ZERO_EXP_AVG_DISCOUNT;
	_avgZeroCurrentDiscount = CURRENT_ZERO_EXP_AVG_DISCOUNT;
	_avgPowerDiscount       = POWER_EXP_AVG_DISCOUNT;
	_boardPowerZero         = boardConfig->powerOffsetMilliWatt;

	LOGi(FMT_INIT "buffers");
	_powerMilliWattHist->init();        // Allocates buffer
	_currentRmsMilliAmpHist->init();    // Allocates buffer
	_voltageRmsMilliVoltHist->init();   // Allocates buffer
	_filteredCurrentRmsHistMA->init();  // Allocates buffer
	_switchHist.init();                 // Allocates buffer

	// Init moving median filter
	unsigned halfWindowSize = POWER_SAMPLING_CURVE_HALF_WINDOW_SIZE;
	//	unsigned halfWindowSize = 5;  // Takes 0.74ms
	//	unsigned halfWindowSize = 16; // Takes 0.93ms
	unsigned windowSize     = halfWindowSize * 2 + 1;
	uint16_t bufSize        = AdcBuffer::getInstance().getChannelLength();
	unsigned blockCount     = (bufSize + halfWindowSize * 2) / windowSize;  // Shouldn't have a remainder!
	_filterParams           = new MedianFilter(halfWindowSize, blockCount);
	_inputSamples           = new PowerVector(bufSize + halfWindowSize * 2);
	_outputSamples          = new PowerVector(bufSize);

	_boardConfig            = boardConfig;

	LOGd(FMT_INIT "ADC");
	adc_config_t adcConfig;
	adcConfig.channelCount                                 = 2;
	// TODO: there are now multiple voltage pins
	adcConfig.channels[VOLTAGE_CHANNEL_IDX].pin            = boardConfig->pinAinVoltage[GAIN_SINGLE];
	adcConfig.channels[VOLTAGE_CHANNEL_IDX].rangeMilliVolt = boardConfig->voltageAdcRangeMilliVolt;
	adcConfig.channels[VOLTAGE_CHANNEL_IDX].referencePin =
			boardConfig->pinAinZeroRef != PIN_NONE ? boardConfig->pinAinZeroRef : CS_ADC_REF_PIN_NOT_AVAILABLE;
	adcConfig.channels[CURRENT_CHANNEL_IDX].pin            = boardConfig->pinAinCurrent[GAIN_SINGLE];
	adcConfig.channels[CURRENT_CHANNEL_IDX].rangeMilliVolt = boardConfig->currentAdcRangeMilliVolt;
	adcConfig.channels[CURRENT_CHANNEL_IDX].referencePin =
			boardConfig->pinAinZeroRef != PIN_NONE ? boardConfig->pinAinZeroRef : CS_ADC_REF_PIN_NOT_AVAILABLE;
	adcConfig.samplingIntervalUs = CS_ADC_SAMPLE_INTERVAL_US;
	_adc->init(adcConfig);

	_adc->setDoneCallback(adc_done_callback);

	// init the adc config
	_adcConfig[VOLTAGE_CHANNEL_IDX].config = adcConfig.channels[VOLTAGE_CHANNEL_IDX];
	_adcConfig[CURRENT_CHANNEL_IDX].config = adcConfig.channels[CURRENT_CHANNEL_IDX];

	// Count all available pins.
	uint8_t pinCount                       = 0;
	for (int i = 0; i < GAIN_COUNT; ++i) {
		if (_boardConfig->pinAinVoltage[i] != PIN_NONE) {
			++pinCount;
		}
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		if (_boardConfig->pinAinVoltageAfterLoad[i] != PIN_NONE) {
			++pinCount;
		}
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		if (_boardConfig->pinAinCurrent[i] != PIN_NONE) {
			++pinCount;
		}
	}
	if (_boardConfig->pinAinZeroRef != PIN_NONE) {
		++pinCount;
	}
	// Increase by 1 for VDD as input.
	++pinCount;

	// Both channels can select any pin.
	_adcConfig[VOLTAGE_CHANNEL_IDX].pinCount = pinCount;
	_adcConfig[CURRENT_CHANNEL_IDX].pinCount = pinCount;

	// Set last softfuse header data.
	_lastSoftfuse.type                       = POWER_SAMPLES_TYPE_SOFTFUSE;
	_lastSoftfuse.index                      = 0;
	_lastSoftfuse.count                      = 0;
	_lastSoftfuse.unixTimestamp              = 0;
	//	_lastSoftfuse.delayUs = 0;
	//	_lastSoftfuse.sampleIntervalUs = CS_ADC_SAMPLE_INTERVAL_US;
	//	_lastSoftfuse.offset = 0;
	//	_lastSoftfuse.multiplier = _currentMultiplier;

	EventDispatcher::getInstance().addListener(this);

	PS_TEST_PIN_INIT

	_isInitialized = true;
}

void PowerSampling::startSampling() {
	LOGi(FMT_START "power sample");

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

void PowerSampling::handleEvent(event_t& event) {
	switch (event.type) {
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
		case CS_TYPE::CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN: selectNextPin(VOLTAGE_CHANNEL_IDX); break;
		case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT:
			enableDifferentialModeCurrent(*(TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT)*)event.data);
			break;
		case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE:
			enableDifferentialModeVoltage(*(TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE)*)event.data);
			break;
		case CS_TYPE::CMD_INC_VOLTAGE_RANGE: changeRange(VOLTAGE_CHANNEL_IDX, 600); break;
		case CS_TYPE::CMD_DEC_VOLTAGE_RANGE: changeRange(VOLTAGE_CHANNEL_IDX, -600); break;
		case CS_TYPE::CMD_INC_CURRENT_RANGE: changeRange(CURRENT_CHANNEL_IDX, 600); break;
		case CS_TYPE::CMD_DEC_CURRENT_RANGE: changeRange(CURRENT_CHANNEL_IDX, -600); break;
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
			event.result.dataSize   = sizeof(_adcRestarts);
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		case CS_TYPE::CMD_GET_ADC_CHANNEL_SWAPS: {
			if (event.result.buf.len < sizeof(_adcChannelSwaps)) {
				event.result.returnCode = ERR_BUFFER_TOO_SMALL;
				return;
			}
			memcpy(event.result.buf.data, &_adcChannelSwaps, sizeof(_adcChannelSwaps));
			event.result.dataSize   = sizeof(_adcChannelSwaps);
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		case CS_TYPE::EVT_ADC_RESTARTED: {
			_adcRestarts.count++;
			_adcRestarts.lastTimestamp = SystemTime::posix();
			_bufferQueue.clear();
			UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ADC_RESTART, NULL, 0);
			//		RecognizeSwitch::getInstance().skip(2);
			break;
		}
		case CS_TYPE::CONFIG_SWITCHCRAFT_THRESHOLD: {
			TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD) threshold;
			memcpy(&threshold, event.data, sizeof(TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD)));
			RecognizeSwitch::getInstance().configure(threshold);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			if (_calibratePowerZeroCountDown) {
				--_calibratePowerZeroCountDown;
			}
			//			toggleVoltageChannelInput();
			break;
		}
		default: {
		}
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
void PowerSampling::powerSampleAdcDone(adc_buffer_id_t bufIndex) {
	adc_buffer_seq_nr_t seqNr = AdcBuffer::getInstance().getBuffer(bufIndex)->seqNr;
	LOGPowerSamplingVerbose("bufId=%u seqNr=%u", bufIndex, seqNr);
	PS_TEST_PIN_TOGGLE

	if (!isConsecutiveBuf(seqNr, _lastBufSeqNr)) {
		LOGw("buf skipped (prev=%u cur=%u)", _lastBufSeqNr, seqNr);
		// Clear buffer queue, as these are no longer consecutive.
		_bufferQueue.clear();
	}
	_lastBufSeqNr = seqNr;

	if (!isValidBuf(bufIndex)) {
		LOGPowerSamplingWarn("buf %u invalid", bufIndex);
		// Clear buffer queue, as these are no longer valid.
		_bufferQueue.clear();
		return;
	}

	adc_buffer_id_t filteredBufIndex;
	if (_bufferQueue.empty()) {
		// Filter current buffer to current buffer.
		filteredBufIndex = bufIndex;
	}
	else {
		// Filter current buffer to last buffer in queue (which is the last unfiltered buffer).
		filteredBufIndex = _bufferQueue[_bufferQueue.size() - 1];
	}

	if (!isValidBuf(filteredBufIndex)) {
		LOGPowerSamplingWarn("buf %u invalid", filteredBufIndex);
		// Clear buffer queue, as these are no longer valid.
		_bufferQueue.clear();
		return;
	}

	LOGnone("filter %u to %u", bufIndex, filteredBufIndex);
	_lastBufIndex         = bufIndex;
	_lastFilteredBufIndex = filteredBufIndex;

	_bufferQueue.push(bufIndex);

	TYPIFY(STATE_SWITCH_STATE) switchState;
	State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));
	_switchHist.push(switchState);

	// Filter current buffer to the previous unfiltered buffer.
	filter(bufIndex, filteredBufIndex, VOLTAGE_CHANNEL_IDX);
	filter(bufIndex, filteredBufIndex, CURRENT_CHANNEL_IDX);

	if (!isValidBuf(filteredBufIndex)) {
		LOGPowerSamplingWarn("buf %u invalid", filteredBufIndex);
		_bufferQueue.clear();
		return;
	}

	removeInvalidBufs();
	if (_bufferQueue.size() >= 2 + numUnfilteredBuffers) {
		adc_buffer_id_t prevIndex =
				_bufferQueue[_bufferQueue.size() - 2 - numUnfilteredBuffers];  // Previous filtered buffer.

		if (isVoltageAndCurrentSwapped(filteredBufIndex, prevIndex)) {
			if (isValidBuf(filteredBufIndex) && isValidBuf(prevIndex)) {
				LOGw("Swap detected.");
				printBuf(filteredBufIndex);
			}
		}
	}

	PS_TEST_PIN_TOGGLE

	// Use filtered samples to calculate the zero.
	if (_recalibrateZeroVoltage) {
		calculateVoltageZero(filteredBufIndex);
	}
	if (_recalibrateZeroCurrent) {
		calculateCurrentZero(filteredBufIndex);
	}

	if (!isValidBuf(filteredBufIndex)) {
		LOGPowerSamplingWarn("buf %u invalid", filteredBufIndex);
		return;
	}

	PS_TEST_PIN_TOGGLE

	if (!calculatePower(filteredBufIndex)) {
		LOGw("Failed to calculate power");
	}

	// TODO: if buffer is invalid, assume power remained similar and increase energy regardless?
	calculateEnergy();

	//	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {
	//		int32_t powerUsage = _slowAvgPowerMilliWatt;
	//		State::getInstance().set(CS_TYPE::STATE_POWER_USAGE, &powerUsage, sizeof(powerUsage));
	//		State::getInstance().set(CS_TYPE::STATE_POWER_USAGE, &_avgPowerMilliWatt, sizeof(_avgPowerMilliWatt));
	int32_t slowAvgPowerMilliWatt = _slowAvgPowerMilliWatt;
	State::getInstance().set(CS_TYPE::STATE_POWER_USAGE, &slowAvgPowerMilliWatt, sizeof(slowAvgPowerMilliWatt));
	State::getInstance().set(CS_TYPE::STATE_ACCUMULATED_ENERGY, &_energyUsedmicroJoule, sizeof(_energyUsedmicroJoule));
	//	}

	PS_TEST_PIN_TOGGLE

	bool switchDetected = RecognizeSwitch::getInstance().detect(_bufferQueue, VOLTAGE_CHANNEL_IDX);
	if (switchDetected) {
		LOGd("Switch event detected!");
		event_t event(CS_TYPE::CMD_SWITCH_TOGGLE, nullptr, 0, cmd_source_t(CS_CMD_SOURCE_SWITCHCRAFT));
		EventDispatcher::getInstance().dispatch(event);
	}

	if (_switchHist.size() >= 3) {
		if (_switchHist[_switchHist.size() - 2].asInt != _switchHist[_switchHist.size() - 1].asInt) {
			// Switch changed state since previous buffer.
			// Store the current and previous buffer.
			adc_buffer_id_t prevFilteredBufferIndex =
					_bufferQueue[_bufferQueue.size() - 2 - numUnfilteredBuffers];  // Previous filtered buffer.
			uint16_t count                         = AdcBuffer::getChannelLength();
			_lastSwitchSamplesHeader.type          = POWER_SAMPLES_TYPE_SWITCH;
			_lastSwitchSamplesHeader.count         = count;
			_lastSwitchSamplesHeader.unixTimestamp = SystemTime::posix();
			_lastSwitchSamplesHeader.delayUs       = 0;
			_lastSwitchSamplesHeader.sampleIntervalUs =
					AdcBuffer::getInstance().getBuffer(prevFilteredBufferIndex)->config[0].samplingIntervalUs;
			for (adc_sample_value_id_t i = 0; i < count; ++i) {
				_lastSwitchSamples[i + 0 * count] =
						AdcBuffer::getInstance().getValue(prevFilteredBufferIndex, VOLTAGE_CHANNEL_IDX, i);
				_lastSwitchSamples[i + 1 * count] =
						AdcBuffer::getInstance().getValue(prevFilteredBufferIndex, CURRENT_CHANNEL_IDX, i);
				_lastSwitchSamples[i + 2 * count] =
						AdcBuffer::getInstance().getValue(filteredBufIndex, VOLTAGE_CHANNEL_IDX, i);
				_lastSwitchSamples[i + 3 * count] =
						AdcBuffer::getInstance().getValue(filteredBufIndex, CURRENT_CHANNEL_IDX, i);
			}
		}
		else if (_switchHist[_switchHist.size() - 3].asInt != _switchHist[_switchHist.size() - 1].asInt) {
			// Switch changed state in previous buffer.
			uint16_t count = AdcBuffer::getChannelLength();
			for (adc_sample_value_id_t i = 0; i < count; ++i) {
				_lastSwitchSamples[i + 4 * count] =
						AdcBuffer::getInstance().getValue(filteredBufIndex, VOLTAGE_CHANNEL_IDX, i);
				_lastSwitchSamples[i + 5 * count] =
						AdcBuffer::getInstance().getValue(filteredBufIndex, CURRENT_CHANNEL_IDX, i);
			}
		}
	}
}

void PowerSampling::initAverages() {
	_avgZeroVoltage        = _voltageZero * 1024;
	_avgZeroCurrent        = _currentZero * 1024;
	_avgPowerMilliWatt     = 0;
	_slowAvgPowerMilliWatt = 0.0;
}

bool PowerSampling::isValidBuf(adc_buffer_id_t bufIndex) {
	return AdcBuffer::getInstance().getBuffer(bufIndex)->valid;
}

bool PowerSampling::isConsecutiveBuf(adc_buffer_seq_nr_t seqNr, adc_buffer_seq_nr_t prevSeqNr) {
	adc_buffer_seq_nr_t diff = seqNr - prevSeqNr;
	return diff == 1;
}

void PowerSampling::removeInvalidBufs() {
	// Find the newest invalid queue index
	int newestInvalidIndex = -1;
	for (uint16_t i = 0; i < _bufferQueue.size(); ++i) {
		if (!isValidBuf(_bufferQueue[i])) {
			newestInvalidIndex = i;
		}
	}
	if (newestInvalidIndex > -1) {
		LOGPowerSamplingDebug(
				"buf %u invalid (%uth in queue of size %u)",
				_bufferQueue[newestInvalidIndex],
				newestInvalidIndex,
				_bufferQueue.size());
		// Remove this and older buffers from queue.
		for (uint16_t i = 0; i <= newestInvalidIndex; ++i) {
			LOGPowerSamplingDebug("remove buf %u from queue", _bufferQueue.peek());
			_bufferQueue.pop();
		}
	}
}

/*
 * Other idea:
 * - compare Vrms[t-1] with Vrms[t] and Irms[t]. If Vrms[t-1] is more similar to Irms[t] than to Vrms[t], then swapped.
 * Problems:
 * - Vzero and Izero can be different and affect Vrm and Irms.
 */
bool PowerSampling::isVoltageAndCurrentSwapped(adc_buffer_id_t bufIndex, adc_buffer_id_t prevBufIndex) {
	// Problem: switchcraft makes this function falsely detect a swap.
	return false;

	adc_sample_value_t prevSample, voltageSample, currentSample;

	uint32_t sumVoltageChannel = 0;
	uint32_t sumCurrentChannel = 0;

	int16_t maxPrevSample      = INT16_MIN;
	int16_t maxVoltageSample   = INT16_MIN;
	int16_t maxCurrentSample   = INT16_MIN;

	int16_t minPrevSample      = INT16_MAX;
	int16_t minVoltageSample   = INT16_MAX;
	int16_t minCurrentSample   = INT16_MAX;

	for (adc_sample_value_id_t i = 0; i < AdcBuffer::getChannelLength(); ++i) {
		prevSample    = AdcBuffer::getInstance().getValue(prevBufIndex, VOLTAGE_CHANNEL_IDX, i);
		voltageSample = AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, i);
		currentSample = AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, i);
		LOGd("%i %i %i", prevSample, voltageSample, currentSample);

		sumVoltageChannel += std::abs(voltageSample - prevSample);
		sumCurrentChannel += std::abs(currentSample - prevSample);

		// Keep up min and max sample values.
		if (prevSample > maxPrevSample) {
			maxPrevSample = prevSample;
		}
		if (prevSample < minPrevSample) {
			minPrevSample = prevSample;
		}
		if (voltageSample > maxVoltageSample) {
			maxVoltageSample = voltageSample;
		}
		if (voltageSample < minVoltageSample) {
			minVoltageSample = voltageSample;
		}
		if (currentSample > maxCurrentSample) {
			maxCurrentSample = currentSample;
		}
		if (currentSample < minCurrentSample) {
			minCurrentSample = currentSample;
		}
	}

	int16_t minMaxVoltageChannel =
			std::abs(maxPrevSample - maxVoltageSample) + std::abs(minPrevSample - minVoltageSample);
	int16_t minMaxCurrentChannel =
			std::abs(maxPrevSample - maxCurrentSample) + std::abs(minPrevSample - minCurrentSample);
	if (minMaxVoltageChannel > minMaxCurrentChannel) {
		LOGd("minMaxVoltageChannel=%i minMaxCurrentChannel=%i", minMaxVoltageChannel, minMaxCurrentChannel);
		LOGd("prev=[%i %i] voltage=[%i %i] current=[%i %i]",
			 minPrevSample,
			 maxPrevSample,
			 minVoltageSample,
			 maxVoltageSample,
			 minCurrentSample,
			 maxCurrentSample);
	}

	if (sumVoltageChannel > sumCurrentChannel) {
		LOGd("sumVoltageChannel=%u sumCurrentChannel=%u", sumVoltageChannel, sumCurrentChannel);
	}
	return (sumCurrentChannel < sumVoltageChannel);
}

void PowerSampling::calculateVoltageZero(adc_buffer_id_t bufIndex) {
	// Simply use the average of an AC period.
	adc_sample_value_id_t numSamples =
			AC_PERIOD_US / AdcBuffer::getInstance().getBuffer(bufIndex)->config[VOLTAGE_CHANNEL_IDX].samplingIntervalUs;
	assert(numSamples <= AdcBuffer::getChannelLength(), "Not enough samples");

	int64_t sum = 0;
	for (adc_sample_value_id_t i = 0; i < numSamples; ++i) {
		sum += AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, i);
	}

	if (!isValidBuf(bufIndex)) {
		// Don't use the calculation.
		LOGPowerSamplingWarn("buf %u invalid", bufIndex);
		return;
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
		int64_t avgZeroVoltageDiscount = _avgZeroVoltageDiscount;  // Make sure calculations are in int64_t
		_avgZeroVoltage =
				((1000 - avgZeroVoltageDiscount) * _avgZeroVoltage + avgZeroVoltageDiscount * zeroVoltage) / 1000;
	}
	if (_zeroVoltageCount < 65535) {
		++_zeroVoltageCount;
	}
}

void PowerSampling::calculateCurrentZero(adc_buffer_id_t bufIndex) {
	// Simply use the average of an AC period.
	adc_sample_value_id_t numSamples =
			AC_PERIOD_US / AdcBuffer::getInstance().getBuffer(bufIndex)->config[CURRENT_CHANNEL_IDX].samplingIntervalUs;
	assert(numSamples <= AdcBuffer::getChannelLength(), "Not enough samples");

	int64_t sum = 0;
	for (adc_sample_value_id_t i = 0; i < numSamples; ++i) {
		sum += AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, i);
	}

	if (!isValidBuf(bufIndex)) {
		// Don't use the calculation.
		LOGPowerSamplingWarn("buf %u invalid", bufIndex);
		return;
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
		int64_t avgZeroCurrentDiscount = _avgZeroCurrentDiscount;  // Make sure calculations are in int64_t
		_avgZeroCurrent =
				((1000 - avgZeroCurrentDiscount) * _avgZeroCurrent + avgZeroCurrentDiscount * zeroCurrent) / 1000;
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
void PowerSampling::filter(adc_buffer_id_t bufIndexIn, adc_buffer_id_t bufIndexOut, adc_channel_id_t channel_id) {

	// Pad the start of the input vector with the first sample in the buffer
	uint16_t j                      = 0;
	adc_sample_value_t padded_value = AdcBuffer::getInstance().getValue(bufIndexIn, channel_id, 0);
	for (uint16_t i = 0; i < _filterParams->half; ++i, ++j) {
		_inputSamples->at(j) = padded_value;
	}

	// Copy samples from buffer to input vector
	uint8_t channel_length = AdcBuffer::getInstance().getChannelLength();
	for (int i = 0; i < channel_length; ++i, ++j) {
		_inputSamples->at(j) = AdcBuffer::getInstance().getValue(bufIndexIn, channel_id, i);
	}

	// Pad the end of the buffer with the last sample in the buffer
	padded_value = AdcBuffer::getInstance().getValue(bufIndexIn, channel_id, channel_length - 1);
	for (uint16_t i = 0; i < _filterParams->half; ++i, ++j) {
		_inputSamples->at(j) = padded_value;
	}

	// Filter the data
	sort_median(*_filterParams, *_inputSamples, *_outputSamples);

	// Copy the result back into the buffer
	for (int i = 0; i < AdcBuffer::getInstance().getChannelLength(); ++i) {
		AdcBuffer::getInstance().setValue(bufIndexOut, channel_id, i, _outputSamples->at(i));
	}
}

bool PowerSampling::calculatePower(adc_buffer_id_t bufIndex) {

	adc_sample_value_id_t numSamples =
			AC_PERIOD_US / AdcBuffer::getInstance().getBuffer(bufIndex)->config[VOLTAGE_CHANNEL_IDX].samplingIntervalUs;
	assert(numSamples <= AdcBuffer::getChannelLength(), "Not enough samples");

	//////////////////////////////////////////////////
	// Calculatate power, Irms, and Vrms
	//////////////////////////////////////////////////

	// The int64_t sum is large enough: 2^63 / (2^12 * 1000 * 2^12 * 1000) = 5*10^5. Many more samples than the 100 we
	// use.
	int64_t pSum       = 0;
	int64_t cSquareSum = 0;
	int64_t vSquareSum = 0;
	int64_t current;
	int64_t voltage;
	for (adc_sample_value_id_t i = 0; i < numSamples; ++i) {
		voltage = (int64_t)AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, i) * 1024 - _avgZeroVoltage;
		current = (int64_t)AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, i) * 1024 - _avgZeroCurrent;
		vSquareSum += (voltage * voltage) / (1024 * 1024);
		cSquareSum += (current * current) / (1024 * 1024);
		pSum += (current * voltage) / (1024 * 1024);
	}
	if (!isValidBuf(bufIndex)) {
		LOGPowerSamplingWarn("buf %u invalid", bufIndex);
		return false;
	}

	int32_t powerMilliWattReal = pSum * _currentMultiplier * _voltageMultiplier * 1000 / numSamples;
	int32_t currentRmsMA       = sqrt((double)cSquareSum * _currentMultiplier * _currentMultiplier / numSamples) * 1000;
	int32_t voltageRmsMilliVolt =
			sqrt((double)vSquareSum * _voltageMultiplier * _voltageMultiplier / numSamples) * 1000;

	////////////////////////////////////////////////////////////////////////////////
	// Calculate Irms of median filtered samples, and filter over multiple periods
	////////////////////////////////////////////////////////////////////////////////

	//	// Calculate Irms again, but now with the filtered current samples
	//	cSquareSum = 0;
	//	for (uint16_t i=0; i<numSamples; ++i) {
	//		current = (int64_t)_outputSamples->at(i)*1000 - _avgZeroCurrent;
	//		cSquareSum += (current * current) / (1000*1000);
	//	}
	//	int32_t filteredCurrentRmsMA =  sqrt((double)cSquareSum * _currentMultiplier * _currentMultiplier / numSamples)
	//* 1000;

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
		for (uint16_t i = 0; i < _filteredCurrentRmsHistMA->size(); ++i) {
			currentRmsSumMA += _filteredCurrentRmsHistMA->operator[](i);
		}
		filteredCurrentRmsMedianMA = currentRmsSumMA / _filteredCurrentRmsHistMA->size();
	}

	// Now that Irms is known: first check the soft fuse.
	// Wait some time, for the measurement to converge.. why does this have to take so long?
	//	if (_zeroCurrentInitialized && _zeroVoltageInitialized) {
	if (_zeroVoltageCount > 200 && _zeroCurrentCount > 200) {
		checkSoftfuse(currentRmsMA, filteredCurrentRmsMedianMA, voltageRmsMilliVolt, bufIndex);
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
		for (uint16_t i = 0; i < _currentRmsMilliAmpHist->size(); ++i) {
			currentRmsMilliAmpSum += _currentRmsMilliAmpHist->operator[](i);
		}
		currentRmsMedianMA = currentRmsMilliAmpSum / _currentRmsMilliAmpHist->size();
	}

	//	// Exponential moving average of the median
	//	int64_t discountCurrent = 200;
	//	_avgCurrentRmsMilliAmp = ((1000-discountCurrent) * _avgCurrentRmsMilliAmp + discountCurrent *
	//  medianCurrentRmsMilliAmp) / 1000;

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
		for (uint16_t i = 0; i < _voltageRmsMilliVoltHist->size(); ++i) {
			voltageRmsMilliVoltSum += _voltageRmsMilliVoltHist->operator[](i);
		}
		_avgVoltageRmsMilliVolt = voltageRmsMilliVoltSum / _voltageRmsMilliVoltHist->size();
	}

	// Calculate apparent power: current_rms * voltage_rms
	__attribute__((unused)) uint32_t powerMilliWattApparent =
			(int64_t)_avgCurrentRmsMilliAmp * _avgVoltageRmsMilliVolt / 1000;

	if (_powerZero == CONFIG_POWER_ZERO_INVALID) {
		calibratePowerZero(_avgPowerMilliWatt);
	}
	else {
		powerMilliWattReal -= _powerZero;
	}

	// Exponential moving average
	int64_t avgPowerDiscount = _avgPowerDiscount;
	_avgPowerMilliWatt =
			((1000 - avgPowerDiscount) * _avgPowerMilliWatt + avgPowerDiscount * powerMilliWattReal) / 1000;

	calculateSlowAveragePower(powerMilliWattReal, _avgPowerMilliWatt);

	/////////////////////////////////////////////////////////
	// Debug prints
	/////////////////////////////////////////////////////////

#ifdef PRINT_POWER_SAMPLES

	//	for (int i = power.voltageIndex; i < numSamples * power.numChannels; i += power.numChannels) {
	//		if (power.buf[i] < 100) {
	//			APP_ERROR_CHECK(1);
	//		}
	//	}

	if (printPower % 10 == 0) {
		LOGd("Izero=%i Vzero=%i", _avgZeroCurrent, _avgZeroVoltage);
		LOGd("I: rms=%umA rmsMedian=%umA rmsFiltered=%umA rmsFilteredMedian=%umA  V: rms=%u  P: avg=%imW real=%imW "
			 "apparent=%imW",
			 currentRmsMA,
			 currentRmsMedianMA,
			 filteredCurrentRmsMA,
			 filteredCurrentRmsMedianMA,
			 voltageRmsMilliVolt,
			 _avgPowerMilliWatt,
			 powerMilliWattReal,
			 powerMilliWattApparent);
	}

	++printPower;
#endif

	uint32_t rtcCount = RTC::getCount();
	if (_logsEnabled.flags.power) {
		// Calculated values
		uart_msg_power_t powerMsg;
		powerMsg.timestamp                  = rtcCount;
		powerMsg.currentRmsMA               = currentRmsMA;
		powerMsg.currentRmsMedianMA         = currentRmsMedianMA;
		powerMsg.filteredCurrentRmsMA       = filteredCurrentRmsMA;
		powerMsg.filteredCurrentRmsMedianMA = filteredCurrentRmsMedianMA;
		powerMsg.avgZeroVoltage             = _avgZeroVoltage;
		powerMsg.avgZeroCurrent             = _avgZeroCurrent;
		powerMsg.powerMilliWattApparent     = powerMilliWattApparent;
		powerMsg.powerMilliWattReal         = powerMilliWattReal;
		powerMsg.avgPowerMilliWattReal      = _avgPowerMilliWatt;

		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_POWER_LOG_POWER, (uint8_t*)&powerMsg, sizeof(powerMsg));
	}

	if (_logsEnabled.flags.current) {
		// Write uart_msg_current_t without allocating a buffer.
		UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_POWER_LOG_CURRENT, sizeof(uart_msg_current_t));
		UartHandler::getInstance().writeMsgPart(
				UART_OPCODE_TX_POWER_LOG_CURRENT, (uint8_t*)&(rtcCount), sizeof(rtcCount));
		adc_sample_value_t val;
		for (adc_sample_value_id_t i = 0; i < AdcBuffer::getChannelLength(); ++i) {
			val = AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, i);
			UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_CURRENT, (uint8_t*)&val, sizeof(val));
		}
		UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_POWER_LOG_CURRENT);
	}

	if (_logsEnabled.flags.filteredCurrent) {
		// Write uart_msg_current_t without allocating a buffer.
		UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT, sizeof(uart_msg_current_t));
		UartHandler::getInstance().writeMsgPart(
				UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT, (uint8_t*)&(rtcCount), sizeof(rtcCount));
		adc_sample_value_t val;
		for (adc_sample_value_id_t i = 0; i < AdcBuffer::getChannelLength(); ++i) {
			val = AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, i);
			UartHandler::getInstance().writeMsgPart(
					UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT, (uint8_t*)&val, sizeof(val));
		}
		UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT);
	}

	if (_logsEnabled.flags.voltage) {
		// Write uart_msg_voltage_t without allocating a buffer.
		UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_POWER_LOG_VOLTAGE, sizeof(uart_msg_voltage_t));
		UartHandler::getInstance().writeMsgPart(
				UART_OPCODE_TX_POWER_LOG_VOLTAGE, (uint8_t*)&(rtcCount), sizeof(rtcCount));
		adc_sample_value_t val;
		for (adc_sample_value_id_t i = 0; i < AdcBuffer::getChannelLength(); ++i) {
			val = AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, i);
			UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_VOLTAGE, (uint8_t*)&val, sizeof(val));
		}
		UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_POWER_LOG_VOLTAGE);
	}

	return true;
}

void PowerSampling::calculateSlowAveragePower(float powerMilliWatt, float fastAvgPowerMilliWatt) {
	if (_switchHist.size() >= 2) {
		if (_switchHist[_switchHist.size() - 2].asInt != _switchHist[_switchHist.size() - 1].asInt) {
			if (_switchHist[_switchHist.size() - 1].asInt == 0) {
				// Switch has just been turned off: reset slow average to 0.
				_slowAvgPowerMilliWatt = 0.0f;
				LOGPowerSamplingDebug("switched off: reset slow avg to 0");
			}
			else {
				// Switch just turned on, or to a different dim percentage: reset to fast average.
				_slowAvgPowerMilliWatt = fastAvgPowerMilliWatt;
				LOGPowerSamplingDebug("switched on: reset slow avg to %i mW", (int32_t)powerMilliWatt);
			}
			_slowAvgPowerCount = 0;
		}
	}

	float significantChangeThreshold =
			std::max(_slowAvgPowerMilliWatt * _powerDiffThresholdPart, _powerDiffThresholdMinMilliWatt);
	if (std::abs(fastAvgPowerMilliWatt - _slowAvgPowerMilliWatt) > significantChangeThreshold) {
		LOGPowerSamplingDebug(
				"Significant power change: cur=%i fastAvg=%i slowAvg=%i diff=%i thresh=%i",
				(int32_t)powerMilliWatt,
				(int32_t)fastAvgPowerMilliWatt,
				(int32_t)_slowAvgPowerMilliWatt,
				(int32_t)std::abs(fastAvgPowerMilliWatt - _slowAvgPowerMilliWatt),
				(int32_t)significantChangeThreshold);
		// Reset the slow average to fast average.
		_slowAvgPowerMilliWatt = fastAvgPowerMilliWatt;
		_slowAvgPowerCount     = 0;
	}

	if (_slowAvgPowerCount < slowAvgPowerConvergedCount) {
		++_slowAvgPowerCount;
	}
	if (_slowAvgPowerCount < 50) {
		// After a reset, slowly go up from 0.5 to 0.01.
		// Since, after a reset, we init with fastAvgPowerMilliWatt, this means we end up with the mean of
		// powerMilliWatt and fastAvgPowerMilliWatt. We do that, because most devices will have some inrush current, so
		// the powerMilliWatt will overshoot. Discount of 0.01 --> 99% of the average is influenced by the last 458
		// values, 50% by the last 68. Because we increase count before this line, we never divide by 0.
		_slowAvgPowerDiscount = 0.5f / _slowAvgPowerCount;
	}
	// Exponential moving average.
	_slowAvgPowerMilliWatt =
			(1.0f - _slowAvgPowerDiscount) * _slowAvgPowerMilliWatt + _slowAvgPowerDiscount * (float)powerMilliWatt;

	// Print slow average shortly after a reset.
	if (_slowAvgPowerCount < 4) {
		LOGPowerSamplingDebug("slowAvg=%i", (int32_t)_slowAvgPowerMilliWatt);
	}
};

void PowerSampling::calibratePowerZero(int32_t powerMilliWatt) {
	TYPIFY(STATE_SWITCH_STATE) switchState;
	State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));
	if (_calibratePowerZeroCountDown != 0 || switchState.asInt != 0) {
		return;
	}

	// Use the slow average instead
	if (_slowAvgPowerCount < slowAvgPowerConvergedCount) {
		return;
	}
	powerMilliWatt = _slowAvgPowerMilliWatt;

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
	// Assume we process every buffer, so simply only multiple power with the buffer duration.
	// Only add negative energy when power is below the threshold.
	if (_slowAvgPowerMilliWatt > 0.0f || _slowAvgPowerMilliWatt < _negativePowerThresholdMilliWatt) {
		_energyUsedmicroJoule +=
				_slowAvgPowerMilliWatt * (CS_ADC_SAMPLE_INTERVAL_US / 1000.0f * AdcBuffer::getChannelLength());
	}
}

void PowerSampling::checkSoftfuse(
		int32_t currentRmsMilliAmp,
		int32_t currentRmsMilliAmpFiltered,
		int32_t voltageRmsMilliVolt,
		adc_buffer_id_t bufIndex) {

	// Get the current state errors
	TYPIFY(STATE_ERRORS) stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors.asInt, sizeof(stateErrors));

	// Maybe use uptime instead.
	if (!_dimmerFailureDetectionStarted && (RTC::getCount() > RTC::msToTicks(5000))) {
		LOGi("Start dimmer failure detection");
		_dimmerFailureDetectionStarted = true;
		printBuf(bufIndex);
	}

	// TODO: use _switchHist instead
	switch_state_t prevSwitchState = _lastSwitchState;
	// Get the current switch state before we dispatch any event (as that may change the switch).
	TYPIFY(STATE_SWITCH_STATE) switchState;
	State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));
	_lastSwitchState = switchState;

	// Check if the switch has been turned off.
	if (switchState.state.relay == 0 && switchState.state.dimmer == 0
		&& (prevSwitchState.state.relay || prevSwitchState.state.dimmer)) {
		_lastSwitchOffTicksValid = true;
		_lastSwitchOffTicks      = RTC::getCount();
	}

	// Check if switch has recently been turned off.
	bool recentlySwitchedOff = false;
	if (_lastSwitchOffTicksValid) {
		// TODO: use tick counter or uptime instead
		uint32_t tickDiff = RTC::difference(RTC::getCount(), _lastSwitchOffTicks);
		if (tickDiff < RTC::msToTicks(1000)) {
			recentlySwitchedOff = true;
		}
		else {
			// Timed out
			_lastSwitchOffTicksValid = false;
		}
	}

	// Store last buffer that could trigger the (dimmer) over current softfuse.
	// Use the unfiltered current RMS for this, so we don't end up with a buffer that is no longer above threshold,
	// while the filtered current RMS still is.
	if ((currentRmsMilliAmp > _currentMilliAmpThresholdDimmer) && (!stateErrors.errors.overCurrentDimmer)) {
		if ((switchState.state.dimmer != 0)
			|| (switchState.state.relay == 0 && !recentlySwitchedOff && _dimmerFailureDetectionStarted)
			|| ((currentRmsMilliAmp > _currentMilliAmpThreshold) && (!stateErrors.errors.overCurrent))) {
			_lastSoftfuse.type          = POWER_SAMPLES_TYPE_SOFTFUSE;
			_lastSoftfuse.index         = 0;
			_lastSoftfuse.count         = AdcBuffer::getChannelLength();
			_lastSoftfuse.unixTimestamp = SystemTime::posix();
			_lastSoftfuse.delayUs       = 0;
			_lastSoftfuse.sampleIntervalUs =
					AdcBuffer::getInstance().getBuffer(bufIndex)->config[CURRENT_CHANNEL_IDX].samplingIntervalUs;
			_lastSoftfuse.offset     = _avgZeroCurrent / 1024;
			_lastSoftfuse.multiplier = _currentMultiplier;
			for (adc_sample_value_id_t i = 0; i < AdcBuffer::getChannelLength(); ++i) {
				_lastSoftfuseSamples[i] = AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, i);
			}
		}
	}

	// Check if data makes sense: RMS voltage should be about 230V.
	if (voltageRmsMilliVolt != 0 && (voltageRmsMilliVolt < 200 * 1000 || 250 * 1000 < voltageRmsMilliVolt)) {
		_adcChannelSwaps.count++;
		_adcChannelSwaps.lastTimestamp = SystemTime::posix();
		LOGw("voltageRmsMilliVolt=%u", voltageRmsMilliVolt);
		return;
	}

	// Count number of consecutive times that the current is over the threshold.
	if (currentRmsMilliAmpFiltered > _currentMilliAmpThreshold) {
		++_consecutiveOvercurrent;
	}
	else {
		_consecutiveOvercurrent = 0;
	}
	if ((_consecutiveOvercurrent > CURRENT_THRESHOLD_CONSECUTIVE) && (!stateErrors.errors.overCurrent)) {
		LOGw("Overcurrent: Irms=%i mA V=[%i %i ..] C=[%i %i ..]",
			 currentRmsMilliAmpFiltered,
			 AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, 0),
			 AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, 1),
			 AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, 0),
			 AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, 1));
		printBuf(bufIndex);

		// Set error first, so the switch knows what it's allowed to do.
		stateErrors.errors.overCurrent = true;
		State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));

		event_t event(CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD);
		EventDispatcher::getInstance().dispatch(event);
		return;
	}

	// Count number of consecutive times that the current is over the dimmer threshold.
	if (currentRmsMilliAmpFiltered > _currentMilliAmpThresholdDimmer) {
		++_consecutiveDimmerOvercurrent;
	}
	else {
		_consecutiveDimmerOvercurrent = 0;
	}
	if ((_consecutiveDimmerOvercurrent > CURRENT_THRESHOLD_DIMMER_CONSECUTIVE)
		&& (!stateErrors.errors.overCurrentDimmer)) {
		if (switchState.state.dimmer != 0) {
			// If the dimmer is on:
			LOGw("Dimmer overcurrent: Irms=%i mA V=[%i %i ..] C=[%i %i ..]",
				 currentRmsMilliAmpFiltered,
				 AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, 0),
				 AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, 1),
				 AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, 0),
				 AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, 1));
			printBuf(bufIndex);

			// Set error first, so the switch knows what it's allowed to do.
			stateErrors.errors.overCurrentDimmer = true;
			State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));

			// Dispatch the event that will turn off the dimmer
			event_t event(CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER);
			EventDispatcher::getInstance().dispatch(event);
		}
		else if (switchState.state.relay == 0 && !recentlySwitchedOff && _dimmerFailureDetectionStarted) {
			// If there is current flowing, but relay and dimmer are both off, then the dimmer is probably broken.
			LOGw("Dimmer failure detected: Irms=%i mA V=[%i %i ..] C=[%i %i ..]",
				 currentRmsMilliAmpFiltered,
				 AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, 0),
				 AdcBuffer::getInstance().getValue(bufIndex, VOLTAGE_CHANNEL_IDX, 1),
				 AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, 0),
				 AdcBuffer::getInstance().getValue(bufIndex, CURRENT_CHANNEL_IDX, 1));
			printBuf(bufIndex);

			// Set error first, so the switch knows what it's allowed to do.
			stateErrors.errors.dimmerOn = true;
			State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));

			event_t event(CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED);
			EventDispatcher::getInstance().dispatch(event);
		}
	}
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
			if (index > 1) {
				LOGw("index=%u", index);
				result.returnCode = ERR_WRONG_PARAMETER;
				return;
			}

			// Check size
			cs_power_samples_header_t* header = (cs_power_samples_header_t*)result.buf.data;
			uint16_t numSamples               = AdcBuffer::getInstance().getChannelLength();
			uint16_t samplesSize              = numSamples * sizeof(adc_sample_value_t);
			size16_t requiredSize             = sizeof(*header) + samplesSize;
			if (result.buf.len < requiredSize) {
				LOGw("size=%u required=%u", result.buf.len, requiredSize);
				result.returnCode = ERR_BUFFER_TOO_SMALL;
				return;
			}

			// Set header fields.
			header->type             = type;
			header->index            = index;
			header->count            = numSamples;
			header->unixTimestamp    = SystemTime::posix();
			header->delayUs          = 0;
			header->sampleIntervalUs = CS_ADC_SAMPLE_INTERVAL_US;
			if (index == VOLTAGE_CHANNEL_IDX) {
				header->offset     = _avgZeroVoltage / 1024;
				header->multiplier = _voltageMultiplier;
			}
			else {
				header->offset     = _avgZeroCurrent / 1024;
				header->multiplier = _currentMultiplier;
			}

			// Copy samples
			adc_buffer_id_t bufIndex =
					(type == POWER_SAMPLES_TYPE_NOW_FILTERED) ? _lastFilteredBufIndex : _lastBufIndex;
			adc_sample_value_t* samples = (adc_sample_value_t*)(result.buf.data + sizeof(*header));
			for (adc_sample_value_id_t i = 0; i < numSamples; ++i) {
				samples[i] = AdcBuffer::getInstance().getValue(bufIndex, index, i);
			}

			// After reading the buffer, check if it was valid
			if (!isValidBuf(bufIndex)) {
				LOGw("buf %u invalid", bufIndex);
				result.returnCode = ERR_NOT_AVAILABLE;
				return;
			}

			result.dataSize   = requiredSize;
			result.returnCode = ERR_SUCCESS;
			break;
		}
		case POWER_SAMPLES_TYPE_SOFTFUSE: {
			// Check index
			if (index > 0) {
				LOGw("index=%u", index);
				result.returnCode = ERR_WRONG_PARAMETER;
				return;
			}

			// Check size
			size16_t samplesSize  = _lastSoftfuse.count * sizeof(int16_t);
			size16_t requiredSize = sizeof(_lastSoftfuse) + samplesSize;
			if (result.buf.len < requiredSize) {
				LOGw("size=%u required=%u", result.buf.len, requiredSize);
				result.returnCode = ERR_BUFFER_TOO_SMALL;
				return;
			}

			// Copy data to buffer.
			memcpy(result.buf.data, &_lastSoftfuse, sizeof(_lastSoftfuse));
			memcpy(result.buf.data + sizeof(_lastSoftfuse), _lastSoftfuseSamples, samplesSize);

			result.dataSize   = requiredSize;
			result.returnCode = ERR_SUCCESS;
			break;
		}
		case POWER_SAMPLES_TYPE_SWITCH: {
			if (index >= numSwitchSamplesBuffers) {
				LOGw("index=%u", index);
				result.returnCode = ERR_WRONG_PARAMETER;
				return;
			}

			// Check size
			size16_t samplesSize  = _lastSwitchSamplesHeader.count * sizeof(int16_t);
			size16_t requiredSize = sizeof(_lastSwitchSamplesHeader) + samplesSize;
			if (result.buf.len < requiredSize) {
				LOGw("size=%u required=%u", result.buf.len, requiredSize);
				result.returnCode = ERR_BUFFER_TOO_SMALL;
				return;
			}

			// Set header data
			_lastSwitchSamplesHeader.index = index;
			if (index % 2 == 0) {
				_lastSwitchSamplesHeader.offset = _avgZeroVoltage / 1024;
				;
				_lastSwitchSamplesHeader.multiplier = _voltageMultiplier;
			}
			else {
				_lastSwitchSamplesHeader.offset = _avgZeroCurrent / 1024;
				;
				_lastSwitchSamplesHeader.multiplier = _currentMultiplier;
			}

			// Copy data to buffer.
			memcpy(result.buf.data, &_lastSwitchSamplesHeader, sizeof(_lastSwitchSamplesHeader));
			memcpy(result.buf.data + sizeof(_lastSwitchSamplesHeader),
				   _lastSwitchSamples + index * _lastSwitchSamplesHeader.count,
				   samplesSize);

			result.dataSize   = requiredSize;
			result.returnCode = ERR_SUCCESS;
			break;
		}
		default: break;
	}
}

void PowerSampling::selectNextPin(adc_channel_id_t channel) {
	_adcConfig[channel].pinIndex = (_adcConfig[channel].pinIndex + 1) % _adcConfig[channel].pinCount;
	LOGd("selectNextPin channel=%u pinIndex=%u", channel, _adcConfig[channel].pinIndex);

	// For now, both channels use the same selection.
	uint8_t pinCount = 0;
	for (int i = 0; i < GAIN_COUNT; ++i) {
		if (_boardConfig->pinAinVoltage[i] != PIN_NONE) {
			if (_adcConfig[channel].pinIndex == pinCount) {
				_adcConfig[channel].config.pin = _boardConfig->pinAinVoltage[i];
			}
			++pinCount;
		}
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		if (_boardConfig->pinAinVoltageAfterLoad[i] != PIN_NONE) {
			if (_adcConfig[channel].pinIndex == pinCount) {
				_adcConfig[channel].config.pin = _boardConfig->pinAinVoltageAfterLoad[i];
			}
			++pinCount;
		}
	}
	for (int i = 0; i < GAIN_COUNT; ++i) {
		if (_boardConfig->pinAinCurrent[i] != PIN_NONE) {
			if (_adcConfig[channel].pinIndex == pinCount) {
				_adcConfig[channel].config.pin = _boardConfig->pinAinCurrent[i];
			}
			++pinCount;
		}
	}
	if (_boardConfig->pinAinZeroRef != PIN_NONE) {
		if (_adcConfig[channel].pinIndex == pinCount) {
			_adcConfig[channel].config.pin = _boardConfig->pinAinZeroRef;
		}
		++pinCount;
	}
	if (_adcConfig[channel].pinIndex == pinCount) {
		_adcConfig[channel].config.pin = CS_ADC_PIN_VDD;
	}
	++pinCount;
	applyAdcConfig(channel);
}

void PowerSampling::enableDifferentialModeCurrent(bool enable) {
	_adcConfig[CURRENT_CHANNEL_IDX].config.referencePin =
			enable ? _boardConfig->pinAinZeroRef : CS_ADC_REF_PIN_NOT_AVAILABLE;
	applyAdcConfig(CURRENT_CHANNEL_IDX);
}

void PowerSampling::enableDifferentialModeVoltage(bool enable) {
	_adcConfig[VOLTAGE_CHANNEL_IDX].config.referencePin =
			enable ? _boardConfig->pinAinZeroRef : CS_ADC_REF_PIN_NOT_AVAILABLE;
	applyAdcConfig(VOLTAGE_CHANNEL_IDX);
}

void PowerSampling::changeRange(adc_channel_id_t channel, int32_t amount) {
	uint16_t newRange = _adcConfig[channel].config.rangeMilliVolt + amount;
	switch (_adcConfig[channel].config.rangeMilliVolt) {
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
	_adcConfig[channel].config.rangeMilliVolt = newRange;

	applyAdcConfig(channel);
}

void PowerSampling::applyAdcConfig(adc_channel_id_t channelIndex) {
	_adc->changeChannel(channelIndex, _adcConfig[channelIndex].config);
}

void PowerSampling::enableSwitchcraft(bool enable) {
	if (enable) {
		RecognizeSwitch::getInstance().start();
	}
	else {
		RecognizeSwitch::getInstance().stop();
	}
}

void PowerSampling::printBuf(adc_buffer_id_t bufIndex) {
	LOGd("ADC buf:");
	// Copy to buf and log that buf, instead of doing a uart msg per sample.
	// Only works if channel length is divisible by 10.
	__attribute__((unused)) adc_sample_value_id_t buf[10];
	for (adc_channel_id_t channel = 0; channel < AdcBuffer::getChannelCount(); ++channel) {
		LOGd("channel %u:", channel);
		for (adc_sample_value_id_t i = 0, j = 0; i < AdcBuffer::getChannelLength(); ++i) {
			buf[j] = AdcBuffer::getInstance().getValue(bufIndex, channel, i);
			if (++j == 10) {
				_logArray(SERIAL_DEBUG, true, buf, sizeof(buf));
				j = 0;
			}
		}
	}
}
