/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */
#pragma once

#include <structs/cs_PowerCurve.h>

class PowerSampling {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static PowerSampling& getInstance() {
		static PowerSampling instance;
		return instance;
	}

	void sampleCurrent(uint8_t type);

private:
	PowerSampling();

	void sampleCurrentInit();
	void sampleCurrentDone(uint8_t type);

	PowerCurve<uint16_t>* _powerCurve;

	bool _adcInitialized;
	bool _voltagePin;

};

