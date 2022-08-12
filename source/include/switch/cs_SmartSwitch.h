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

/**
 * Class that:
 * - From intended intensity 0-100, decides whether to use the dimmer or relay.
 * - Checks if dimming is allowed, and if switching is allowed.
 * - Stores state in State class.
 * - [future] Slowly (seconds) fades to intended state.
 */
class SmartSwitch : public EventListener {
public:
	void init(const boards_config_t& board);

	/**
	 * Start switch.
	 *
	 * To be called once there is enough power to switch relay, and enable dimmer.
	 */
	void start();

	/**
	 * Set intended intensity.
	 *
	 * Intended state is only changed by this function.
	 *
	 * @param[in]  Intensity value: 0-100
	 * @return     Return code. When it's not success, the current intensity might have changed to something else.
	 *             ERR_NOT_POWERED when trying to dim while the dimmer is not powered.
	 */
	cs_ret_code_t set(uint8_t intensity);

	/**
	 * Get current switch intensity.
	 *
	 * @return     The current switch intensity: 0-100.
	 */
	uint8_t getCurrentIntensity();

	/**
	 * Get the intended switch state.
	 *
	 * @return     Intended switch intensity: 0-100.
	 */
	uint8_t getIntendedState();

	/**
	 * Get actual switch state.
	 */
	switch_state_t getActualState();

	/**
	 * Callback function definition.
	 */
	typedef function<void(uint8_t newIntensity)> callback_on_intensity_change_t;

	/**
	 * Register a callback function that's called when state changes unexpectedly.
	 */
	void onUnexpextedIntensityChange(const callback_on_intensity_change_t& closure);

	/**
	 * Handle events.
	 */
	void handleEvent(event_t& evt) override;

private:
	SafeSwitch _safeSwitch;

	/**
	 * Intended switch state by the user, as percentage.
	 */
	uint8_t _intendedState;

	/**
	 * Callback to be called when switch state changes unexpectedly, so anything but set().
	 */
	callback_on_intensity_change_t _callbackOnIntensityChange;

	/**
	 * Cached value of what's stored in State.
	 */
	TYPIFY(STATE_SWITCH_STATE) _storedState;

	/**
	 * Cached value of what's stored in State.
	 */
	TYPIFY(CONFIG_DIMMING_ALLOWED) _allowDimming = false;

	/**
	 * Cached value of what's stored in State.
	 */
	TYPIFY(CONFIG_SWITCH_LOCKED) _switchLocked   = false;

	/**
	 * Override of switch lock, necessary to restore state at startup.
	 */
	bool _allowSwitchingOverride                 = false;

	/**
	 * Whether switch state is allowed to be changed.
	 */
	bool allowSwitching();

	/**
	 * Get intensity from a switch state.
	 */
	uint8_t getIntensityFromSwitchState(switch_state_t switchState);

	/**
	 * From set intended state, try to set the relay and/or dimmer to correct values.
	 * Does not set intended state.
	 */
	cs_ret_code_t resolveIntendedState();

	/**
	 * Set relay.
	 * Checks if switching is allowed.
	 */
	cs_ret_code_t setRelay(bool on);

	/**
	 * Set relay without checks.
	 * Also updates State.
	 */
	cs_ret_code_t setRelayUnchecked(bool on);

	/**
	 * Set dimmer.
	 * Checks if dimming and switching is allowed.
	 */
	cs_ret_code_t setDimmer(uint8_t intensity, bool fade = true);

	/**
	 * Set dimmer without checks.
	 * Also updates State.
	 */
	cs_ret_code_t setDimmerUnchecked(uint8_t intensity, bool fade);

	/**
	 * Handle unexpected state change by safeSwitch.
	 */
	void handleUnexpectedStateChange(switch_state_t newState);

	/**
	 * Send intensity update to listeners.
	 * Only to be called when state changes due to events, not due to calls to set().
	 * This prevents the user to get updates before the call is even finished, this is what return codes are for.
	 */
	void sendUnexpectedIntensityUpdate(uint8_t newIntensity);

	/**
	 * Store switch state to State class.
	 * Also updates storedState.
	 */
	void store(switch_state_t newState);

	/**
	 * Set switch lock.
	 * Also updates State.
	 */
	cs_ret_code_t setSwitchLock(bool lock);

	/**
	 * Set allow dimming.
	 * Also updates State.
	 */
	cs_ret_code_t setAllowDimming(bool allowed);

	void handleAllowDimmingSet();

	cs_ret_code_t handleCommandSetRelay(bool on);

	cs_ret_code_t handleCommandSetDimmer(uint8_t intensity);
};
