/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Mar 23, 2015
 * License: LGPLv3+
 */
//#include "cs_BluetoothLE.h"
#include "processing/cs_Sensors.h"
//
#include "cfg/cs_Boards.h"
#include "structs/buffer/cs_MasterBuffer.h"
#include "drivers/cs_Serial.h"
#include "drivers/cs_ADC.h"
#include "drivers/cs_RTC.h"
#include "structs/cs_CurrentCurve.h"

#include <ble/cs_Nordic.h>
#include "drivers/cs_PWM.h"

/* NOTE: only use with BOARDS = CROWNSTONE_SENSOR */

/*
 * ENABLE SENSORS
 * only one GPIO is available right now, so only
 * one sensor can be used at a time
 */
//#define LIGHT_SENSOR_ENABLED
//#define THERMAL_SENSOR_ENABLED
//#define PUSH_BUTTON_ENABLED
#define SWITCH_ENABLED

/* choose which way the switch should be checked
 * 1. interrupt: get an interrupt from the GPIO
 *		handler whenever the signal toggles.
 *		BUT: can register a signal change wrongly if
 *		switch is only moved slightly but doesnt change
 *		position totally. Can be a problem of the switch
 *		being tested
 * 2. polling: check the state of the switch in
 *		every execution of tick and update it accordingly
 *		is not as fast, but does not have the above problem
 */
#ifdef SWITCH_ENABLED
//#define SWITCH_INTERRUPT
#define SWITCH_POLL
#endif

/* choose which way the push button should be checked,
 * see description above for switch
 */
#ifdef PUSH_BUTTON_ENABLED
#define PUSH_BUTTON_INTERRUPT
//#define PUSH_BUTTON_POLL
#endif

/* compiler flag initializes ADC.
 * add additional defined checks if another sensor needs to use
 * the ADC
 */
#define ADC_USED defined(LIGHT_SENSOR_ENABLED) || defined(THERMAL_SENSOR_ENABLED)

/* compiler flag starts interrupt handler
 * add additional defined checks if interrupt handler should be used
 * for other sensors
 */
#define INTERRUPT_USED defined(PUSH_BUTTON_INTERRUPT) || defined(SWITCH_INTERRUPT)

/* Update defines for other boards (instead of adding meaningless PIN definitions
 * to all other boards)
 */
#if (BOARD != CROWNSTONE_SENSOR)
	#ifndef PIN_GPIO_BUTTON
	#define PIN_GPIO_BUTTON 0
	#endif

	#ifndef PIN_AIN_SENSOR
	#define PIN_AIN_SENSOR 0
	#endif
#endif

Sensors::Sensors() : _appTimerId(0), _initialized(false),
	_lastLightCheck(0), _lastThermalCheck(0), _lastPushButtonCheck(0), _lastSwitchCheck(0),
	_lastSwitchOn(false), _lastPushed(false)
{
	Timer::getInstance().createRepeated(_appTimerId, (app_timer_timeout_handler_t)Sensors::staticTick);
}

void Sensors::startTicking() {
	Timer::getInstance().start(_appTimerId, HZ_TO_TICKS(SENSORS_UPDATE_FREQUENCY), this);
}

void Sensors::stopTicking() {
	Timer::getInstance().stop(_appTimerId);
}

#ifdef SWITCH_POLL
/* Check Switch position
 *
 * return true if sensor was checked and switchOn variable
 * contains a valid value
 * return false if sensor was not checked / skipped
 */
bool Sensors::checkSwitch(uint32_t time, bool &switchOn) {
	if (time - _lastSwitchCheck > SWITCH_CHECK_INTERVAL) {
		// check the state of the switch:
		//   ON when signal is HIGH
		//   OFF when signal is LOW
		switchOn = nrf_gpio_pin_read(PIN_GPIO_BUTTON);

		return true;
	}

	return false;
}
#endif

#ifdef PUSH_BUTTON_POLL
/* Check push button state
 *
 * return true if sensor was checked and pushed variable
 * contains a valid value
 * return false if sensor was not checked / skipped
 */
