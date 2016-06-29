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

#include <mesh/cs_MeshControl.h>
extern "C" {
	#include <protocol/notification_buffer.h>
}

//#define PRINT_SAMPLE_CURRENT
//#define PRINT_DEBUG

PowerSampling::PowerSampling() :
		_staticPowerSamplingStartTimer(0),
		_staticPowerSamplingReadTimer(0),
		_powerSamplesBuffer(NULL),
		_currentSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
		_voltageSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
		_powerSamplesMeshMsg(NULL),
		_powerSamplesCount(0)
{

}

// adc done callback is already decoupled from adc interrupt
void adc_done_callback() {
	PowerSampling::getInstance().powerSampleFinish();
}

void PowerSampling::init() {

	Timer::getInstance().createSingleShot(_staticPowerSamplingStartTimer, (app_timer_timeout_handler_t)PowerSampling::staticPowerSampleStart);
	Timer::getInstance().createSingleShot(_staticPowerSamplingReadTimer, (app_timer_timeout_handler_t)PowerSampling::staticPowerSampleRead);

	Settings& settings = Settings::getInstance();
	settings.get(CONFIG_VOLTAGE_MULTIPLIER, &_voltageMultiplier);
	settings.get(CONFIG_CURRENT_MULTIPLIER, &_currentMultiplier);
	settings.get(CONFIG_VOLTAGE_ZERO, &_voltageZero);
	settings.get(CONFIG_CURRENT_ZERO, &_currentZero);
	settings.get(CONFIG_POWER_ZERO, &_powerZero);

	LOGi("Init buffers");
	uint16_t contSize = _currentSampleCircularBuf.getMaxByteSize() + _voltageSampleCircularBuf.getMaxByteSize();
	contSize += sizeof(power_samples_mesh_message_t);

	uint16_t burstSize = _powerSamples.getMaxLength();

	size_t size = (burstSize > contSize) ? burstSize : contSize;
	_powerSamplesBuffer = (buffer_ptr_t) calloc(size, sizeof(uint8_t));
	LOGd("power sample buffer=%u size=%u", _powerSamplesBuffer, size);

#if CONTINUOUS_POWER_SAMPLER == 1
	buffer_ptr_t buffer = _powerSamplesBuffer;
	_currentSampleCircularBuf.assign(buffer, _currentSampleCircularBuf.getMaxByteSize());
	buffer += _currentSampleCircularBuf.getMaxByteSize();
	_voltageSampleCircularBuf.assign(buffer, _voltageSampleCircularBuf.getMaxByteSize());
	buffer += _voltageSampleCircularBuf.getMaxByteSize();
#if MESHING == 1
	_powerSamplesMeshMsg = (power_samples_mesh_message_t*) buffer;
	buffer += sizeof(power_samples_mesh_message_t);
#endif
//	_currentSampleTimestamps.assign();
//	_voltageSampleTimestamps.assign();

#else
	_powerSamples.assign(_powerSamplesBuffer, size);
#endif

	LOGi("Init ADC");
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
//	Timer::getInstance().start(_staticPowerSamplingStartTimer, MS_TO_TICKS(POWER_SAMPLE_BURST_INTERVAL), this);
#endif
}

