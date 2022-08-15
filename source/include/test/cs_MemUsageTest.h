/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 20, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>

class MemUsageTest {
public:
	MemUsageTest(const boards_config_t& boardsConfig);

	void start();

	/**
	 * To be called each tick.
	 */
	void onTick();

private:
	boards_config_t _boardConfig;
	bool _started           = false;

	int _behaviourIndex     = 0;

	int _rssiDataStoneId    = 1;

	int _presenceProfileId  = 0;
	int _presenceLocationId = 0;

	int _trackedDeviceId    = 0;

	int _stateType          = 0;

	// Returns true when done with all.
	bool setNextBehaviour();

	// Returns true when done with all.
	bool sendNextRssiData();

	// Returns true when done with all.
	bool sendNextPresence();

	// Returns true when done with all.
	bool setNextTrackedDevice();

	// Returns true when done with all.
	bool setNextStateType();

	// Returns true on success.
	bool setNextStateType(int stateType);

	void printRamStats();
};
