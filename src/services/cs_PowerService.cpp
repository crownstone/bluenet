/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 21, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */


//#include <cmath> // try not to use this!
#include <cstdio>

#include "common/cs_Boards.h"
#include "common/cs_Config.h"
#include "cs_nRF51822.h"
#include "drivers/cs_RTC.h"
#include "services/cs_PowerService.h"

using namespace BLEpp;

PowerService::PowerService(Nrf51822BluetoothStack& _stack) :
		_stack(&_stack), _currentLimitCharacteristic(NULL), _currentLimitVal(0)
{

	setUUID(UUID(POWER_SERVICE_UUID));
	//setUUID(UUID(0x3800)); // there is no BLE_UUID for indoor localization (yet)

//	LOGi("Create power service");
//
//	characStatus.reserve(5);
//	characStatus.push_back( { "PWM",
//			PWM_UUID,
//			true,
//			static_cast<addCharacteristicFunc>(&PowerService::addPWMCharacteristic)});
//	characStatus.push_back( { "Sample Current",
//			SAMPLE_CURRENT_UUID,
//			true,
//			static_cast<addCharacteristicFunc>(&PowerService::addSampleCurrentCharacteristic)});
//	characStatus.push_back( { "Current Curve",
//			CURRENT_CURVE_UUID,
//			true,
//			static_cast<addCharacteristicFunc>(&PowerService::addCurrentCurveCharacteristic)});
//	characStatus.push_back( { "Current Consumption",
//			CURRENT_CONSUMPTION_UUID,
//			true,
//			static_cast<addCharacteristicFunc>(&PowerService::addCurrentConsumptionCharacteristic)});
//	characStatus.push_back( { "Current Limit",
//			CURRENT_LIMIT_UUID,
//			false,
//			static_cast<addCharacteristicFunc>(&PowerService::addCurrentLimitCharacteristic)});

	Storage::getInstance().getHandle(PS_ID_POWER_SERVICE, _storageHandle);
	loadPersistentStorage();

	// we have to figure out why this goes wrong
//	setName(std::string("Power Service"));

//	// set timer with compare interrupt every 10ms
//	timer_config(10);
}

void PowerService::loadPersistentStorage() {
	Storage::getInstance().readStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void PowerService::savePersistentStorage() {
	Storage::getInstance().writeStorage(_storageHandle, &_storageStruct, sizeof(_storageStruct));
}

void PowerService::addPWMCharacteristic() {
	_pwmCharacteristic = createCharacteristicRef<uint8_t>();
	(*_pwmCharacteristic)
		.setUUID(UUID(getUUID(), PWM_UUID))
		.setName("PWM")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t& value) -> void {
//			LOGi("set pwm to %i", value);
			PWM::getInstance().setValue(0, value);
		});
}

// Do we really want to use the PWM for this, or just set the pin to zero?
// TODO: turn off normally, but make sure we enable the completely PWM again on request
void PowerService::turnOff() {
	// update pwm characteristic so that the current value can be read from the characteristic
	(*_pwmCharacteristic) = 0;
	PWM::getInstance().setValue(0, 0);
}

// Do we really want to use the PWM for this, or just set the pin to zero?
// TODO: turn on normally, but make sure we enable the completely PWM again on request
void PowerService::turnOn() {
	// update pwm characteristic so that the current value can be read from the characteristic
	(*_pwmCharacteristic) = 255;
	PWM::getInstance().setValue(0, 255);
}

/**
 * Dim the light, note that we use PWM. You might need another way to dim the light! For example by only turning on for
 * a specific duty-cycle after the detection of a zero crossing.
 */
void PowerService::dim(uint8_t value) {
	// update pwm characteristic so that the current value can be read from the characteristic
	(*_pwmCharacteristic) = value;
	PWM::getInstance().setValue(0, value);
}

