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
#include <storage/cs_State.h>

#if BUILD_MESHING == 1
#include <mesh/cs_MeshControl.h>
//extern "C" {
//	#include <notification_buffer.h>
//}
#endif

//#define USE_LED_DEBUG

#ifdef USE_LED_DEBUG
#define PIN_GPIO_LED_2 18
#define PIN_GPIO_LED_3 19
#define PIN_GPIO_LED_4 20
#endif

#define PRINT_POWERSAMPLING_VERBOSE
//#define POWERSAMPLING_ABOVE_ZERO_ONLY

PowerSampling::PowerSampling() :
#if (NORDIC_SDK_VERSION >= 11)
//		_powerSamplingStartTimerId(NULL),
		_powerSamplingSentDoneTimerId(NULL),
#else
//		_powerSamplingStartTimerId(UINT32_MAX),
		_powerSamplingSentDoneTimerId(UINT32_MAX),
#endif
		_powerSamplesBuffer(NULL),
//		_currentSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
//		_voltageSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
		_powerSamplesContMsg(NULL)
//		_powerSamplesCount(0)
{
#if (NORDIC_SDK_VERSION >= 11)
//	_powerSamplingStartTimerData = { {0} };
//	_powerSamplingStartTimerId = &_powerSamplingStartTimerData;
	_powerSamplingReadTimerData = { {0} };
	_powerSamplingSentDoneTimerId = &_powerSamplingReadTimerData;
#endif

}

// adc done callback is already decoupled from adc interrupt
void adc_done_callback(nrf_saadc_value_t* buf, uint16_t size, uint8_t bufNum) {
	PowerSampling::getInstance().powerSampleAdcDone(buf, size, bufNum);
}