void PowerSampling::startSampling() {
//	LOGd("Start power sample");
#if CONTINUOUS_POWER_SAMPLER == 1
	_currentSampleCircularBuf.clear();
	_voltageSampleCircularBuf.clear();
	_powerSamplesCount = 0;
//	_currentSampleTimestamps.clear();
//	_voltageSampleTimestamps.clear();
	Timer::getInstance().start(_staticPowerSamplingReadTimer, MS_TO_TICKS(POWER_SAMPLE_CONT_INTERVAL), this);
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
	}

	uint16_t numCurentSamples = _currentSampleCircularBuf.size();
	uint16_t numVoltageSamples = _voltageSampleCircularBuf.size();
	uint16_t numSamples = (numCurentSamples > numVoltageSamples) ? numVoltageSamples : numCurentSamples;
	if (numSamples > 0) {
		uint16_t power;
		uint16_t current;
		uint16_t voltage;
		int32_t diff;
		if (_powerSamplesCount == 0) {
			current = _currentSampleCircularBuf.pop();
			voltage = _voltageSampleCircularBuf.pop();
			power = current*voltage;
#if MESHING == 1
			_powerSamplesMeshMsg->samples[_powerSamplesCount] = power;
#endif
//			_lastPowerSample = power;
			numSamples--;
			_powerSamplesCount++;
		}
		for (int i=0; i<numSamples; i++) {
			current = _currentSampleCircularBuf.pop();
			voltage = _voltageSampleCircularBuf.pop();
			power = current*voltage;
#if MESHING == 1
			_powerSamplesMeshMsg->samples[_powerSamplesCount] = power;
#endif

//			diff = _lastPowerSample - power;
//			_lastPowerSample = power;
			_powerSamplesCount++;

		}

		if (_powerSamplesCount >= POWER_SAMPLE_MESH_NUM_SAMPLES) {
#if MESHING == 1
			if (!nb_full()) {
				MeshControl::getInstance().sendPowerSamplesMessage(_powerSamplesMeshMsg);
				_powerSamplesCount = 0;
			}
#else
			LOGd("Send message of %u samples", _powerSamplesCount);
			_powerSamplesCount = 0;
#endif
		}

	}

	Timer::getInstance().start(_staticPowerSamplingReadTimer, MS_TO_TICKS(POWER_SAMPLE_CONT_INTERVAL), this);
}


