/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Mar 23, 2015
 * License: LGPLv3+
 */
#include "cs_Sensors.h"
#include "drivers/cs_Serial.h"
#include "common/cs_Boards.h"
#include "drivers/cs_ADC.h"
#include "drivers/cs_RTC.h"

#include "cs_BluetoothLE.h"

#include "nrf_gpio.h"
#include "nrf_gpiote.h"

//#define LIGHT_SENSOR_ENABLED
//#define THERMAL_SENSOR_ENABLED
#define PUSH_BUTTON_ENABLED
//#define SWITCH_ENABLED

// helper function
#define freqToInterval(x) 1000 / x

// light sensor
#define LIGHT_THRESHOLD 0.2 * MAX_VALUE
#define LIGHT_CHECK_INTERVAL freqToInterval(1) // 1 Hz

// thermistor
#define THERMAL_CHECK_INTERVAL freqToInterval(1) // 1 Hz

// push button
#define PUSH_BUTTON_THRESHOLD 0.9 * MAX_VALUE
#define PUSH_BUTTON_CHECK_INTERVAL freqToInterval(20) // 20 Hz

// switch button
#define SWITCH_THRESHOLD 0.9 * MAX_VALUE
#define SWITCH_CHECK_INTERVAL freqToInterval(20) // 20 Hz

// general
#define ADC_USED defined(LIGHT_SENSOR_ENABLED) || defined(THERMAL_SENSOR_ENABLED)

#define BUTTON_USED defined(PUSH_BUTTON_ENABLED) || defined(SWITCH_ENABLED)




Sensors::Sensors() : _initialized(false),
	_lastLightCheck(0), _lastThermalCheck(0), _lastPushButtonCheck(0), _lastSwitchCheck(0),
	_lastPushed(false), _lastSwitchOn(false)
{
	// NOTHING to do
}

bool Sensors::checkSwitch(uint32_t time, bool &switchOn) {
	if (time - _lastSwitchCheck > 50) {
		uint16_t sample = sampleSensor();

		if (sample == -1) {
			LOGe("failed to read ADC!!");
			return false;
		}

		// do simple thresholding
		if (sample > SWITCH_THRESHOLD) {
			switchOn = true;

			// a push was detected, debounce signal. skip checking for 100 ms
			_lastSwitchCheck = time + 100;
		} else {
			switchOn = false;
			_lastSwitchCheck = time;
		}

		return true;
	}

	return false;
}

bool Sensors::checkPushButton(uint32_t time, bool &pushed) {
	if (time - _lastPushButtonCheck > 50) {
		uint16_t sample = sampleSensor();

		if (sample == -1) {
			LOGe("failed to read ADC!!");
			return false;
		}

		// do simple thresholding
		if (sample > PUSH_BUTTON_THRESHOLD) {
			pushed = true;

			// a push was detected, debounce signal. skip checking for 100 ms
			_lastPushButtonCheck = time + 100;
		} else {
			pushed = false;
			_lastPushButtonCheck = time;
		}

		return true;
	}

	return false;
}


void Sensors::initADC() {
	ADC::getInstance().init(PIN_AIN_SENSOR);

	LOGi("Start RTC");
	RealTimeClock::getInstance().init();
	RealTimeClock::getInstance().start();
	ADC::getInstance().setClock(RealTimeClock::getInstance());

	// Wait for the RTC to actually start
	nrf_delay_ms(1);

	LOGi("Start ADC");
	ADC::getInstance().start();
}

/* Read the ADC to get light intensity values for an LDR (light dependent resistor)
 *
 * returns values 0-1023, where 1023 is max intensity
 *  -1 for error
 */
uint16_t Sensors::sampleSensor() {
	AdcSamples* samples = ADC::getInstance().getSamples();

	// Give some time to sample
	// This is no guarantee that the buffer will be full! (interrupt can happen before we get to lock the buffer)
	while (!samples->full()) {
		nrf_delay_ms(10);
	}

	samples->lock();
	uint16_t numSamples = samples->size();

	if (numSamples) {
		uint32_t average = 0;
		uint32_t timestamp = 0;
		uint16_t voltage = 0;
		samples->getFirstElement(timestamp, voltage);
//		_log(INFO, "%u,  ", voltage);
		do {
//			_log(INFO, "%u,  ", voltage);
//			if (!(++i % 5)) {
//				_log(INFO, "\r\n");
//			}
			average += voltage;
		} while (samples->getNextElement(timestamp, voltage));
//		_log(INFO, "\r\n");

		samples->unlock();

		average /= numSamples;
//		LOGd("average: %d", average);

		// Measured voltage goes from 0-3.6V, measured as 0-1023(10 bit),
		// but Input voltage is from 0-3V (for Nordic EK) and 0-3.3 for Crownstone
		// so we need to rescale the value to have max at 3V (or 3.3V resp)
		// instead of 3.6V
#if(BOARD==CROWNSTONE)
		average *= 3.6/3.3;
#elif(BOARD==PCA10001)
		average *= 3.6/3.0;
#endif

		return average;
	}

	samples->unlock();
	return -1;
}

bool Sensors::checkLightSensor(uint32_t time, uint16_t &value) {
	if (time - _lastLightCheck > 1000) {
		value = sampleSensor();

		if (value == -1) {
			LOGe("failed to read ADC!!");
			return false;
		}

		// convert to percentage
		LOGi("light intensity: %3d%%", (uint16_t)(100.0 * value/MAX_VALUE));
		_lastLightCheck = time;

		return true;
	}

	return false;
}

