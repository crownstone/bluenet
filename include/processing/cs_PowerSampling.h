/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */
#pragma once

#include <structs/cs_PowerSamples.h>
#include <structs/buffer/cs_CircularBuffer.h>
#include <protocol/cs_MeshMessageTypes.h>

class PowerSampling {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static PowerSampling& getInstance() {
		static PowerSampling instance;
		return instance;
	}

	void init();

	void stopSampling();

	static void staticPowerSampleFinish(PowerSampling *ptr) {
		ptr->powerSampleFinish();
	}

	/** Initializes and starts the ADC, also starts interval timer.
	 */
	void powerSampleFirstStart();

	/** Starts a new power sample burst.
	 *  Called at a low interval.
	 */
	void startSampling();
	static void staticPowerSampleStart(PowerSampling *ptr) {
		ptr->startSampling();
	}

	/** Called when the sample burst is finished.
	 *  Calculates the power usage, updates the state.
	 *  Sends the samples if the central is subscribed for that.
	 */
	void powerSampleFinish();

	/** Called at a short interval.
	 *  Reads out the buffer.
	 *  Sends the samples via notifications and/or mesh.
	 */
	void powerSampleReadBuffer();
	static void staticPowerSampleRead(PowerSampling *ptr) {
		ptr->powerSampleReadBuffer();
	}

	/** Fill up the current curve and send it out over bluetooth
	 * @type specifies over which characteristic the current curve should be sent.
	 */
	void sampleCurrentDone(uint8_t type);

	void getBuffer(buffer_ptr_t& buffer, uint16_t& size);

//	void finished() {
//		Timer::getInstance().start(_powerSamplingFinishTimer, 5, this);
//	}

private:
	PowerSampling();

//	app_timer_id_t _powerSamplingFinishTimer;

	app_timer_id_t _staticPowerSamplingStartTimer;
	app_timer_id_t _staticPowerSamplingReadTimer;

	buffer_ptr_t _powerSamplesBuffer; //! Buffer that holds the data for burst or continous sampling

//	DifferentialBuffer<uint32_t> _currentSampleTimestamps;
//	DifferentialBuffer<uint32_t> _voltageSampleTimestamps;
	CircularBuffer<uint16_t> _currentSampleCircularBuf;
	CircularBuffer<uint16_t> _voltageSampleCircularBuf;
	power_samples_mesh_message_t* _powerSamplesMeshMsg;
	uint16_t _powerSamplesCount;
//	uint16_t _lastPowerSample;
	uint16_t _burstCount;

	PowerSamples _powerSamples;

	uint16_t _burstSamplingInterval;
	uint16_t _contSamplingInterval;
	float _voltageMultiplier;
	float _currentMultiplier;
	float _voltageZero;
	float _currentZero;
	float _powerZero;
	uint16_t _zeroAvgWindow;
};

