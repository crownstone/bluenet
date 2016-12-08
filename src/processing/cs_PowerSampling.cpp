/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */

#include <processing/cs_PowerSampling.h>

#include <cfg/cs_Boards.h>
#include <storage/cs_Settings.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_ADC.h>
#include <drivers/cs_RTC.h>
#include <protocol/cs_StateTypes.h>
#include <events/cs_EventDispatcher.h>

#if BUILD_MESHING == 1
#include <mesh/cs_MeshControl.h>
//extern "C" {
//	#include <notification_buffer.h>
//}
#endif

#define PRINT_POWERSAMPLING_VERBOSE

PowerSampling::PowerSampling() :
#if (NORDIC_SDK_VERSION >= 11)
		_powerSamplingStartTimerId(NULL),
		_powerSamplingReadTimerId(NULL),
#else
		_powerSamplingStartTimerId(UINT32_MAX),
		_powerSamplingReadTimerId(UINT32_MAX),
#endif
		_powerSamplesBuffer(NULL),
		_currentSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
		_voltageSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
		_powerSamplesContMsg(NULL),
		_powerSamplesCount(0)
{
#if (NORDIC_SDK_VERSION >= 11)
	_powerSamplingStartTimerData = { {0} };
	_powerSamplingStartTimerId = &_powerSamplingStartTimerData;
	_powerSamplingReadTimerData = { {0} };
	_powerSamplingReadTimerId = &_powerSamplingReadTimerData;
#endif

}

// adc done callback is already decoupled from adc interrupt
void adc_done_callback(nrf_saadc_value_t* buf, uint16_t size, uint8_t bufNum) {
	PowerSampling::getInstance().powerSampleFinish(buf, size, bufNum);
}

void PowerSampling::init() {
#if (HARDWARE_BOARD==PCA10040)
	nrf_gpio_cfg_output(PIN_GPIO_LED_3);
	nrf_gpio_cfg_output(PIN_GPIO_LED_4);
	nrf_gpio_pin_set(PIN_GPIO_LED_3);
	nrf_gpio_pin_set(PIN_GPIO_LED_4);
#endif

	Timer::getInstance().createSingleShot(_powerSamplingStartTimerId, (app_timer_timeout_handler_t)PowerSampling::staticPowerSampleStart);
	Timer::getInstance().createSingleShot(_powerSamplingReadTimerId, (app_timer_timeout_handler_t)PowerSampling::staticPowerSampleRead);

	Settings& settings = Settings::getInstance();
	settings.get(CONFIG_POWER_SAMPLE_BURST_INTERVAL, &_burstSamplingInterval);
	settings.get(CONFIG_POWER_SAMPLE_CONT_INTERVAL, &_contSamplingInterval);
	settings.get(CONFIG_VOLTAGE_MULTIPLIER, &_voltageMultiplier);
	settings.get(CONFIG_CURRENT_MULTIPLIER, &_currentMultiplier);
	settings.get(CONFIG_VOLTAGE_ZERO, &_voltageZero);
	settings.get(CONFIG_CURRENT_ZERO, &_currentZero);
	settings.get(CONFIG_POWER_ZERO, &_powerZero);
	settings.get(CONFIG_POWER_ZERO_AVG_WINDOW, &_zeroAvgWindow);
	// Octave: a=0.05; x=[0:1000]; y=(1-a).^x; y2=cumsum(y)*a; figure(1); plot(x,y); figure(2); plot(x,y2); find(y2 > 0.99)(1)
	_avgZeroDiscount = 0.02; //! 99% of the value is influenced by the last 228 values
	_avgPowerDiscount = 0.05; //! 99% of the value is influenced by the last 90 values
	_avgZeroInitialized = false;
	_avgPowerInitialized = false;


	LOGi(FMT_INIT, "buffers");
	uint16_t contSize = _currentSampleCircularBuf.getMaxByteSize() + _voltageSampleCircularBuf.getMaxByteSize();
	contSize += sizeof(power_samples_cont_message_t);

	uint16_t burstSize = _powerSamples.getMaxLength();

	_burstCount = 0;
	_voltageZero = 0.0;

	size_t size = (burstSize > contSize) ? burstSize : contSize;
	_powerSamplesBuffer = (buffer_ptr_t) calloc(size, sizeof(uint8_t));
#ifdef PRINT_POWERSAMPLING_VERBOSE
	LOGd("power sample buffer=%u size=%u", _powerSamplesBuffer, size);
#endif

#if CONTINUOUS_POWER_SAMPLER == 1
	buffer_ptr_t buffer = _powerSamplesBuffer;
	_currentSampleCircularBuf.assign(buffer, _currentSampleCircularBuf.getMaxByteSize());
	buffer += _currentSampleCircularBuf.getMaxByteSize();
	_voltageSampleCircularBuf.assign(buffer, _voltageSampleCircularBuf.getMaxByteSize());
	buffer += _voltageSampleCircularBuf.getMaxByteSize();
	_powerSamplesContMsg = (power_samples_cont_message_t*) buffer;
	buffer += sizeof(power_samples_cont_message_t);
//	_currentSampleTimestamps.assign();
//	_voltageSampleTimestamps.assign();

#else
	_powerSamples.assign(_powerSamplesBuffer, size);
#endif

	uint8_t pins[] = {PIN_AIN_CURRENT, PIN_AIN_VOLTAGE};
	ADC::getInstance().init(pins, 2);

#if CONTINUOUS_POWER_SAMPLER == 1
	ADC::getInstance().setBuffers(&_currentSampleCircularBuf, 0);
	ADC::getInstance().setBuffers(&_voltageSampleCircularBuf, 1);
//	ADC::getInstance().setTimestampBuffers(&_currentSampleTimestamps, 0);
//	ADC::getInstance().setTimestampBuffers(&_voltageSampleTimestamps, 1);
#else
	ADC::getInstance().setBuffers(_powerSamples.getCurrentSamplesBuffer(), 0);
	ADC::getInstance().setBuffers(_powerSamples.getVoltageSamplesBuffer(), 1);
	ADC::getInstance().setTimestampBuffers(_powerSamples.getCurrentTimestampsBuffer(), 0);
	ADC::getInstance().setTimestampBuffers(_powerSamples.getVoltageTimestampsBuffer(), 1);
	ADC::getInstance().setDoneCallback(adc_done_callback);
//	Timer::getInstance().start(_staticPowerSamplingStartTimer, MS_TO_TICKS(_burstSamplingInterval), this);
#endif
}