uint16_t Sensors::checkThermalSensor(uint32_t time) {
	if (time - _lastThermalCheck > 1000) {
		uint16_t thermalIntensity = sampleSensor();

		if (thermalIntensity == -1) {
			LOGe("failed to read ADC!!");
			return -1;
		}

		// convert to percentage
		LOGi("thermal intensity: %3d%%", (uint16_t)(100.0 * thermalIntensity/MAX_VALUE));
		_lastThermalCheck = time;

		return thermalIntensity;
	}

	return -1;
}

void switchPwmSignal() {

	uint8_t pwmChannel;
	uint32_t pwmValue;
	// check first the current pwm value ...
	PWM::getInstance().getValue(pwmChannel, pwmValue);

	// if currently off, turn on, otherwise turn off
	if (pwmValue == 0) {
		PWM::getInstance().setValue(0, 255);
	} else {
		PWM::getInstance().setValue(0, 0);
	}
}

void switchPwmOn() {
	uint8_t pwmChannel;
	uint32_t pwmValue;
	// check first the current pwm value ...
	PWM::getInstance().getValue(pwmChannel, pwmValue);

	// if currently off, turn on, otherwise do nothing
	if (pwmValue == 0) {
		PWM::getInstance().setValue(0, 255);
	}
}

void switchPwmOff() {
	uint8_t pwmChannel;
	uint32_t pwmValue;
	// check first the current pwm value ...
	PWM::getInstance().getValue(pwmChannel, pwmValue);

	// if currently on, turn off, otherwise do nothing
	if (pwmValue == 255) {
		PWM::getInstance().setValue(0, 0);
	}
}

void Sensors::tick() {

	if (!_initialized) {

#if ADC_USED
		initADC();
#endif

		uint8_t pwmChannel;
		uint32_t pwmValue;
		// check first the current pwm value ...
		PWM::getInstance().getValue(pwmChannel, pwmValue);

#ifdef SWITCH_ENABLED
		// update last switch position to current pwm value, so
		// that in case current switch position does not correspond
		// to current pwm value, the pwm value can be updated
		_lastSwitchOn = pwmValue != 0;
#endif

		_initialized = true;

		gpiote_init();
	}

//	LOGi("tick tock");
	uint32_t now = RealTimeClock::now();

#ifdef SWITCH_ENABLED
	bool switchOn = false;

	if (checkSwitch(now, switchOn)) {
		if (switchOn && !_lastSwitchOn) {
			LOGi("switch ON");
			switchPwmOn();
		} else if (!switchOn && _lastSwitchOn) {
			LOGi("switch OFF");
			switchPwmOff();
		}
		_lastSwitchOn = switchOn;
	}
#endif

#ifdef LIGHT_SENSOR_ENABLED
	uint16_t lightValue;

	if (checkLightSensor(now, lightValue)) {
		if (lightValue < LIGHT_THRESHOLD) {
			switchPwmOn();
		} else {
			switchPwmOff();
		}
	}
#endif

#ifdef THERMAL_SENSOR_ENABLED
	checkThermalSensor(now);
#endif

}

#if BUTTON_USED

void Sensors::gpiote_init(void) {
	LOGi("gpio_init");

	/* GPIOTE interrupt handler normally runs in STACK_LOW priority, need to put it
	in APP_LOW in order to use */
	NVIC_SetPriority(GPIOTE_IRQn, 3);
	NVIC_EnableIRQ(GPIOTE_IRQn);

    // #ifdef VERSION 1
	nrf_gpio_cfg_input(PIN_GPIO_BUTTON, NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_input(PIN_GPIO_BUTTON, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
	NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
	// #endif

	// #ifdef VERSION 2 (gives more false events)
	//	nrf_gpiote_event_config(0, PIN_GPIO_BUTTON, NRF_GPIOTE_POLARITY_HITOLO);
	//	NRF_GPIOTE->INTENSET |= GPIOTE_INTENSET_IN0_Enabled << GPIOTE_INTENSET_IN0_Pos;
	// #endif

//	LOGi(".. done");
}

extern "C" void GPIOTE_IRQHandler()
{
//	LOGi("GPIOTE_IRQHandler");

	// #ifdef VERSION 1
	uint32_t pins_state = NRF_GPIO->IN;
	if ((pins_state & (1 << (PIN_GPIO_BUTTON))) == 0)
	{
		LOGi("Button pressed");
	}
	NRF_GPIOTE->EVENTS_PORT = 0;
	// #endif

	// #ifdef VERSION 2
//	if ((NRF_GPIOTE->EVENTS_IN[0] == 1) &&
//		(NRF_GPIOTE->INTENSET & GPIOTE_INTENSET_IN0_Msk))
//	{
//		LOGi("Button pressed %d", ++count);
//		NRF_GPIOTE->EVENTS_IN[0] = 0;
//	}
	// #endif

//	for (int i = 31; i >= 0; --i) {
//		//		LOGi("%d: %d", i, pins_state & (1 << i));
//		_log(INFO, "%d", (pins_state & (1 << i)) != 0 ? 1 : 0);
//		_log(INFO, "%s", i%4 == 0 ? " " : "");
//	}
//	_log(INFO, "\r\n");
//
//	LOGi("... done");

}

#endif
