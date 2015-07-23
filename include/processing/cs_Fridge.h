/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 15, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <cstdint>

#define FRIDGE_UPDATE_FREQUENCY 0.2 // hz

class Fridge {
public:
	Fridge();
	void tick();
	static void staticTick(Fridge* ptr) {
		ptr->tick();
	}
	void startTicking();
	void stopTicking();
private:
	uint32_t _appTimerId;
	int8_t _minTemp;
	int8_t _maxTemp;
};


