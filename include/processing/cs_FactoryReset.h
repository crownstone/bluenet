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
	bool recover(uint32_t resetCode);
	bool factoryReset(uint32_t resetCode);
	void enableRecovery(bool enable);
	static void staticDisableRecovery(FactoryReset *ptr) {
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
