/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 13, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <ble/cs_Nordic.h>
#include <drivers/cs_Timer.h>
#include <processing/cs_Switch.h>
#include <events/cs_EventListener.h>
#include <protocol/cs_CommandTypes.h>

class Watchdog : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static Watchdog& getInstance() {
		static Watchdog instance;
		return instance;
	}

	void init();

	void keepAliveTimeout();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

private:
	Watchdog();

	void keepAlive();

	bool                               _hasKeepAliveState;
	keep_alive_state_message_payload_t _lastKeepAlive;

	app_timer_t                        _keepAliveTimerData;
	app_timer_id_t                     _keepAliveTimerId;

};

