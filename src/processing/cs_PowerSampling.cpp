/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */

#include <processing/cs_PowerSampling.h>

#include <cfg/cs_Boards.h>
#include <drivers/cs_Serial.h>
#include <drivers/cs_ADC.h>
#include <structs/cs_PowerCurve.h>
#include <structs/buffer/cs_MasterBuffer.h>

PowerSampling::PowerSampling() :
		_powerCurve(NULL), _adcInitialized(false), _voltagePin(false)
{
	_powerCurve = new PowerCurve<uint16_t>();

	MasterBuffer& mb = MasterBuffer::getInstance();
	uint8_t *buffer = NULL;
	uint16_t size = 0;
	mb.getBuffer(buffer, size);

	LOGd("Assign buffer of size %i to current curve", size);
	_powerCurve->assign(buffer, size);
}
;

void PowerSampling::sampleCurrentInit() {
	LOGi("Start ADC");
	ADC::getInstance().start();

	//	//! Wait for the ADC to actually start
	//	nrf_delay_ms(5);
}

void PowerSampling::sampleCurrent(uint8_t type) {
	//! Start storing the samples
	_voltagePin = false;
	_powerCurve->clear();
//	ADC::getInstance().setPowerCurve(_powerCurve);

	ADC::getInstance().init(PIN_AIN_CURRENT);
	ADC::getInstance().start();
	while (!_powerCurve->isFull()) {
		while (!NRF_ADC->EVENTS_END) {
		}
		//			NRF_ADC->EVENTS_END	= 0;
		//			LOGd("got sample");
		if (_voltagePin) {
			PC_ERR_CODE res = _powerCurve->addVoltage(NRF_ADC->RESULT, RTC::getCount());
//			LOGd("%i %i voltage: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
			ADC::getInstance().config(PIN_AIN_CURRENT);
		} else {
			PC_ERR_CODE res = _powerCurve->addCurrent(NRF_ADC->RESULT, RTC::getCount());
//			LOGd("%i %i current: %i", res, _powerCurve->length(), NRF_ADC->RESULT);
			ADC::getInstance().config(PIN_AIN_VOLTAGE);
		}
		_voltagePin = !_voltagePin;
		ADC::getInstance().start();
	} LOGd("sampleCurrentDone");
	sampleCurrentDone(type);
}

void PowerSampling::sampleCurrentDone(uint8_t type) {
//	//! Stop storing the samples
//	ADC::getInstance().setCurrentCurve(NULL);
	ADC::getInstance().setPowerCurve(NULL);

	uint16_t numSamples = _powerCurve->length();
	LOGd("numSamples = %i", numSamples);
	if (type & 0x2) {
		EventDispatcher::getInstance().dispatch(EVT_POWER_CURVE, NULL, _powerCurve->getDataLength());
	}

#if SERIAL_VERBOSITY==DEBUG
	if (numSamples>1) {
		uint16_t currentSample = 0;
		uint16_t voltageSample = 0;
		uint32_t currentTimestamp = 0;
		uint32_t voltageTimestamp = 0;
		LOGd("Samples:");
		for (uint16_t i=0; i<numSamples; ++i) {
			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
				break;
			}
			_log(DEBUG, "%u %u %u %u,  ", currentTimestamp, currentSample, voltageTimestamp, voltageSample);
			if (!((i+1) % 3)) {
				_log(DEBUG, "\r\n");
			}
		}
		_log(DEBUG, "\r\n");
	}
#endif

#ifdef MIRCO_VIEW
	if (numSamples>1) {
		uint16_t currentSample = 0;
		uint16_t voltageSample = 0;
		uint32_t currentTimestamp = 0;
		uint32_t voltageTimestamp = 0;
		write("3 [");
		for (uint16_t i=0; i<numSamples; ++i) {
			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
				break;
			}
			write("%u %u ", currentTimestamp, currentSample);
//			write("%u %u %u %u ", currentTimestamp, currentSample, voltageTimestamp, voltageSample);
		}
	}
#endif

	if (numSamples > 1 && (type & 0x1)) {
		uint16_t currentSample = 0;
		uint16_t voltageSample = 0;
		uint32_t currentTimestamp = 0;
		uint32_t voltageTimestamp = 0;
		uint32_t minCurrentSample = 1024;
		uint32_t maxCurrentSample = 0;
		for (uint16_t i = 0; i < numSamples; ++i) {
			if (_powerCurve->getValue(i, currentSample, voltageSample, currentTimestamp, voltageTimestamp) != PC_SUCCESS) {
				break;
			}
			if (currentSample > maxCurrentSample) {
				maxCurrentSample = currentSample;
			}
			if (currentSample < minCurrentSample) {
				minCurrentSample = currentSample;
			}

			//! TODO: do some power = voltage * current, make sure we use timestamps too.
		}
		//! 1023*1000*1200 = 1.3e9 < 4.3e9 (max uint32)
		//! currentSampleAmplitude is max: 1023*1000*1200/1023/2 = 600000
		uint32_t currentSampleAmplitude = (maxCurrentSample - minCurrentSample) * 1000 * 1200 / 1023 / 2;    //! uV
		//! max of currentAmplitude depends on amplification and shunt value
		uint32_t currentAmplitude = currentSampleAmplitude / VOLTAGE_AMPLIFICATION / SHUNT_VALUE;    //! mA
		//! currentRms should be max:  16000
		uint32_t currentRms = currentAmplitude * 1000 / 1414;    //! mA
		LOGi("currentRms = %i mA", currentRms);
		uint32_t powerRms = currentRms * 230 / 1000;    //! Watt
		LOGi("powerRms = %i Watt", powerRms);

		EventDispatcher::getInstance().dispatch(EVT_POWER_CONSUMPTION, &powerRms, 4);
	}

}