void PowerSampling::init(uint8_t pinAinCurrent, uint8_t pinAinVoltage) {
#ifdef USE_LED_DEBUG
	nrf_gpio_cfg_output(PIN_GPIO_LED_2);
	nrf_gpio_cfg_output(PIN_GPIO_LED_3);
	nrf_gpio_cfg_output(PIN_GPIO_LED_4);
	nrf_gpio_pin_set(PIN_GPIO_LED_2);
	nrf_gpio_pin_set(PIN_GPIO_LED_3);
	nrf_gpio_pin_set(PIN_GPIO_LED_4);
#endif

//	Timer::getInstance().createSingleShot(_powerSamplingStartTimerId, (app_timer_timeout_handler_t)PowerSampling::staticPowerSampleStart);
	Timer::getInstance().createSingleShot(_powerSamplingSentDoneTimerId, (app_timer_timeout_handler_t)PowerSampling::staticPowerSampleRead);

	Settings& settings = Settings::getInstance();
	settings.get(CONFIG_POWER_SAMPLE_BURST_INTERVAL, &_burstSamplingInterval);
	settings.get(CONFIG_POWER_SAMPLE_CONT_INTERVAL, &_contSamplingInterval);
	settings.get(CONFIG_VOLTAGE_MULTIPLIER, &_voltageMultiplier);
	settings.get(CONFIG_CURRENT_MULTIPLIER, &_currentMultiplier);
	settings.get(CONFIG_VOLTAGE_ZERO, &_voltageZero);
	settings.get(CONFIG_CURRENT_ZERO, &_currentZero);
	settings.get(CONFIG_POWER_ZERO, &_powerZero);
//	settings.get(CONFIG_POWER_ZERO_AVG_WINDOW, &_zeroAvgWindow); // No longer used

	initAverages();
	_avgZeroVoltageDiscount = VOLTAGE_ZERO_EXP_AVG_DISCOUNT;
	_avgZeroCurrentDiscount = CURRENT_ZERO_EXP_AVG_DISCOUNT;
	_avgPowerDiscount = POWER_EXP_AVG_DISCOUNT;
	_sendingSamples = false;


	LOGi(FMT_INIT, "buffers");
//	uint16_t contSize = _currentSampleCircularBuf.getMaxByteSize() + _voltageSampleCircularBuf.getMaxByteSize();
//	contSize += sizeof(power_samples_cont_message_t);
	uint16_t contSize = sizeof(power_samples_cont_message_t);

	uint16_t burstSize = _powerSamples.getMaxLength();

//	_burstCount = 0;

	size_t size = (burstSize > contSize) ? burstSize : contSize;
	_powerSamplesBuffer = (buffer_ptr_t) calloc(size, sizeof(uint8_t));
#ifdef PRINT_POWERSAMPLING_VERBOSE
	LOGd("power sample buffer=%u size=%u", _powerSamplesBuffer, size);
#endif

#if CONTINUOUS_POWER_SAMPLER == 1
	buffer_ptr_t buffer = _powerSamplesBuffer;
//	_currentSampleCircularBuf.assign(buffer, _currentSampleCircularBuf.getMaxByteSize());
//	buffer += _currentSampleCircularBuf.getMaxByteSize();
//	_voltageSampleCircularBuf.assign(buffer, _voltageSampleCircularBuf.getMaxByteSize());
//	buffer += _voltageSampleCircularBuf.getMaxByteSize();
	_powerSamplesContMsg = (power_samples_cont_message_t*) buffer;
	buffer += sizeof(power_samples_cont_message_t);
//	_currentSampleTimestamps.assign();
//	_voltageSampleTimestamps.assign();

#else
	_powerSamples.assign(_powerSamplesBuffer, size);
#endif

	const pin_id_t pins[] = {pinAinCurrent, pinAinVoltage};
	ADC::getInstance().init(pins, 2);

#if CONTINUOUS_POWER_SAMPLER == 1
//	ADC::getInstance().setBuffers(&_currentSampleCircularBuf, 0);
//	ADC::getInstance().setBuffers(&_voltageSampleCircularBuf, 1);
//	ADC::getInstance().setTimestampBuffers(&_currentSampleTimestamps, 0);
//	ADC::getInstance().setTimestampBuffers(&_voltageSampleTimestamps, 1);
#else
//	ADC::getInstance().setBuffers(_powerSamples.getCurrentSamplesBuffer(), 0);
//	ADC::getInstance().setBuffers(_powerSamples.getVoltageSamplesBuffer(), 1);
//	ADC::getInstance().setTimestampBuffers(_powerSamples.getCurrentTimestampsBuffer(), 0);
//	ADC::getInstance().setTimestampBuffers(_powerSamples.getVoltageTimestampsBuffer(), 1);
	ADC::getInstance().setDoneCallback(adc_done_callback);
//	Timer::getInstance().start(_staticPowerSamplingStartTimer, MS_TO_TICKS(_burstSamplingInterval), this);
#endif
}

void PowerSampling::startSampling() {
#ifdef PRINT_POWERSAMPLING_VERBOSE
	LOGd(FMT_START, "power sample");
#endif
	// Get operation mode
	State::getInstance().get(STATE_OPERATION_MODE, _operationMode);

#if CONTINUOUS_POWER_SAMPLER == 1
//	_currentSampleCircularBuf.clear();
//	_voltageSampleCircularBuf.clear();
//	_powerSamplesCount = 0;
//	_currentSampleTimestamps.clear();
//	_voltageSampleTimestamps.clear();
//	Timer::getInstance().start(_powerSamplingReadTimerId, MS_TO_TICKS(_contSamplingInterval), this);
#else
	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_START);
	_powerSamples.clear();
#endif
	ADC::getInstance().start();
}

void PowerSampling::stopSampling() {
	// todo:
}

void PowerSampling::sentDone() {
	_sendingSamples = false;
}




