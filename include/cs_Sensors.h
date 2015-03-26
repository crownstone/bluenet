/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Mar 23, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstdint>

#define MAX_VALUE 1023

class Sensors {
public:
	Sensors();

	void tick();

private:
	bool _initialized;
	uint32_t _lastLightCheck, _lastThermalCheck;
	uint32_t _lastPushButtonCheck, _lastSwitchCheck;
	bool _lastPushed, _lastSwitchOn;

	void initADC();
	void gpiote_init(void);

	uint16_t sampleSensor();

	bool checkLightSensor(uint32_t time, uint16_t &value);
	uint16_t checkThermalSensor(uint32_t time);
	bool checkPushButton(uint32_t time, bool &pushed);
	bool checkSwitch(uint32_t time, bool &switchOn);
};

