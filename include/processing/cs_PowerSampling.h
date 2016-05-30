/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */
#pragma once

#include <structs/cs_PowerCurve.h>

#define POWER_SAMPLING_AVERAGE_MASK 1 << 0
#define POWER_SAMPLING_CURVE_MASK  1 << 1
#define POWER_SAMPLING_ONE_SHOT_MASK 1 << 2

class PowerSampling {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static PowerSampling& getInstance() {
		static PowerSampling instance;
		return instance;
	}

	static void staticTick(PowerSampling *ptr) {
		ptr->sampleCurrent();
	}

	void startSampling(uint8_t type);
	void stopSampling();

private:
	PowerSampling();

	void sampleCurrent();

	/** Fill up the current curve and send it out over bluetooth
	 * @type specifies over which characteristic the current curve should be sent.
	 */
	void sampleCurrentDone();

	void sampleCurrentInit();
	void sampleCurrentDone(uint8_t type);

	app_timer_id_t _samplingTimer;

	PowerCurve<uint16_t>* _powerCurve;

	bool _adcInitialized;
	bool _voltagePin;

	uint32_t _startTimestamp;

	bool _sampling;
	uint8_t _samplingType;

	uint16_t _samplingTime;
	uint16_t _samplingInterval;

};

