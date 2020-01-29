/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 28, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <common/cs_Types.h>
#include <drivers/cs_Dimmer.h>
#include <drivers/cs_Relay.h>
#include <events/cs_EventListener.h>

class SafeSwitch : public EventListener {
public:
	/**
	 * Constructor.
	 */
	SafeSwitch(const boards_config_t& board, Dimmer& dimmer, Relay& relay);

	/**
	 * Set relay.
	 *
	 * It could be that something else is done instead, check the error code.
	 *
	 * @return     Error code: if not successful, check what the current state is.
	 */
	cs_ret_code_t setRelay(bool on);

	/**
	 * Set dimmer intensity.
	 *
	 * It could be that something else is done instead, check the error code.
	 *
	 * @return     Error code: if not successful, check what the current state is.
	 */
	cs_ret_code_t setDimmer(uint8_t intensity);

	/**
	 * Get current switch state.
	 *
	 * @return     Current switch state.
	 */
	switch_state_t getState();

	/**
	 * Handle events.
	 */
	void handleEvent(event_t& evt) override;

private:
	Dimmer* dimmer;
	Relay* relay;

	uint32_t hardwareBoard;

	switch_state_t currentState;
	switch_state_t storedState;

	bool relayPowered = false;
	bool dimmerPowered = false;

	uint32_t dimmerPowerUpCountDown = PWM_BOOT_DELAY_MS / TICK_INTERVAL_MS;

	bool checkedDimmerPowerUsage = false;
	uint32_t dimmerCheckCountDown = 0;

	void start();

	cs_ret_code_t setRelayUnchecked(bool on);

	/**
	 * Set dimmer intensity.
	 *
	 * Checks: similar
	 * Does not check: safe, powered
	 */
	cs_ret_code_t setDimmerUnchecked(uint8_t intensity);

	/**
	 * Set dimmer intensity, and start power usage check.
	 *
	 * Does not check: safe, powered
	 */
	cs_ret_code_t startDimmerPowerCheck(uint8_t intensity);

	void cancelDimmerPowerCheck();

	void checkDimmerPower();

	void dimmerPoweredUp();

	void sendDimmerPoweredEvent();


	void forceSwitchOff();

	void forceRelayOnAndDimmerOff();


	state_errors_t getErrorState();

	bool isSwitchOverLoaded(state_errors_t stateErrors);

	bool hasDimmerError(state_errors_t stateErrors);

	bool isSafeToDim(state_errors_t stateErrors);

	bool isSafeToTurnRelayOn(state_errors_t stateErrors);

	bool isSafeToTurnRelayOff(state_errors_t stateErrors);

	/**
	 * Store switch state to State class.
	 *
	 * Also updates storedState.
	 */
	void store(switch_state_t newState);

	void goingToDfu();
};


