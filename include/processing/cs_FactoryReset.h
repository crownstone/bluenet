/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Aug 4, 2016
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <cstdint>
#include "drivers/cs_Timer.h"

class FactoryReset {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static FactoryReset& getInstance() {
		static FactoryReset instance;
		return instance;
	}

	void init();

	/* Function to recover: no need to be admin to call this */
	bool recover(uint32_t resetCode);

	/* Function to go into factory reset mode */
	bool factoryReset(uint32_t resetCode);

	/* Function to actually wipe the memory */
	bool finishFactoryReset();

	/* Enable/disable ability to recover */
	void enableRecovery(bool enable);

	/* Static function for the timeout */
	static void staticTimeout(FactoryReset *ptr) {
		ptr->timeout();
	}

private:
	FactoryReset();
	bool factoryReset();
	void resetTimeout();
	void timeout();

	bool _recoveryEnabled;
	uint32_t _rtcStartTime;
	app_timer_id_t _recoveryDisableTimer;
};
