/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */

#include <processing/cs_PowerSampling.h>

#include <cfg/cs_Boards.h>
#include <cfg/cs_Settings.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_ADC.h>
#include <structs/cs_PowerCurve.h>

#include <protocol/cs_MeshControl.h>
extern "C" {
	#include <protocol/notification_buffer.h>
}

#define POWER_SAMPLING_UPDATE_FREQUENCY 10

//#define PRINT_SAMPLE_CURRENT

PowerSampling::PowerSampling() :
		_samplingTimer(0),
//		_staticPowerSamplingStartTimer(NULL),
		_staticPowerSamplingReadTimer(0),
		_powerSamplesBuffer(NULL),
		_currentSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
		_voltageSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
		_powerSamplesMeshMsg(NULL),
		_powerSamplesCount(0),
		_powerSamplesProcessed(false)
{
	init();
}

void PowerSampling::init() {
	LOGi(">> init");

//	Timer::getInstance().createSingleShot(_staticPowerSamplingStartTimer, (app_timer_timeout_handler_t)PowerSampling::staticPowerSampleStart);
	Timer::getInstance().createSingleShot(_staticPowerSamplingReadTimer, (app_timer_timeout_handler_t)PowerSampling::staticPowerSampleRead);
	Timer::getInstance().createSingleShot(_samplingTimer, (app_timer_timeout_handler_t)PowerSampling::staticTick);
//
//	//! Start sampling
//	powerSampleFirstStart();
}

void PowerSampling::tick() {
	if (!_powerSamplesProcessed && _powerSamples.full()) {
		powerSampleFinish();
	}

	scheduleNextTick();
}

void PowerSampling::scheduleNextTick() {
	Timer::getInstance().start(_samplingTimer, HZ_TO_TICKS(POWER_SAMPLING_UPDATE_FREQUENCY), this);
}

void PowerSampling::powerSampleInit() {

	LOGi(">> power sample init");

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
#if CHAR_MESHING == 1
	_powerSamplesMeshMsg = (power_samples_mesh_message_t*) buffer;
	buffer += sizeof(power_samples_mesh_message_t);
#endif
//	_currentSampleTimestamps.assign();
//	_voltageSampleTimestamps.assign();

#else
	_powerSamples.assign(_powerSamplesBuffer, size);
#endif
}


void PowerSampling::powerSampleFirstStart() {
	LOGi("Init ADC");
	uint8_t pins[] = {PIN_AIN_CURRENT, PIN_AIN_VOLTAGE};
	ADC::getInstance().init(pins, 2);

#if CONTINUOUS_POWER_SAMPLER == 1
	ADC::getInstance().setBuffers(&_currentSampleCircularBuf, 0);
	ADC::getInstance().setBuffers(&_voltageSampleCircularBuf, 1);
//	ADC::getInstance().setTimestampBuffers(&_currentSampleTimestamps, 0);
//	ADC::getInstance().setTimestampBuffers(&_voltageSampleTimestamps, 1);
	powerSampleStart();
#else
	ADC::getInstance().setBuffers(_powerSamples.getCurrentSamplesBuffer(), 0);
	ADC::getInstance().setBuffers(_powerSamples.getVoltageSamplesBuffer(), 1);
	ADC::getInstance().setTimestampBuffers(_powerSamples.getCurrentTimestampsBuffer(), 0);
	ADC::getInstance().setTimestampBuffers(_powerSamples.getVoltageTimestampsBuffer(), 1);
//	Timer::getInstance().start(_staticPowerSamplingStartTimer, MS_TO_TICKS(POWER_SAMPLE_BURST_INTERVAL), this);
	powerSampleStart();
#endif
}


