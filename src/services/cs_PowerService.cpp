/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

/*
//#include <cstdio>
//
#include "services/cs_PowerService.h"
//
#include "cfg/cs_Boards.h"
#include "cfg/cs_Config.h"
#include "cfg/cs_UuidConfig.h"
#include "structs/buffer/cs_MasterBuffer.h"

#include "drivers/cs_ADC.h"
#include "drivers/cs_PWM.h"
#include "drivers/cs_Timer.h"
#include "drivers/cs_LPComp.h"

#include "protocol/cs_Mesh.h"
#include "protocol/cs_MeshControl.h"
#include "cfg/cs_Settings.h"
#include "cfg/cs_StateVars.h"

// change to #def to enable output of sample current values
#undef PRINT_SAMPLE_CURRENT
*/

#include <cstdio>

#include <services/cs_PowerService.h>

#include <cfg/cs_UuidConfig.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_Timer.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_PowerSampling.h>
#include <structs/buffer/cs_MasterBuffer.h>

using namespace BLEpp;

PowerService::PowerService() : EventListener(),
		_pwmCharacteristic(NULL),
		_relayCharacteristic(NULL),
		_sampleCurrentCharacteristic(NULL),
		_powerConsumptionCharacteristic(NULL),
		_currentCurveCharacteristic(NULL)
		_powerSamplesCharacteristic(NULL),
//		_currentLimitCharacteristic(NULL),
		_powerSamplesBuffer(NULL),
		_currentSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
		_voltageSampleCircularBuf(POWER_SAMPLE_CONT_NUM_SAMPLES),
#if CHAR_MESHING == 1
		_powerSamplesMeshMsg(NULL),
#endif
{
	EventDispatcher::getInstance().addListener(this);

	setUUID(UUID(POWER_UUID));

	setName(BLE_SERVICE_POWER);

	addCharacteristics();

	// todo: enable again if tick is needed
//	Timer::getInstance().createSingleShot(_appTimerId, (app_timer_timeout_handler_t)PowerService::staticTick);

	Timer::getInstance().createSingleShot(_staticPowerSamplingStartTimer, (app_timer_timeout_handler_t)PowerService::staticPowerSampleStart);
	Timer::getInstance().createSingleShot(_staticPowerSamplingReadTimer, (app_timer_timeout_handler_t)PowerService::staticPowerSampleRead);

	//! Start sampling
	powerSampleFirstStart();
}

void PowerService::addCharacteristics() {
	LOGi("Create power service");

#if CHAR_PWM==1
	LOGi("add PWM Characteristic");
	addPWMCharacteristic();
#else
	LOGi("skip PWM Characteristic");
#endif

#if CHAR_RELAY==1
	LOGi("add Relay Characteristic");
	addRelayCharacteristic();
#else
	LOGi("skip Relay Characteristic");
#endif

#if CHAR_SAMPLE_CURRENT==1
	powerSampleInit();
//	LOGi("add Sample Current Characteristic");
//	addSampleCurrentCharacteristic();
	LOGi("add Power sample characteristic");
	addPowerSamplesCharacteristic();
	LOGi("add Current Consumption Characteristic");
	addPowerConsumptionCharacteristic();
#else
	LOGi("skip Sample Current Characteristic");
	LOGi("skip Current Curve Characteristic");
	LOGi("skip Current Consumption Characteristic");
#endif

	addCharacteristicsDone();
}

void PowerService::tick() {
	if (!_powerSamplesProcessed && _powerSamples.full()) {
		powerSampleFinish();
	}

	scheduleNextTick();
}

void PowerService::scheduleNextTick() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(POWER_SERVICE_UPDATE_FREQUENCY), this);
}

void PowerService::addPWMCharacteristic() {
	_pwmCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_pwmCharacteristic);
	_pwmCharacteristic->setUUID(UUID(getUUID(), PWM_UUID));
	_pwmCharacteristic->setName(BLE_CHAR_PWM);
	_pwmCharacteristic->setDefaultValue(255);
	_pwmCharacteristic->setWritable(true);
	_pwmCharacteristic->onWrite([&](const uint8_t& value) -> void {
		CommandHandler::getInstance().handleCommand(CMD_PWM, (buffer_ptr_t)&value, 4);
	});
}

void PowerService::addRelayCharacteristic() {

	_relayCharacteristic = new Characteristic<uint8_t>();
	addCharacteristic(_relayCharacteristic);
	_relayCharacteristic->setUUID(UUID(getUUID(), RELAY_UUID));
	_relayCharacteristic->setName(BLE_CHAR_RELAY);
	_relayCharacteristic->setDefaultValue(255);
	_relayCharacteristic->setWritable(true);
	_relayCharacteristic->onWrite([&](const uint8_t& value) -> void {
		CommandHandler::getInstance().handleCommand(CMD_SWITCH, (buffer_ptr_t)&value, 4);
	});
}

void PowerService::addPowerSamplesCharacteristic() {
	_powerSamplesCharacteristic = new Characteristic<uint8_t*>();
	addCharacteristic(_powerSamplesCharacteristic);
	_powerSamplesCharacteristic->setUUID(UUID(getUUID(), POWER_SAMPLES_UUID));
	_powerSamplesCharacteristic->setName("Power Curve");
	_powerSamplesCharacteristic->setWritable(false);
	_powerSamplesCharacteristic->setNotifies(true);

	buffer_ptr_t buffer = NULL;
	uint16_t size = 0;
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
	LOGd("buffer=%u", buffer);

	_powerSamplesCharacteristic->setValue(buffer);
	_powerSamplesCharacteristic->setMaxLength(size);
	_powerSamplesCharacteristic->setDataLength(size);
}

void PowerService::addPowerConsumptionCharacteristic() {
	_powerConsumptionCharacteristic = new Characteristic<uint16_t>();
	addCharacteristic(_powerConsumptionCharacteristic);
	_powerConsumptionCharacteristic->setUUID(UUID(getUUID(), CURRENT_CONSUMPTION_UUID));
	_powerConsumptionCharacteristic->setName("Power Consumption");
	_powerConsumptionCharacteristic->setDefaultValue(0);
	_powerConsumptionCharacteristic->setWritable(false);
	_powerConsumptionCharacteristic->setNotifies(true);
}

void PowerService::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case EVT_POWER_OFF:
	case EVT_POWER_ON: {
		if (_pwmCharacteristic) {
			LOGi("pwm update");
			(*_pwmCharacteristic) = *(uint8_t*)p_data;
		}
		break;
	}
	case EVT_POWER_CURVE: {
		if (_currentCurveCharacteristic) {
			LOGi("current curve update");
			_currentCurveCharacteristic->setDataLength(length);
			_currentCurveCharacteristic->notify();
		}
		break;
	}
	case EVT_POWER_CONSUMPTION: {
		if (_powerConsumptionCharacteristic) {
			LOGi("power consumption update");
			(*_powerConsumptionCharacteristic) = *(uint32_t*)p_data;
		}
	}
	}
}


void PowerService::powerSampleInit() {
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


void PowerService::powerSampleFirstStart() {
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


void PowerService::powerSampleStart() {
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


void PowerService::powerSampleReadBuffer() {

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


void PowerService::powerSampleFinish() {
	LOGd("powerSampleFinish");

	//! TODO: check if current and voltage samples are of equal length.
	//! If not, one may have been cleared at some point due to too large timestamp difference.

	_powerSamplesProcessed = true;
	_powerSamplesCharacteristic->setDataLength(_powerSamples.getDataLength());
	_powerSamplesCharacteristic->notify();

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

	//! Unlock the buffer
	MasterBuffer::getInstance().unlock();
}