void PowerService::addSampleCurrentCharacteristic() {
	createCharacteristic<uint8_t>()
		.setUUID(UUID(getUUID(), SAMPLE_CURRENT_UUID))
		.setName("Sample Current")
		.setDefaultValue(0)
		.setWritable(true)
		.onWrite([&](const uint8_t& value) -> void {
			if (!_adcInitialized) {
				// Init only when you sample, so that the the pin is only configured as AIN after the big spike at startup.
				ADC::getInstance().init(PIN_AIN_ADC);
				sampleCurrentInit();
				_adcInitialized = true;
			}

//			sampleCurrentInit();
			uint16_t current_rms = sampleCurrentFinish(value);
			if ((value & 0x01) && _currentConsumptionCharacteristic != NULL) {
				(*_currentConsumptionCharacteristic) = current_rms;
			}
			if ((value & 0x02) && _currentCurveCharacteristic != NULL) {
				(*_currentCurveCharacteristic) = _currentCurve;
				_currentCurve.clear();
			}
		});
	_adcInitialized = false;
//	ADC::getInstance().init(PIN_AIN_ADC);
}

void PowerService::addCurrentCurveCharacteristic() {
	_currentCurveCharacteristic = &createCharacteristic<CurrentCurve>()
		.setUUID(UUID(getUUID(), CURRENT_CURVE_UUID))
		.setName("Current Curve")
		.setWritable(false)
		.setNotifies(true);
//		.onWrite([&](const uint16_t& value) -> void {
//			sampleVoltageCurveInit();
////			PWM::getInstance().setValue(0, value);
//			sampleVoltageCurveFinish();
//		})
//		.onRead([&] () -> uint16_t {
//			sampleVoltageCurveInit();
//			return sampleVoltageCurveFinish();
//		});
	_currentCurve.init();
}

void PowerService::addCurrentConsumptionCharacteristic() {
//	LOGd("create characteristic to read power consumption");
	_currentConsumptionCharacteristic = &createCharacteristic<uint16_t>()
		.setUUID(UUID(getUUID(), CURRENT_CONSUMPTION_UUID))
		.setName("Current Consumption")
		.setDefaultValue(0)
		.setWritable(false)
		.setNotifies(true);
}

uint8_t PowerService::getCurrentLimit() {
	loadPersistentStorage();
	Storage::getUint8(_storageStruct.current_limit, _currentLimitVal, 0);
	LOGi("Obtained current limit from FLASH: %i", _currentLimitVal);
	return _currentLimitVal;
}

/**
 * The characteristic that writes a current limit to persistent memory.
 *
 * TODO: Check https://devzone.nordicsemi.com/question/1745/how-to-handle-flashwrit-in-an-safe-way/ 
 *       Writing to persistent memory should be done between connection/advertisement events...
 */
void PowerService::addCurrentLimitCharacteristic() {
	_currentLimitCharacteristic = createCharacteristicRef<uint8_t>();
	(*_currentLimitCharacteristic)
		.setNotifies(true)
		.setUUID(UUID(getUUID(), CURRENT_LIMIT_UUID))
		.setName("Current Limit")
		.setDefaultValue(getCurrentLimit())
		.setWritable(true)
		.onWrite([&](const uint8_t &value) -> void {
			LOGi("Set current limit to: %i", value);
			_currentLimitVal = value;
			if (!_currentLimitInitialized) {
				_currentLimit.init();
				_currentLimitInitialized = true;
			}
			_currentLimit.start(&_currentLimitVal);
			LOGi("Write value to persistent memory");
			Storage::setUint8(_currentLimitVal, _storageStruct.current_limit);
			savePersistentStorage();
		});

	// TODO: we have to delay the init, since there is a spike on the AIN pin at startup!
	// For now: init at onWrite, so we can still test it.
//	_currentLimit.start(&_currentLimitVal);
//	_currentLimit.init();
	_currentLimitInitialized = false;
}

//static int tmp_cnt = 100;
//static int tick_cnt = 100;

/**
 * TODO: We should only need to do this once on startup.
 */
void PowerService::tick() {
	LPComp::getInstance().tick();
	// check if current is not beyond current_limit if the latter is set
//	if (++tmp_cnt > tick_cnt) {
//		if (_currentLimitCharacteristic) {
//			getCurrentLimit();
//			LOGi("Set default current limit value to %i", _current_limit_val);
//			*_currentLimitCharacteristic = _current_limit_val;
//		}
//		tmp_cnt = 0;
//	}
}

