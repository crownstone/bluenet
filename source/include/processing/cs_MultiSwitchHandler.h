/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 19, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "common/cs_Types.h"
#include "events/cs_EventListener.h"

class MultiSwitchHandler : public EventListener {
private:
	MultiSwitchHandler();

public:
	//! Gets a static singleton (no dynamic memory allocation)
	static MultiSwitchHandler& getInstance() {
		static MultiSwitchHandler instance;
		return instance;
	}
	MultiSwitchHandler(MultiSwitchHandler const&) = delete;
	void operator=(MultiSwitchHandler const&) = delete;

	void init();
	void handleMultiSwitch(internal_multi_switch_item_t* cmd, cmd_source_with_counter_t& source);
	// Handle events as EventListener
	void handleEvent(event_t& event);

private:
	stone_id_t _ownId = 0;
};
