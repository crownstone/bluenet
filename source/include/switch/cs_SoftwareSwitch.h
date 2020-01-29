/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 28, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <common/cs_Types.h>
#include <events/cs_EventListener.h>
#include <switch/cs_SafeSwitch.h>

class SoftwareSwitch : public EventListener {
public:
	SoftwareSwitch(SafeSwitch* safeSwitch);
	cs_ret_code_t set(uint8_t intensity);


	switch_state_t getCurrentState();

	switch_state_t getIntendedState();


	cs_ret_code_t setAllowSwitching(bool allowed);

	cs_ret_code_t setAllowDimming(bool allowed);

	/**
	 * Handle events.
	 */
	void handleEvent(event_t& evt) override;

private:
	SafeSwitch* safeSwitch;

//	switch_state_t currentState;
	uint8_t intendedState;

	TYPIFY(CONFIG_PWM_ALLOWED) allowDimming = false;
	bool allowSwitching = true;

	cs_ret_code_t resolveIntendedState();

	cs_ret_code_t setRelay(bool on, switch_state_t currentState);
	cs_ret_code_t setRelayUnchecked(bool on);
	cs_ret_code_t setDimmer(uint8_t intensity, switch_state_t currentState);
	cs_ret_code_t setDimmerUnchecked(uint8_t intensity);
};