void PowerService::sampleCurrentInit() {
	/*
	   uint64_t rms_sum = 0;
	   uint32_t voltage_min = 0xffffffff;
	   uint32_t voltage_max = 0;
	// start reading adc
	uint32_t voltage;
	uint32_t samples = 100000;
	//uint32_t subsample = samples / curve_size;
	*/
	//LOGi("Stop advertising");
	//stack->stopAdvertising();

//	ADC::getInstance().getSamples()->clear();

	LOGi("Start RTC");
	RealTimeClock::getInstance().init();
	RealTimeClock::getInstance().start();
	ADC::getInstance().setClock(RealTimeClock::getInstance());

	// Wait for the RTC to actually start
	nrf_delay_ms(1);

	LOGi("Start ADC");
	ADC::getInstance().start();

	// Wait for the ADC to actually start
//	nrf_delay_ms(5);
}

uint16_t PowerService::sampleCurrentFinish(uint8_t type) {
//	while (!ADC::getInstance().getBuffer()->full()) {
//	while (!ADC::getInstance().getSamples()->buf->full()) {
//		nrf_delay_ms(10);
//	}
//	nrf_delay_ms(20);

//	LOGi("Stop ADC");
//	ADC::getInstance().stop();

//	// Wait for the ADC to actually stop
//	nrf_delay_ms(1);

//	LOGi("Stop RTC");
//	RealTimeClock::getInstance().stop();
/*
	for (uint32_t i=0; i<samples; ++i) {

		nrf_adc_read(PIN_ADC, &voltage);

		rms_sum += voltage*voltage;
		if (voltage < voltage_min)
			voltage_min = voltage;
		if (voltage > voltage_max)
			voltage_max = voltage;
		//if (!(i % subsample))
		if (i < curve_size)
			curve[i] = voltage;
	}
	uint32_t rms = sqrt(rms_sum/(100*1000));

	// 8a max --> 0.96v max --> voltage value is max 960000, which is smaller than 2^32

	// measured voltage goes from 0-3.6v(due to 1/3 multiplier), measured as 0-255(8bit) or 0-1024(10bit)
//			voltage = voltage*1000*1200*3/255; // nv   8 bit
	voltage     = voltage    *1000*1200*3/1024; // nv   10 bit
	voltage_min = voltage_min*1000*1200*3/1024;
	voltage_max = voltage_max*1000*1200*3/1024;
	rms         = rms        *1000*1200*3/1024;

	uint16_t current = rms / SHUNT_VALUE; // ma

	LOGd("voltage(nV): last=%lu", voltage);
       	//rms=%lu min=%lu max=%lu current=%i mA", voltage, rms, voltage_min, voltage_max, current);
*/

/*
	uint16_t voltageMax = 0;
	int i = 0;
	while (!ADC::getInstance().getBuffer()->empty()) {
		uint16_t voltage = ADC::getInstance().getBuffer()->pop();

		// First and last elements of the buffer are timestamps
		if ((type & 0x1) && (voltage > voltageMax) && (i>0) && (ADC::getInstance().getBuffer()->size() > 1)) {
			voltageMax = voltage;
		}

		if ((type & 0x2) && _currentCurveCharacteristic != NULL) {
			_currentCurve.add(voltage);
			_log(INFO, "%u, ", voltage);
			if (!(++i % 10)) {
				_log(INFO, "\r\n");
			}
		}
	}
	if (type & 0x2) {
		_log(INFO, "\r\n");
	}

	if (type & 0x1) {
		// measured voltage goes from 0-1.2V, measured as 0-1023(10 bit)
		// 1023 * 1200 / 1023 = 1200 < 2^16
		voltageMax = voltageMax*1200/1023; // mV
		uint16_t currentRms = voltageMax * 1000 / SHUNT_VALUE * 1000 / 1414; // mA
		LOGi("Irms = %u mA\r\n", currentRms);
		return currentRms;
	}
*/



/*
	uint32_t voltageSquareMean = 0;
	uint16_t numSamples = ADC::getInstance().getSamples()->buf->size();
	int i = 0;
	while (!ADC::getInstance().getSamples()->buf->empty()) {
		uint16_t voltage = ADC::getInstance().getSamples()->buf->pop();

		if (type & 0x1) {
			voltageSquareMean += voltage*voltage;
		}

		if ((type & 0x2) && _currentCurveCharacteristic != NULL) {
			_currentCurve.add(voltage);
			_log(INFO, "%u, ", voltage);
			if (!(++i % 10)) {
				_log(INFO, "\r\n");
			}
		}
	}
*/



	AdcSamples* samples = ADC::getInstance().getSamples();
	// Give some time to sample
	// This is no guarantee that the buffer will be full! (interrupt can happen before we get to lock the buffer)
	while (!samples->full()) {
		nrf_delay_ms(10);
	}

	samples->lock();
	uint16_t numSamples = samples->size();
	if ((type & 0x2) && _currentCurveCharacteristic != NULL) {
		// TODO: this is very ugly
		uint8_t* buf = _currentCurve.payload();
		samples->serialize(buf);
		_currentCurve.setPayload(buf, samples->getSerializedLength());

		uint16_t len = samples->getSerializedLength();
		for (uint16_t i=0; i<len; ++i) {
			_log(DEBUG, "%i ", (int8_t)buf[i]);
			if (!((i+1) % 20)) {
				_log(DEBUG, "\r\n");
			}
		}
		_log(DEBUG, "\r\n");
		for (uint16_t i=0; i<len; ++i) {
			_log(DEBUG, "%u ", buf[i]);
			if (!((i+1) % 20)) {
				_log(DEBUG, "\r\n");
			}
		}
		_log(DEBUG, "\r\n");

	}

	LOGd("numSamples=%u", numSamples);
	LOGd("samples->getSerializedLength()=%u", samples->getSerializedLength());
	LOGd("_currentCurve.getMaxLength()=%u", _currentCurve.getMaxLength());
	LOGd("_currentCurve.getSerializedLength()=%u", _currentCurve.getSerializedLength());

	if (numSamples) {
		uint32_t voltageSquareMean = 0;
		uint32_t timestamp = 0;
		uint16_t voltage = 0;
		samples->getFirstElement(timestamp, voltage);
		uint16_t i = 0;
#ifdef MICRO_VIEW
		write("3 [");
#endif
		do {
			_log(INFO, "%u %u,  ", timestamp, voltage);
#ifdef MICRO_VIEW
			write("%u %u ", timestamp, voltage);
#endif
			if (!(++i % 5)) {
				_log(INFO, "\r\n");
			}
			voltageSquareMean += voltage*voltage;
		} while (samples->getNextElement(timestamp, voltage));
		_log(INFO, "\r\n");
		LOGd("i=%u", i);
#ifdef MICRO_VIEW
		write("]r\n");
#endif

		samples->unlock();

		if (type & 0x1) {
			// Take the mean
			voltageSquareMean /= numSamples;

			// Measured voltage goes from 0-1.2V, measured as 0-1023(10 bit)
			// Convert to mV^2
			voltageSquareMean = voltageSquareMean*1200/1023*1200/1023;

			// Convert to A^2, use I=V/R
			uint16_t currentSquareMean = voltageSquareMean * 1000 / SHUNT_VALUE * 1000 / SHUNT_VALUE * 1000*1000;

			LOGi("currentSquareMean = %u mA^2", currentSquareMean);
			return currentSquareMean;
		}
	}
	samples->unlock();
	return 0;
}

PowerService& PowerService::createService(Nrf51822BluetoothStack& stack) {
//	LOGd("Create power service");
	PowerService* svc = new PowerService(stack);
	stack.addService(svc);
//	svc->GenericService::addSpecificCharacteristics();

	svc->addPWMCharacteristic();
	svc->addSampleCurrentCharacteristic();
	svc->addCurrentCurveCharacteristic();
	svc->addCurrentConsumptionCharacteristic();
	svc->addCurrentLimitCharacteristic();


	return *svc;
}