void PowerSampling::powerSampleAdcDone(nrf_saadc_value_t* buf, uint16_t size, uint8_t bufNum) {
//	// Connect pinAinCurrent to GND and pinAinVoltage to VDD to perform this test.
//	for (int i=1; i<size; i++) {
//		if (i % 2 == 0) {
//			if (!(buf[i] < 4000)) {
//				write("buf: ");
//				for (int j=0; j<size; j++) {
//					write("%d ", buf[j]);
//					if ((j+1) % 30 == 0) write("\r\n");
//				}
//				write("\r\n");
//			}
//			assert(buf[i] < 4000, "wrong pin!");
//		}
//	}
//	LOGd("buf=%d size=%d bufNum=%d", buf, size, bufNum);
//	write("%d %d %d ... %d\r\n", buf[0], buf[1], buf[2], buf[size-1]);

	uint16_t voltageIndex = 1;
	uint16_t currentIndex = 0;
	uint16_t numChannels = 2;

#ifdef POWERSAMPLING_ABOVE_ZERO_ONLY
	currentIndex = determineCurrentIndex(buf, size, numChannels, voltageIndex, currentIndex, CS_ADC_SAMPLE_INTERVAL_US, 20000);
	if (currentIndex == 1) {
		voltageIndex = 0;
	}
#endif

#ifdef USE_LED_DEBUG
	nrf_gpio_pin_toggle(PIN_GPIO_LED_3);
#endif
	calculateVoltageZero(buf, size, numChannels, voltageIndex, currentIndex, CS_ADC_SAMPLE_INTERVAL_US, 20000); // Takes 23 us
	calculateCurrentZero(buf, size, numChannels, voltageIndex, currentIndex, CS_ADC_SAMPLE_INTERVAL_US, 20000);
#ifdef USE_LED_DEBUG
	nrf_gpio_pin_toggle(PIN_GPIO_LED_3);
#endif
	calculatePower(buf, size, numChannels, voltageIndex, currentIndex, CS_ADC_SAMPLE_INTERVAL_US, 20000); // Takes 17 us
#ifdef USE_LED_DEBUG
	nrf_gpio_pin_toggle(PIN_GPIO_LED_3);
#endif

	if (!_sendingSamples && _operationMode == OPERATION_MODE_NORMAL) {
		copyBufferToPowerSamples(buf, size, numChannels, voltageIndex, currentIndex); // Takes 2 us
#ifdef USE_LED_DEBUG
	nrf_gpio_pin_toggle(PIN_GPIO_LED_3);
#endif
	}

	if (_operationMode == OPERATION_MODE_NORMAL) {
		EventDispatcher::getInstance().dispatch(STATE_POWER_USAGE, &_avgPowerMilliWatt, 4);
	}
#ifdef USE_LED_DEBUG
	nrf_gpio_pin_toggle(PIN_GPIO_LED_3);
#endif
	ADC::getInstance().releaseBuffer(buf);
}

void PowerSampling::getBuffer(buffer_ptr_t& buffer, uint16_t& size) {
#if CONTINUOUS_POWER_SAMPLER == 1
	buffer = (buffer_ptr_t) _powerSamplesContMsg;
	size = sizeof(power_samples_cont_message_t);
#else
	_powerSamples.getBuffer(buffer, size);
#endif
}




void PowerSampling::copyBufferToPowerSamples(nrf_saadc_value_t* buf, uint16_t bufSize, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex) {
#ifdef USE_LED_DEBUG
	nrf_gpio_pin_toggle(PIN_GPIO_LED_4);
#endif
	// First clear the old samples
	// Dispatch event that samples are will be cleared
	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_START);
	_powerSamples.clear();
#ifdef USE_LED_DEBUG
	nrf_gpio_pin_toggle(PIN_GPIO_LED_4);
#endif
	// Use dummy timestamps for now, for backward compatibility
	uint32_t startTime = RTC::getCount(); // Not really the start time
	uint32_t dT = CS_ADC_SAMPLE_INTERVAL_US * RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1) / 1000 / 1000;

	for (int i=0; i<bufSize; i+=numChannels) {
		if (_powerSamples.getCurrentSamplesBuffer()->full() || _powerSamples.getVoltageSamplesBuffer()->full()) {
			readyToSendPowerSamples();
			return;
		}
		_powerSamples.getCurrentSamplesBuffer()->push(buf[i+currentIndex]);
		_powerSamples.getVoltageSamplesBuffer()->push(buf[i+voltageIndex]);
		if (!_powerSamples.getCurrentTimestampsBuffer()->push(startTime + (i/numChannels) * dT) || !_powerSamples.getVoltageTimestampsBuffer()->push(startTime + (i/numChannels) * dT)) {
			_powerSamples.getCurrentSamplesBuffer()->clear();
			_powerSamples.getVoltageSamplesBuffer()->clear();
			return;
		}
	}