bool Sensors::checkPushButton(uint32_t time, bool &pushed) {
	if (time - _lastPushButtonCheck > PUSH_BUTTON_CHECK_INTERVAL) {
		// check the state of the push button:
		//   PUSHED when signal is HIGH
		//   RELEASED when signal is LOW
		pushed = nrf_gpio_pin_read(PIN_GPIO_BUTTON) == 0;

		return true;
	}

	return false;
}
#endif

/* Initialize ADC for sensor sampling
 */
void Sensors::initADC() {
	ADC::getInstance().init(PIN_AIN_SENSOR);

	LOGi("Start ADC");
	ADC::getInstance().start();
}

/* Read the ADC to get sensor values
 *
 * returns values 0-1023, where 1023 is max value
 *  0xFFFF for error
 */
uint16_t Sensors::sampleSensor() {
	uint16_t result = -1;

	MasterBuffer& mb = MasterBuffer::getInstance();
	if (!mb.isLocked()) {
		mb.lock();
	} else {
		LOGe("Buffer is locked. Cannot be written!");
		return result;
	}

	CurrentCurve<uint16_t> _currentCurve;

	// Start storing the samples
	_currentCurve.clear();
	ADC::getInstance().setCurrentCurve(&_currentCurve);

	// Give some time to sample
	while (!_currentCurve.isFull()) {
		nrf_delay_ms(10);
	}

	// Stop storing the samples
	ADC::getInstance().setCurrentCurve(NULL);

	uint16_t numSamples = _currentCurve.length();
	LOGd("numSamples = %i", numSamples);

	if (numSamples>1) {
		uint32_t timestamp = 0;
		uint16_t voltage = 0;
		uint16_t average = 0;

		for (uint16_t i=0; i<numSamples; ++i) {
			if (_currentCurve.getValue(i, voltage, timestamp) != CC_SUCCESS) {
				break;
			}
			//			_log(INFO, "%u %u,  ", timestamp, voltage);
			//			if (!((i+1) % 5)) {
			//				_log(INFO, "\r\n");
			//			}
			average += voltage;
		}
		//		_log(INFO, "\r\n");
		average /= numSamples;

		// Measured voltage goes from 0-3.6V, measured as 0-1023(10 bit),
		// but Input voltage is from 0-3V (for Nordic EK) and 0-3.3 for Crownstone
		// so we need to rescale the value to have max at 3V (or 3.3V resp)
		// instead of 3.6V
#if(BOARD==CROWNSTONE)
		average *= 3.6/3.3;
#elif(BOARD==PCA10001)
		average *= 3.6/3.0;
#endif

		result = average;
	}

	// Unlock the buffer
	mb.unlock();

	return result;
}

/* Check light sensor by sampling ADC values
 */
bool Sensors::checkLightSensor(uint32_t time, uint16_t &value) {
	if (time - _lastLightCheck > LIGHT_CHECK_INTERVAL) {
		value = sampleSensor();

		if (value == -1) {
			LOGe("failed to read ADC!!");
			return false;
		}

		// convert to percentage
		LOGd("light intensity: %3d%%", (uint16_t)(100.0 * value/MAX_VALUE));
		_lastLightCheck = time;

		return true;
	}

	return false;
}

/* Check thermistor sensor by sampling ADC values
 */
uint16_t Sensors::checkThermalSensor(uint32_t time) {
	if (time - _lastThermalCheck > THERMAL_CHECK_INTERVAL) {
		uint16_t thermalIntensity = sampleSensor();

		if (thermalIntensity == -1) {
			LOGe("failed to read ADC!!");
			return -1;
		}

		// convert to percentage
		LOGd("thermal intensity: %3d%%", (uint16_t)(100.0 * thermalIntensity/MAX_VALUE));
		_lastThermalCheck = time;

		return thermalIntensity;
	}

	return -1;
}

/* Initialize enabled sensor(s)
 *
 */
void Sensors::init() {

#if ADC_USED
	initADC();
#endif

#ifdef PUSH_BUTTON_ENABLED
	initPushButton();
#endif

#ifdef SWITCH_ENABLED
	initSwitch();
#endif

	_initialized = true;
}

