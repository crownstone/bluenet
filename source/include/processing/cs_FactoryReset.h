/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 4, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>

#include "drivers/cs_Timer.h"
#include "events/cs_EventListener.h"

// All the classes that are expected to factory reset.
enum FactoryResetClassBit {
	FACTORY_RESET_BIT_STATE = 0,
#if BUILD_MICROAPP_SUPPORT == 1
	FACTORY_RESET_BIT_MICROAPP,
#endif
#if BUILD_MESHING == 1 && MESH_PERSISTENT_STORAGE == 1
	FACTORY_RESET_BIT_MESH,
#endif
	FACTORY_RESET_NUM_BITS
};
#define FACTORY_RESET_MASK_ALL ((1 << FACTORY_RESET_NUM_BITS) - 1)

class FactoryReset : public EventListener {
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
	bool finishFactoryReset(uint8_t deviceType);

	/* Enable/disable ability to recover */
	void enableRecovery(bool enable);

	/* Static function for the timeout */
	static void staticTimeout(FactoryReset* ptr) { ptr->timeout(); }

	/* Static function for the timeout */
	static void staticProcess(FactoryReset* ptr) { ptr->process(); }

	/**
	 * Handle events.
	 */
	void handleEvent(event_t& event);

private:
	FactoryReset();

	inline bool validateResetCode(uint32_t resetCode);
	bool performFactoryReset();
	void resetTimeout();
	void timeout();
	void process();
	void onClassFactoryResetDone(const FactoryResetClassBit bit);

	bool _recoveryEnabled;
	uint32_t _rtcStartTime;

	// Used to check if all classes are factory reset.
	uint8_t _successfullyFactoryResetBitmask = 0;

	app_timer_t _recoveryDisableTimerData;
	app_timer_id_t _recoveryDisableTimerId;

	app_timer_t _recoveryProcessTimerData;
	app_timer_id_t _recoveryProcessTimerId;
};