#ifdef USE_LED_DEBUG
	nrf_gpio_pin_toggle(PIN_GPIO_LED_4);
#endif
	//! TODO: are we actually ready here?
	readyToSendPowerSamples();
#ifdef USE_LED_DEBUG
	nrf_gpio_pin_toggle(PIN_GPIO_LED_4);
#endif
}


void PowerSampling::readyToSendPowerSamples() {
	//! Mark that the power samples are being sent now
	_sendingSamples = true;
	//! Dispatch event that samples are now filled and ready to be sent
	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_END, _powerSamplesBuffer, _powerSamples.getDataLength());
	//! Simply use an amount of time for sending, should be event based or polling based
	Timer::getInstance().start(_powerSamplingSentDoneTimerId, MS_TO_TICKS(_burstSamplingInterval), this);
}

void PowerSampling::initAverages() {
	_avgZeroVoltage = _voltageZero * 1000;
	_avgZeroCurrent = _currentZero * 1000;
	_avgPower = 0.0;
}

uint16_t PowerSampling::determineCurrentIndex(nrf_saadc_value_t* buf, uint16_t length, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex, uint32_t sampleIntervalUs, uint32_t acPeriodUs) {
	uint16_t actualCurrentIndex = currentIndex;
#ifdef POWERSAMPLING_ABOVE_ZERO_ONLY
	uint16_t numSamples = acPeriodUs / sampleIntervalUs; //! one AC period
	for (int i=0; i<numSamples*numChannels; i+=numChannels) {
		if (buf[i+currentIndex] > 2023) {
			actualCurrentIndex = currentIndex;
			break;
		}
		if (buf[i+voltageIndex] > 2023) {
			actualCurrentIndex = voltageIndex;
			break;
		}
	}
#endif
	return actualCurrentIndex;
}

void PowerSampling::calculateVoltageZero(nrf_saadc_value_t* buf, uint16_t bufSize, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex, uint32_t sampleIntervalUs, uint32_t acPeriodUs) {
	//! Assume zero line is the average of all samples
	uint16_t numSamples = acPeriodUs / sampleIntervalUs; //! 20 one AC period
	int64_t sum = 0;
	for (int i=voltageIndex; i<numSamples*numChannels; i+=numChannels) {
		sum += buf[i];
	}
	int32_t vZero = sum / numSamples;

//	//! Assume zero line is in between the min and max values
//	nrf_saadc_value_t vMin = INT16_MAX;
//	nrf_saadc_value_t vMax = INT16_MIN;
//	nrf_saadc_value_t v;
//	for (int i=voltageIndex; i<bufSize; i+=numChannels) {
//		v = buf[i];
//		if (v > vMax) {
//			vMax = v;
//		}
//		if (v < vMin) {
//			vMin = v;
//		}
//	}
//	int32_t vZero = (vMax - vMin) / 2 + vMin;

#ifdef POWERSAMPLING_ABOVE_ZERO_ONLY
	_avgZeroVoltage = vZero * 1000;
#else
	//! Exponential moving average
	_avgZeroVoltage = ((1000 - _avgZeroVoltageDiscount) * _avgZeroVoltage + _avgZeroVoltageDiscount * vZero * 1000) / 1000;
#endif
}


void PowerSampling::calculateCurrentZero(nrf_saadc_value_t* buf, uint16_t bufSize, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex, uint32_t sampleIntervalUs, uint32_t acPeriodUs) {
	//! Assume zero line is the average of all samples
	uint16_t numSamples = acPeriodUs / sampleIntervalUs; //! 20 one AC period
	int64_t sum = 0;
	for (int i=currentIndex; i<numSamples*numChannels; i+=numChannels) {
		sum += buf[i];
	}
	int32_t cZero = sum / numSamples;

//	//! Assume zero line is in between the min and max values
//	nrf_saadc_value_t cMin = INT16_MAX;
//	nrf_saadc_value_t cMax = INT16_MIN;
//	nrf_saadc_value_t c;
//	for (int i=currentIndex; i<bufSize; i+=numChannels) {
//		c = buf[i];
//		if (c > cMax) {
//			cMax = c;
//		}
//		if (c < cMin) {
//			cMin = c;
//		}
//	}
//	int32_t cZero = (cMax - cMin) / 2 + cMin;

#ifdef POWERSAMPLING_ABOVE_ZERO_ONLY
	_avgZeroCurrent = cZero * 1000;
#else
	//! Exponential moving average
	_avgZeroCurrent = ((1000 - _avgZeroCurrentDiscount) * _avgZeroCurrent + _avgZeroCurrentDiscount * cZero * 1000) / 1000;
#endif
}


