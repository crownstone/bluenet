/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 19, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


#include <test/cs_TestAccess.h>
#include <switch/cs_SwitchAggregator.h>

template<>
class TestAccess<SwitchAggregator> {
public:
	// the object that this test class instance will operate on.
	SwitchAggregator& _switchAggregator;

	TestAccess(SwitchAggregator& switchAggregator) : _switchAggregator(switchAggregator) {}

	// assumes the aggregator is already attached to the event bus.
	void setOverrideState(std::optional<uint8_t> override) {
		if(override.has_value()){
			internal_multi_switch_item_cmd_t switchCmdPacket = {
					.switchCmd = override.value()
			};
			event_t switchEvent(CS_TYPE::CMD_SWITCH, &switchCmdPacket, sizeof(switchCmdPacket));

			_switchAggregator.handleEvent(switchEvent);
		} else {
			_switchAggregator._overrideState.reset();
		}

	}

	std::optional<uint8_t> getOverrideState() {
		return _switchAggregator._overrideState;
	}
};