/* Tick function
 * check enabled sensor and update PWM signal
 * also initializes sensors on first run
 */
void Sensors::tick() {

	if (!_initialized) {
		init();
	}

	uint32_t now = RTC::now();

#ifdef PUSH_BUTTON_POLL
	bool pushed = false;
	if (checkPushButton(now, pushed)) {
		if (pushed != _lastPushed) {
			if (pushed) {
				LOGi("Button pushed, switch pwm signal");
				Sensors::switchPwmSignal();
			}
			_lastPushed = pushed;
		}
	}
#endif

#ifdef SWITCH_POLL
	bool switchOn = false;
	if (checkSwitch(now, switchOn)) {
		if (switchOn != _lastSwitchOn) {
			if (switchOn) {
				LOGi("switch ON");
				switchPwmOn();
			} else {
				LOGi("switch OFF");
				switchPwmOff();
			}
			_lastSwitchOn = switchOn;
		}
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

#ifdef SWITCH_ENABLED
/* initialize switch sensor based on compiler flags
 */
void Sensors::initSwitch() {

#ifdef SWITCH_INTERRUPT
	initGPIOTE();

	nrf_gpiote_event_config(0, PIN_GPIO_BUTTON, NRF_GPIOTE_POLARITY_TOGGLE);
	NRF_GPIOTE->INTENSET |= GPIOTE_INTENSET_IN0_Enabled << GPIOTE_INTENSET_IN0_Pos;
#endif

#ifdef SWITCH_POLL
	nrf_gpio_cfg_input(PIN_GPIO_BUTTON, NRF_GPIO_PIN_PULLUP);
#endif

	// check initial state of switch
	if (nrf_gpio_pin_read(PIN_GPIO_BUTTON)) {
		switchPwmOn();
	} else {
		switchPwmOff();
	}

}
#endif

#ifdef PUSH_BUTTON_ENABLED
/* Initialize push button, uses GPIO interrupt event handler
 */
void Sensors::initPushButton() {

#ifdef PUSH_BUTTON_POLL
	nrf_gpio_cfg_input(PIN_GPIO_BUTTON, NRF_GPIO_PIN_PULLUP);
#endif

#ifdef PUSH_BUTTON_INTERRUPT
	initGPIOTE();

	nrf_gpio_cfg_input(PIN_GPIO_BUTTON, NRF_GPIO_PIN_PULLUP);
	nrf_gpiote_event_config(0, PIN_GPIO_BUTTON, NRF_GPIOTE_POLARITY_TOGGLE);
	NRF_GPIOTE->INTENSET |= GPIOTE_INTENSET_IN0_Enabled << GPIOTE_INTENSET_IN0_Pos;

//	nrf_gpio_cfg_input(PIN_GPIO_BUTTON, NRF_GPIO_PIN_PULLUP);
//	nrf_gpio_cfg_sense_input(PIN_GPIO_BUTTON, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
//	NRF_GPIOTE->INTENSET |= GPIOTE_INTENSET_PORT_Enabled << GPIOTE_INTENSET_PORT_Pos;
#endif
}
#endif

#if INTERRUPT_USED
/* Initialize GPIO interrupt event handler
 */
void Sensors::initGPIOTE() {
	uint32_t err_code = 0;

	/* GPIOTE interrupt handler normally runs in STACK_LOW priority, need to put it
	in APP_LOW in order to use */
#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_SetPriority(GPIOTE_IRQn, NRF_APP_PRIORITY_LOW);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_SetPriority(GPIOTE_IRQn, NRF_APP_PRIORITY_LOW);
#endif

#if(NRF51_USE_SOFTDEVICE == 1)
	err_code = sd_nvic_EnableIRQ(GPIOTE_IRQn);
	APP_ERROR_CHECK(err_code);
#else
	NVIC_EnableIRQ(GPIOTE_IRQn);
#endif

}

#ifdef SWITCH_INTERRUPT
// keep track of the switch time. this variable is used to avoid flickering
// signal for switch
#endif
static uint32_t _interruptTimeout = 0;

#ifdef PUSH_BUTTON_INTERRUPT
static bool _lastPushed = false;
#endif

/* GPIO interrupt event handler
 */
extern "C" void GPIOTE_IRQHandler()
{
//	LOGd("GPIOTE_IRQHandler");

	// keep track of the GPIO pin states
	uint32_t pins_state = NRF_GPIO->IN;

#ifdef PUSH_BUTTON_INTERRUPT
	if ((NRF_GPIOTE->EVENTS_IN[0] == 1) &&
		(NRF_GPIOTE->INTENSET & GPIOTE_INTENSET_IN0_Msk)) {

		// signal seems to flicker some times when releasing the button
		// to avoid a wrong push registration, ignore such an event
		// if both pushed and _lastPushed are true, only register push
		// button event if pushed and _lastPushed are not equal

		bool pushed = (pins_state & (1 << PIN_GPIO_BUTTON)) == 0;

		uint32_t now = RTC::now();
		if (now > _interruptTimeout) {

//			if (pushed != _lastPushed) {
				if (pushed && !_lastPushed) {
					LOGi("Button pushed, switch pwm signal");
					Sensors::switchPwmSignal();
	//			} else {
	//				LOGi("released");
				}
				_lastPushed = pushed;

		//		NRF_GPIOTE->EVENTS_PORT = 0;
				// clear the event again
				NRF_GPIOTE->EVENTS_IN[0] = 0;
//			}
			_interruptTimeout = now + 100;
		} else {
			NRF_GPIOTE->EVENTS_IN[0] = 0;
		}
	}
#endif

#ifdef SWITCH_INTERRUPT

	// check if the event is for us
	if ((NRF_GPIOTE->EVENTS_IN[0] == 1) &&
		(NRF_GPIOTE->INTENSET & GPIOTE_INTENSET_IN0_Msk)) {

		bool switchON = (pins_state & (1 << PIN_GPIO_BUTTON)) == 0;

		// if switch is moved really slowly, signal flickers on transition
		// to avoid registering the signal all the time, add a delay
		// and only register the signal again if no other signal has been
		// received in the meantime.
		uint32_t now = RTC::now();
		if (now > _interruptTimeout) {
			if (switchON) {
				LOGi("switch ON");
				Sensors::switchPwmOn();
			} else {
				LOGi("switch OFF");
				Sensors::switchPwmOff();
			}
		}
		_interruptTimeout = now + 100;

		// clear the event again
		NRF_GPIOTE->EVENTS_IN[0] = 0;
	}

#endif

//	for (int i = 31; i >= 0; --i) {
//		_log(DEBUG, "%d", (pins_state & (1 << i)) != 0 ? 1 : 0);
//		_log(DEBUG, "%s", i%4 == 0 ? " " : "");
//	}
//	_log(DEBUG, "\r\n");

}

#endif

/* Switch PWM signal.
 * Checks current pwm signal, then toggles from ON to OFF and vice versa
 */
void Sensors::switchPwmSignal() {

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

/* Switch PWM signal ON
 * Checks current pwm signal and only changes if it is currently OFF
 */
void Sensors::switchPwmOn() {
	uint8_t pwmChannel;
	uint32_t pwmValue;
	// check first the current pwm value ...
	PWM::getInstance().getValue(pwmChannel, pwmValue);

	// if currently off, turn on, otherwise do nothing
	if (pwmValue == 0) {
		PWM::getInstance().setValue(0, 255);
	}
}

/* Switch PWM signal OFF
 * Checks current pwm signal and only changes if it is currently ON
 */
void Sensors::switchPwmOff() {
	uint8_t pwmChannel;
	uint32_t pwmValue;
	// check first the current pwm value ...
	PWM::getInstance().getValue(pwmChannel, pwmValue);

	// if currently on, turn off, otherwise do nothing
	if (pwmValue == 255) {
		PWM::getInstance().setValue(0, 0);
	}
}