void PowerSampling::startSampling() {
#ifdef PRINT_POWERSAMPLING_VERBOSE
	LOGd(FMT_START, "power sample");
#endif

#if CONTINUOUS_POWER_SAMPLER == 1
	_currentSampleCircularBuf.clear();
	_voltageSampleCircularBuf.clear();
	_powerSamplesCount = 0;
//	_currentSampleTimestamps.clear();
//	_voltageSampleTimestamps.clear();
	Timer::getInstance().start(_powerSamplingReadTimerId, MS_TO_TICKS(_contSamplingInterval), this);
#else
	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_START);
	_powerSamples.clear();
#endif
	ADC::getInstance().start();
}

void PowerSampling::stopSampling() {
	// todo:
}

void PowerSampling::powerSampleReadBuffer() {

	//! If one of the buffers is full, this means they're not aligned anymore: clear all.
	if (_currentSampleCircularBuf.full() || _voltageSampleCircularBuf.full()) {
		startSampling();
		Timer::getInstance().start(_powerSamplingReadTimerId, MS_TO_TICKS(_contSamplingInterval), this);
		return;
	}

	uint16_t numCurrentSamples = _currentSampleCircularBuf.size();
	uint16_t numVoltageSamples = _voltageSampleCircularBuf.size();
	uint16_t numSamples = (numCurrentSamples > numVoltageSamples) ? numVoltageSamples : numCurrentSamples;
	if (numSamples > 0) {
		uint16_t power;
		uint16_t current;
		uint16_t voltage;
//		int32_t diff;
		if (_powerSamplesCount == 0) {
			current = _currentSampleCircularBuf.pop();
			voltage = _voltageSampleCircularBuf.pop();
			power = current*voltage;
			_powerSamplesContMsg->samples[_powerSamplesCount] = power;
//			_lastPowerSample = power;
			numSamples--;
			_powerSamplesCount++;
		}
		for (int i=0; i<numSamples; i++) {
			current = _currentSampleCircularBuf.pop();
			voltage = _voltageSampleCircularBuf.pop();
			power = current*voltage;
			_powerSamplesContMsg->samples[_powerSamplesCount] = power;

//			diff = _lastPowerSample - power;
//			_lastPowerSample = power;
			_powerSamplesCount++;


			if (_powerSamplesCount >= POWER_SAMPLE_CONT_NUM_SAMPLES_MSG) {
				EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_END, _powerSamplesContMsg, sizeof(power_samples_cont_message_t));

#if MESHING == 1
//				if (!nb_full()) {
					MeshControl::getInstance().sendPowerSamplesMessage(_powerSamplesContMsg);
					_powerSamplesCount = 0;
//				}
#else

#ifdef PRINT_POWERSAMPLING_VERBOSE
				LOGd("Send message of %u samples", _powerSamplesCount);
#endif
				_powerSamplesCount = 0;
#endif
			}
		}

	}

	Timer::getInstance().start(_powerSamplingReadTimerId, MS_TO_TICKS(_contSamplingInterval), this);
}