void PowerSampling::powerSampleStart() {
	LOGd("Start power sample");
#if CONTINUOUS_POWER_SAMPLER == 1
	_currentSampleCircularBuf.clear();
	_voltageSampleCircularBuf.clear();
	_powerSamplesCount = 0;
//	_currentSampleTimestamps.clear();
//	_voltageSampleTimestamps.clear();
	Timer::getInstance().start(_staticPowerSamplingReadTimer, MS_TO_TICKS(POWER_SAMPLE_CONT_INTERVAL), this);
#else
	_powerSamples.clear();
	_powerSamplesProcessed = false;
#endif
	ADC::getInstance().start();
}


void PowerSampling::powerSampleReadBuffer() {

	//! If one of the buffers is full, this means they're not aligned anymore: clear all.
	if (_currentSampleCircularBuf.full() || _voltageSampleCircularBuf.full()) {
		powerSampleStart();
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
#if CHAR_MESHING == 1
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
#if CHAR_MESHING == 1
			_powerSamplesMeshMsg->samples[_powerSamplesCount] = power;
#endif

//			diff = _lastPowerSample - power;
//			_lastPowerSample = power;
			_powerSamplesCount++;

		}

		if (_powerSamplesCount >= POWER_SAMPLE_MESH_NUM_SAMPLES) {
#if CHAR_MESHING == 1
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
	LOGd("powerSampleFinish");

	//! TODO: check if current and voltage samples are of equal length.
	//! If not, one may have been cleared at some point due to too large timestamp difference.

	_powerSamplesProcessed = true;
	EventDispatcher::getInstance().dispatch(EVT_POWER_CURVE, _powerSamplesBuffer, _powerSamples.getDataLength());
//	_powerSamplesCharacteristic->setDataLength(_powerSamples.getDataLength());
//	_powerSamplesCharacteristic->notify();

//#if SERIAL_VERBOSITY==DEBUG
//	//! Print samples
//	uint32_t currentTimestamp = 0;
//	uint32_t voltageTimestamp = 0;
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

#if SERIAL_VERBOSITY==DEBUG
	uint32_t currentTimestamp = 0;
	_log(DEBUG, "current samples:\r\n");
	for (int i=0; i<_powerSamples.size(); i++) {
		_powerSamples.getCurrentTimestampsBuffer()->getValue(currentTimestamp, i);
		_log(DEBUG, "%u %u,  ", currentTimestamp, (*_powerSamples.getCurrentSamplesBuffer())[i]);
		if (!((i+1) % 6)) {
			_log(DEBUG, "\r\n");
		}
	}
	uint32_t voltageTimestamp = 0;
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

}


//void PowerSampling::sampleCurrentInit() {
//	LOGi("Start ADC");
//	ADC::getInstance().start();
//
//	//	//! Wait for the ADC to actually start
//	//	nrf_delay_ms(5);
//}

//void PowerSampling::sampleCurrent(uint8_t type) {
//	//! Start storing the samples
//	_voltagePin = false;
//	_powerCurve->clear();
////	ADC::getInstance().setPowerCurve(_powerCurve);
//
//	ADC::getInstance().init(PIN_AIN_CURRENT);
//	ADC::getInstance().start();
//	while (!_powerCurve->isFull()) {
//		while (!NRF_ADC->EVENTS_END) {
//		}
//		//			NRF_ADC->EVENTS_END	= 0;
//		//			LOGd("got sample");
//		if (_voltagePin) {
//			PC_ERR_CODE res = _powerCurve->addVoltage(NRF_ADC->RESULT, RTC::getCount());
////			LOGd("%i %i voltage: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
//			ADC::getInstance().config(PIN_AIN_CURRENT);
//		} else {
//			PC_ERR_CODE res = _powerCurve->addCurrent(NRF_ADC->RESULT, RTC::getCount());
////			LOGd("%i %i current: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
//			ADC::getInstance().config(PIN_AIN_VOLTAGE);
//		}
//		_voltagePin = !_voltagePin;
//		ADC::getInstance().start();
//	} LOGd("sampleCurrentDone");
//	sampleCurrentDone(type);
//}

//void PowerSampling::sampleCurrentDone(uint8_t type) {
////	//! Stop storing the samples
////	ADC::getInstance().setCurrentCurve(NULL);
//	ADC::getInstance().setPowerCurve(NULL);
//
//	uint16_t numSamples = _powerCurve->length();
//	LOGd("numSamples = %i", numSamples);
//	if (type & 0x2) {
//		EventDispatcher::getInstance().dispatch(EVT_POWER_CURVE, NULL, _powerCurve->getDataLength());
//	}
//
//#if SERIAL_VERBOSITY==DEBUG
//	if (numSamples>1) {
//		uint16_t currentSample = 0;
//		uint16_t voltageSample = 0;
//		uint32_t currentTimestamp = 0;
//		uint32_t voltageTimestamp = 0;
//		LOGd("Samples:");
//		for (uint16_t i=0; i<numSamples; ++i) {
//			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
//				break;
//			}
//			_log(DEBUG, "%u %u %u %u,  ", currentTimestamp, currentSample, voltageTimestamp, voltageSample);
//			if (!((i+1) % 3)) {
//				_log(DEBUG, "\r\n");
//			}
//		}
//		_log(DEBUG, "\r\n");
//	}
//#endif
//
//#ifdef MIRCO_VIEW
//	if (numSamples>1) {
//		uint16_t currentSample = 0;
//		uint16_t voltageSample = 0;
//		uint32_t currentTimestamp = 0;
//		uint32_t voltageTimestamp = 0;
//		write("3 [");
//		for (uint16_t i=0; i<numSamples; ++i) {
//			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
//				break;
//			}
//			write("%u %u ", currentTimestamp, currentSample);
////			write("%u %u %u %u ", currentTimestamp, currentSample, voltageTimestamp, voltageSample);
//		}
//	}
//#endif
//
//	if (numSamples > 1 && (type & 0x1)) {
//		uint16_t currentSample = 0;
//		uint16_t voltageSample = 0;
//		uint32_t currentTimestamp = 0;
//		uint32_t voltageTimestamp = 0;
//		uint32_t minCurrentSample = 1024;
//		uint32_t maxCurrentSample = 0;
//		for (uint16_t i = 0; i < numSamples; ++i) {
//			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
//				break;
//			}
//			if (currentSample > maxCurrentSample) {
//				maxCurrentSample = currentSample;
//			}
//			if (currentSample < minCurrentSample) {
//				minCurrentSample = currentSample;
//			}
//
//			//! TODO: do some power = voltage * current, make sure we use timestamps too.
//		}
//		//! 1023*1000*1200 = 1.3e9 < 4.3e9 (max uint32)
//		//! currentSampleAmplitude is max: 1023*1000*1200/1023/2 = 600000
//		uint32_t currentSampleAmplitude = (maxCurrentSample - minCurrentSample) * 1000 * 1200 / 1023 / 2;    //! uV
//		//! max of currentAmplitude depends on amplification and shunt value
//		uint32_t currentAmplitude = currentSampleAmplitude / VOLTAGE_AMPLIFICATION / SHUNT_VALUE;    //! mA
//		//! currentRms should be max:  16000
//		uint32_t currentRms = currentAmplitude * 1000 / 1414;    //! mA
//		LOGi("currentRms = %i mA", currentRms);
//		uint32_t powerRms = currentRms * 230 / 1000;    //! Watt
//		LOGi("powerRms = %i Watt", powerRms);
//
//		EventDispatcher::getInstance().dispatch(EVT_POWER_CONSUMPTION, &powerRms, 4);
//	}
//
//}


//void PowerSampling::startSampling(uint8_t type) {
//	_sampling = true;
//	_samplingType = type;
//	LOGi("start sampling");
//
//	_startTimestamp = 0;
//
//	Timer::getInstance().start(_samplingTimer, 5, this);
//}

//void PowerSampling::stopSampling() {
//	LOGi("stop sampling");
//	_sampling = false;
//}

//void PowerSampling::sampleCurrent() {
//
//	if (!_sampling) {
//		return;
//	}
//	if (!_startTimestamp) {
//
//		// Start storing the samples
//		_voltagePin = false;
//		_powerCurve->clear();
//		ADC::getInstance().setPowerCurve(_powerCurve);
//
//		ADC::getInstance().init(PIN_AIN_CURRENT);
//		ADC::getInstance().start();
//
//		_startTimestamp = RTC::getCount();
//
//	} else if (_powerCurve->isFull()) {
//
//		LOGw("sampling duration: %u", RTC::ticksToMs(RTC::difference(RTC::getCount(), _startTimestamp)));
//
//		uint32_t sampleDone = RTC::getCount();
//
//		sampleCurrentDone();
//
//		LOGw("sampling done duration: %u", RTC::ticksToMs(RTC::difference(RTC::getCount(), sampleDone)));
//
//		if (_sampling && !(_samplingType & POWER_SAMPLING_ONE_SHOT_MASK)) {
//			int32_t pause = MS_TO_TICKS(_samplingInterval) - RTC::difference(RTC::getCount(), _startTimestamp);
//			pause = pause < 5 ? 5 : pause;
//			LOGw("pause: %u", RTC::ticksToMs(pause));
//			Timer::getInstance().start(_samplingTimer, pause, this);
//			_startTimestamp = 0;
//		}
//		return;
//	} else {
//		if (NRF_ADC->EVENTS_END) {
//			if (_voltagePin) {
//				PC_ERR_CODE res = _powerCurve->addVoltage(NRF_ADC->RESULT, RTC::getCount());
//	//			LOGd("%i %i voltage: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
//				ADC::getInstance().config(PIN_AIN_CURRENT);
//			}
//			else {
//				PC_ERR_CODE res = _powerCurve->addCurrent(NRF_ADC->RESULT, RTC::getCount());
//	//			LOGd("%i %i current: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
//				ADC::getInstance().config(PIN_AIN_VOLTAGE);
//			}
//			_voltagePin = !_voltagePin;
//			ADC::getInstance().start();
//		}
//	}
//
//	Timer::getInstance().start(_samplingTimer, 5, this);
//
//}

//void PowerService::sampleCurrent() {
//
//	if (!_sampling) {
//		return;
//	}
//	uint32_t start = RTC::now();
//
//	LOGw("sample current");
//
//	// Start storing the samples
//	_voltagePin = false;
//	_powerCurve->clear();
////	ADC::getInstance().setPowerCurve(_powerCurve);
//
//	ADC::getInstance().init(PIN_AIN_CURRENT);
//	ADC::getInstance().start();
//	while (!_powerCurve->isFull()) {
//		while(!NRF_ADC->EVENTS_END) {}
//		//			NRF_ADC->EVENTS_END	= 0;
//		//			LOGd("got sample");
//		if (_voltagePin) {
//			PC_ERR_CODE res = _powerCurve->addVoltage(NRF_ADC->RESULT, RTC::getCount());
////			LOGd("%i %i voltage: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
//			ADC::getInstance().config(PIN_AIN_CURRENT);
//		}
//		else {
//			PC_ERR_CODE res = _powerCurve->addCurrent(NRF_ADC->RESULT, RTC::getCount());
////			LOGd("%i %i current: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
//			ADC::getInstance().config(PIN_AIN_VOLTAGE);
//		}
//		_voltagePin = !_voltagePin;
//		ADC::getInstance().start();
//	}
//
//	uint32_t done = RTC::now();
//	LOGw("sample dt: %d", done- start);
//
//	sampleCurrentDone();
//
//	if (_sampling) {
//		int32_t pause = _samplingInterval - (RTC::now() - start);
//		pause = pause < 1 ? 1 : pause;
//		LOGw("pause: %d", pause);
//		Timer::getInstance().start(_staticSamplingTimer, MS_TO_TICKS(pause), this);
//	}
//}

//void PowerSampling::sampleCurrentDone() {
//
//	LOGw("sample current done");
//
////	uint32_t done = RTC::now();
////	//! Stop storing the samples
////	ADC::getInstance().setCurrentCurve(NULL);
//	ADC::getInstance().setPowerCurve(NULL);
//
//	uint16_t numSamples = _powerCurve->length();
//	LOGd("numSamples = %i", numSamples);
//	if (_samplingType & POWER_SAMPLING_CURVE_MASK) {
//		EventDispatcher::getInstance().dispatch(EVT_POWER_CURVE, NULL, _powerCurve->getDataLength());
//	}
//
//#ifdef PRINT_SAMPLE_CURRENT
//	if (numSamples>1) {
//		uint16_t currentSample = 0;
//		uint16_t voltageSample = 0;
//		uint32_t currentTimestamp = 0;
//		uint32_t voltageTimestamp = 0;
//		LOGd("Samples:");
//		for (uint16_t i=0; i<numSamples; ++i) {
//			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
//				break;
//			}
//			_log(DEBUG, "%u %u %u %u,  ", currentTimestamp, currentSample, voltageTimestamp, voltageSample);
//			if (!((i+1) % 3)) {
//				_log(DEBUG, "\r\n");
//			}
//		}
//		_log(DEBUG, "\r\n");
//	}
//#endif
//
//#ifdef MIRCO_VIEW
//	if (numSamples>1) {
//		uint16_t currentSample = 0;
//		uint16_t voltageSample = 0;
//		uint32_t currentTimestamp = 0;
//		uint32_t voltageTimestamp = 0;
//		write("3 [");
//		for (uint16_t i=0; i<numSamples; ++i) {
//			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
//				break;
//			}
//			write("%u %u ", currentTimestamp, currentSample);
////			write("%u %u %u %u ", currentTimestamp, currentSample, voltageTimestamp, voltageSample);
//		}
//	}
//#endif
//
//	if (numSamples > 1 && (_samplingType & POWER_SAMPLING_AVERAGE_MASK)) {
//		uint16_t currentSample = 0;
//		uint16_t voltageSample = 0;
//		uint32_t currentTimestamp = 0;
//		uint32_t voltageTimestamp = 0;
//		uint32_t minCurrentSample = 1024;
//		uint32_t maxCurrentSample = 0;
//		for (uint16_t i=0; i<numSamples; ++i) {
//			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
//				break;
//			}
//			if (currentSample > maxCurrentSample) {
//				maxCurrentSample = currentSample;
//			}
//			if (currentSample < minCurrentSample) {
//				minCurrentSample = currentSample;
//			}
//
//			//! TODO: do some power = voltage * current, make sure we use timestamps too.
//		}
//		//! 1023*1000*1200 = 1.3e9 < 4.3e9 (max uint32)
//		//! currentSampleAmplitude is max: 1023*1000*1200/1023/2 = 600000
//		uint32_t currentSampleAmplitude = (maxCurrentSample-minCurrentSample) * 1000 * 1200 / 1023 / 2; //! uV
//		//! max of currentAmplitude depends on amplification and shunt value
//		uint32_t currentAmplitude = currentSampleAmplitude / VOLTAGE_AMPLIFICATION / SHUNT_VALUE; //! mA
//		//! currentRms should be max:  16000
//		uint32_t currentRms = currentAmplitude * 1000 / 1414; //! mA
//		LOGi("currentRms = %i mA", currentRms);
//		uint32_t powerRms = currentRms * 230 / 1000; //! Watt
//		LOGi("powerRms = %i Watt", powerRms);
//
//		EventDispatcher::getInstance().dispatch(EVT_POWER_CONSUMPTION, &powerRms, 4);
//	}
//}


void PowerSampling::getBuffer(buffer_ptr_t& buffer, uint16_t& size) {
#if CONTINUOUS_POWER_SAMPLER == 1
#if CHAR_MESHING == 1
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