void PowerSampling::calculatePower(nrf_saadc_value_t* buf, size_t bufSize, uint16_t numChannels, uint16_t voltageIndex, uint16_t currentIndex, uint32_t sampleIntervalUs, uint32_t acPeriodUs) {
	int64_t pSum = 0;
	uint32_t intervalUs = sampleIntervalUs;
	uint16_t numSamples = acPeriodUs / intervalUs; //! one AC period
	if ((int)bufSize < numSamples*numChannels) {
		LOGe("Should have at least a whole period in a buffer!");
		//! Should have at least a whole period in a buffer!
		return;
	}
	//! Power = sum(voltage * current) / numSamples
	//! Voltage = (voltageMeasured - voltageZero) * voltageMultiplier
	//! Current = (currentMeasured - currentZero) * currentMultiplier
	//! dt = sampleIntervalUs / 1000 / 1000
	//! Power in mW = Power * 1000
	for (int i=0; i<numSamples*numChannels; i+=numChannels) {
#ifdef POWERSAMPLING_ABOVE_ZERO_ONLY
		if (buf[i+currentIndex] > _avgZeroCurrent/1000) {
			pSum += (buf[i+currentIndex] - _avgZeroCurrent/1000) * (buf[i+voltageIndex] - _avgZeroVoltage/1000); // 2^63 / (2^12 * 2^12) = many many samples before it could overflow
		}
#else
		pSum += (buf[i+currentIndex] - _avgZeroCurrent/1000) * (buf[i+voltageIndex] - _avgZeroVoltage/1000); // 2^63 / (2^12 * 2^12) = many many samples before it could overflow
#endif
	}
#ifdef POWERSAMPLING_ABOVE_ZERO_ONLY
	int64_t powerMilliWatt = pSum * _currentMultiplier * _voltageMultiplier * 1000 / numSamples * 2 - _powerZero;
#else
	int64_t powerMilliWatt = pSum * _currentMultiplier * _voltageMultiplier * 1000 / numSamples - _powerZero;
#endif

	// TODO: don't use hardcoded 220V here
	if (powerMilliWatt > CURRENT_USAGE_THRESHOLD * 220) {
		LOGd("current above threshold");
		EventDispatcher::getInstance().dispatch(EVT_CURRENT_USAGE_ABOVE_THRESHOLD);
	}
	else if (powerMilliWatt > CURRENT_USAGE_THRESHOLD_PWM  * 220) {
		LOGd("current above pwm threshold");
		EventDispatcher::getInstance().dispatch(EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM);
	}

	//! Exponential moving average
	//! TODO: should maybe make this an integer calculation, but that wasn't working when i tried.
	double discount = _avgPowerDiscount / 1000.0;
//	_avgPower = ROUNDED_DIV((100 - _avgPowerMilliDiscount) * _avgPower + _avgPowerMilliDiscount * powerMilliWatt * 1, 100); // Must be done in int64, or else it can overflow for 4000W
//	_avgPower = ((100 - _avgPowerMilliDiscount) * _avgPower + _avgPowerMilliDiscount * powerMilliWatt * 1) / 100.0; // Must be done in int64, or else it can overflow for 4000W
	_avgPower = ((1.0 - discount) * _avgPower + discount * powerMilliWatt * 1);

//	_avgPowerMilliWatt = _avgZeroVoltage;
//	_avgPowerMilliWatt = _avgZeroCurrent;
//	_avgPowerMilliWatt = pSum;
//	_avgPowerMilliWatt = powerMilliWatt;
	_avgPowerMilliWatt = _avgPower / 1;
}


