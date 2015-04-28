/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Mar 23, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstdint>

//#include <common/cs_Types.h>

#define SENSORS_UPDATE_FREQUENCY 2 // hz

#define MAX_VALUE 1023

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


class Sensors {
public:
	Sensors();

	void tick();
	static void staticTick(Sensors* ptr) {
		ptr->tick();
	}

	void startTicking();
	void stopTicking();

	// helper functions, only operate on PWM instance
	// static so that they can be used by the
	// GPIO interrupt handler
	static void switchPwmSignal();
	static void switchPwmOn();
	static void switchPwmOff();

private:
	uint32_t _appTimerId;
	bool _initialized;
	uint32_t _lastLightCheck, _lastThermalCheck;
	uint32_t _lastPushButtonCheck, _lastSwitchCheck;
	bool _lastSwitchOn, _lastPushed;

	void init();

	void initADC();
	void initGPIOTE();
	void initPushButton();
	void initSwitch();

	uint16_t sampleSensor();

	bool checkLightSensor(uint32_t time, uint16_t &value);
	uint16_t checkThermalSensor(uint32_t time);
	bool checkPushButton(uint32_t time, bool &pushed);
	bool checkSwitch(uint32_t time, bool &switchOn);

};