void PowerSampling::powerSampleFinish(nrf_saadc_value_t* buf, uint16_t size, uint8_t bufNum) {
	//! TODO: check if current and voltage samples are of equal length.
	//! If not, one may have been cleared at some point due to too large timestamp difference.

//	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_END, _powerSamplesBuffer, _powerSamples.getDataLength());

#if (HARDWARE_BOARD==PCA10040)
	nrf_gpio_pin_clear(PIN_GPIO_LED_3);
#endif
	calculateZero(buf, size, 2, 0, 1); // Takes 16 us
#if (HARDWARE_BOARD==PCA10040)
	nrf_gpio_pin_set(PIN_GPIO_LED_3);
	nrf_gpio_pin_clear(PIN_GPIO_LED_4);
#endif
	calculatePower(buf, size, 2, 0, 1, 400, 20000); // Takes 18 us
#if (HARDWARE_BOARD==PCA10040)
	nrf_gpio_pin_set(PIN_GPIO_LED_4);
#endif


	EventDispatcher::getInstance().dispatch(STATE_POWER_USAGE, &_avgPowerMiliWatt, 4);

	ADC::getInstance().releaseBuffer(bufNum);
//	//! Start new sample after some time
//	Timer::getInstance().start(_powerSamplingStartTimerId, MS_TO_TICKS(_burstSamplingInterval), this);
}

void PowerSampling::getBuffer(buffer_ptr_t& buffer, uint16_t& size) {
#if CONTINUOUS_POWER_SAMPLER == 1
	buffer = (buffer_ptr_t) _powerSamplesContMsg;
	size = sizeof(power_samples_cont_message_t);
#else
	_powerSamples.getBuffer(buffer, size);
#endif
}





void PowerSampling::calculateZero(nrf_saadc_value_t* buf, uint16_t bufSize, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex) {
	nrf_saadc_value_t vMin = INT16_MAX;
	nrf_saadc_value_t vMax = INT16_MIN;
	nrf_saadc_value_t v;
	for (int i=voltageIndex; i<bufSize; i+=numChannels) {
		v = buf[i];
		if (v > vMax) {
			vMax = v;
		}
		if (v < vMin) {
			vMin = v;
		}
	}
	nrf_saadc_value_t vZero = (vMax - vMin) / 2;

	//! Exponential moving average
	if (_avgZeroInitialized) {
		_avgZeroVoltage = (1.0 - _avgZeroDiscount) * _avgZeroVoltage + _avgZeroDiscount * vZero;
	}
	else {
		_avgZeroVoltage = vZero;
		_avgZeroInitialized = true;
	}
	_avgZeroCurrent = _avgZeroVoltage;
}


void PowerSampling::calculatePower(nrf_saadc_value_t* buf, size_t bufSize, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex, uint32_t sampleIntervalUs, uint32_t acPeriodUs) {
	uint16_t startIndex;
	uint16_t diffIndex;
	if (voltageIndex < currentIndex) {
		startIndex = voltageIndex;
		diffIndex = currentIndex - voltageIndex;
	}
	else {
		startIndex = currentIndex;
		diffIndex = voltageIndex - currentIndex;
	}

	int64_t pSum = 0;
	uint32_t intervalUs = sampleIntervalUs;
	uint16_t numSamples = acPeriodUs / intervalUs; //! 20 ms
	if (bufSize < numSamples*numChannels) {
		//! Should have at least a whole period in a buffer!
		return;
	}
	//! Power = sum(voltage * current * dt) = sum(voltage*current) * dt, since dt doesn't change
	//! Voltage = voltageMeasured * voltageMultiplier
	//! Current = currentMeasured * currentMultiplier
	//! dt = sampleIntervalUs / 1000 / 1000
	//! Power in mW = Power * 1000
	for (int i=startIndex; i<numSamples*numChannels; i+=numChannels) {
		pSum += buf[i] * buf[i+diffIndex]; //! 2^31 / (2^12 * 2^12) = 128 samples before it could overflow
	}
	int32_t powerMiliWatt = pSum * _currentMultiplier * _voltageMultiplier * intervalUs / 1000 - _powerZero;
	if (_avgPowerInitialized) {
		_avgPowerMiliWatt = (1.0 - _avgPowerDiscount) * _avgPowerMiliWatt + _avgPowerDiscount * powerMiliWatt;
	}
	else {
		_avgPowerMiliWatt = powerMiliWatt;
		_avgPowerInitialized = true;
	}
}