void PowerSampling::powerSampleFinish() {

	//! TODO: check if current and voltage samples are of equal length.
	//! If not, one may have been cleared at some point due to too large timestamp difference.

	EventDispatcher::getInstance().dispatch(EVT_POWER_SAMPLES_END, _powerSamplesBuffer, _powerSamples.getDataLength());

	uint32_t currentTimestamp = 0;
	uint32_t voltageTimestamp = 0;

//#ifdef PRINT_SAMPLE_CURRENT
//	//! Print samples
//	currentTimestamp = 0;
//	voltageTimestamp = 0;
//	_log(DEBUG, "samples:\r\n");
//	for (int i=0; i<_powerSamples.size(); i++) {
//		_powerSamples.getCurrentTimestampsBuffer()->getValue(currentTimestamp, i);
//		_powerSamples.getVoltageTimestampsBuffer()->getValue(voltageTimestamp, i);
//		_log(DEBUG, "%u %u %u %u,  ", currentTimestamp, (*_powerSamples.getCurrentSamplesBuffer())[i], voltageTimestamp, (*_powerSamples.getVoltageSamplesBuffer())[i]);
//		if (!((i+1) % 3)) {
//			_log(DEBUG, "\r\n");
//		}
//	}
//	_log(DEBUG, "\r\n");
//#endif

#ifdef PRINT_SAMPLE_CURRENT
	currentTimestamp = 0;
	_log(DEBUG, "current samples:\r\n");
	for (int i=0; i<_powerSamples.size(); i++) {
		_powerSamples.getCurrentTimestampsBuffer()->getValue(currentTimestamp, i);
		_log(DEBUG, "%u %u,  ", currentTimestamp, (*_powerSamples.getCurrentSamplesBuffer())[i]);
		if (!((i+1) % 6)) {
			_log(DEBUG, "\r\n");
		}
	}
	voltageTimestamp = 0;
	_log(DEBUG, "\r\nvoltage samples:\r\n");
	for (int i=0; i<_powerSamples.size(); i++) {
		_powerSamples.getVoltageTimestampsBuffer()->getValue(voltageTimestamp, i);
		_log(DEBUG, "%u %u,  ", voltageTimestamp, (*_powerSamples.getVoltageSamplesBuffer())[i]);
		if (!((i+1) % 6)) {
			_log(DEBUG, "\r\n");
		}
	}
	_log(DEBUG, "\r\n");
#endif



	//! Calculate average power consumption in Watt

	currentTimestamp = 0;
	voltageTimestamp = 0;

	uint16_t vMin = UINT16_MAX;
	uint16_t vMax = 0;


	uint16_t v;
	_powerSamples.getVoltageTimestampsBuffer()->getValue(voltageTimestamp, 0);
	uint32_t prevTime = voltageTimestamp;
	uint32_t endTime = prevTime + RTC::msToTicks(20);
	uint16_t vPrev = (*_powerSamples.getVoltageSamplesBuffer())[0];
	int zeroCrossings = 0;
	uint32_t prevZeroCrossingTime;

	double tSum = 0.0;
	double pSum = 0.0;
	for (int i=1; i<_powerSamples.size(); i++) {
		v = (*_powerSamples.getVoltageSamplesBuffer())[i];
		_powerSamples.getVoltageTimestampsBuffer()->getValue(voltageTimestamp, i);

//		LOGd("%u %u %u %u", vPrev, v, prevTime, voltageTimestamp);

//		//! Check for zero crossing
//		if ((vPrev < _voltageZero && v > _voltageZero) || (vPrev > _voltageZero && v < _voltageZero)) {
//			//! Only one crossing can happen in 5ms
//			LOGd("zero at %u", voltageTimestamp);
//			if (zeroCrossings == 0) {
//				zeroCrossings++;
//				prevZeroCrossingTime = voltageTimestamp;
//			}
//			else if (zeroCrossings > 0 && RTC::difference(voltageTimestamp, prevZeroCrossingTime) > RTC::msToTicks(5)) {
//				zeroCrossings++;
//				prevZeroCrossingTime = voltageTimestamp;
//			}
//		}
//
//		//! Integrate between two zero crossings
//		if (0 < zeroCrossings && zeroCrossings <= 1) {
//			uint32_t dt = RTC::difference(voltageTimestamp, prevTime);
//			double dts = dt / (double)RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1); //! seconds
//
//			double voltage = _voltageMultiplier * (v - _voltageZero);
//			double current = _currentMultiplier * ((*_powerSamples.getCurrentSamplesBuffer())[i] - _currentZero);
//			LOGd("%f %f %f", voltage, current, dts);
//			pSum += voltage * current * dts;
//			tSum += dts;
//		}

		//! Integrate whole period, based on 50Hz (20ms)
		if (voltageTimestamp <= endTime) {
			uint32_t dt = RTC::difference(voltageTimestamp, prevTime);
			double dts = dt / (double)RTC_CLOCK_FREQ / (NRF_RTC0->PRESCALER + 1); //! seconds

			double voltage = _voltageMultiplier * (v - _voltageZero);
			double current = _currentMultiplier * ((*_powerSamples.getCurrentSamplesBuffer())[i] - _currentZero);
//			LOGd("%f %f %f", voltage, current, dts);
			pSum += voltage * current * dts;
			tSum += dts;
		}


		prevTime = voltageTimestamp;
		vPrev = v;
	}
//	LOGd("pSum=%f", pSum);
	pSum /= tSum;
	pSum -= _powerZero;
	int32_t avgPower = pSum;

#ifdef PRINT_DEBUG
	LOGd("pSum=%f, tSum=%f, avgPower=%i", pSum, tSum, avgPower);
#endif

	//! Only send valid updates
//	if (zeroCrossings > 1) {
	if (tSum > 0.0) {
		EventDispatcher::getInstance().dispatch(STATE_POWER_USAGE, &avgPower, 4);
	}


	//! Start new sample after some time
	Timer::getInstance().start(_staticPowerSamplingStartTimer, MS_TO_TICKS(POWER_SAMPLE_BURST_INTERVAL), this);
}

void PowerSampling::getBuffer(buffer_ptr_t& buffer, uint16_t& size) {
#if CONTINUOUS_POWER_SAMPLER == 1
#if MESHING == 1
	buffer = (buffer_ptr_t) _powerSamplesMeshMsg;
	size = sizeof(_powerSamplesMeshMsg);
#else
	//! Something silly for now
	buffer = (buffer_ptr_t) _currentSampleCircularBuf.getBuffer();
	size = 0;
#endif
#else
	_powerSamples.getBuffer(buffer, size);
#endif
}
