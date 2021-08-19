/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <behaviour/cs_BehaviourHandler.h>
#include <behaviour/cs_TwilightHandler.h>
#include <events/cs_EventListener.h>
#include <optional>
#include <structs/buffer/cs_CircularBuffer.h>
#include <switch/cs_SmartSwitch.h>

/**
 * Handler that aggregates events related to switching such as SwitchCraft,
 * Behaviours, Twilight and App/User side actions. Based on the incoming data
 * this object decides what state to set the SmartSwitch to.
 */
class SwitchAggregator : public EventListener {
public:
	SwitchAggregator();

	void init(const boards_config_t& board);

	/**
	 * When swSwitch is locked, only CMD_SWITCH_LOCKED events will be handled.
	 * Else events may alter the intended states and subsequently trigger an
	 * actual state change.
	 */
	virtual void handleEvent(event_t& evt) override;

	/**
	 * To be called when there is enough power to use the switch.
	 */
	void switchPowered();

private:
	TwilightHandler twilightHandler;
	BehaviourHandler behaviourHandler;

	SmartSwitch smartSwitch;

	// the latest states requested by other parts of the system.
	std::optional<uint8_t> overrideState = {};
	std::optional<uint8_t> behaviourState = {};
	std::optional<uint8_t> twilightState = {};

	// the last state that was aggregated and passed on towards the SoftwareSwitch.
	std::optional<uint8_t> aggregatedState = {};

	// Cache of previous time update.
	uint32_t _lastTimestamp = 0;

	/**
	 * Which source claimed the switch.
	 *
	 * Until timeout, nothing with a different source can set the switch.
	 * Unless that source overrules the current source.
	 */
	cmd_source_with_counter_t _source = cmd_source_with_counter_t(CS_CMD_SOURCE_NONE);
	uint32_t _ownerTimeoutCountdown = 0;

	// Max number of switch commands to store in history.
	const static uint8_t _maxSwitchHistoryItems = 10;

	/**
	 * Keep up a history of switch commands.
	 * This can be commands from any source, user or automated.
	 */
	CircularBuffer<cs_switch_history_item_t> _switchHistory;

	// ================================== State updaters ==================================

	/**
	 * Checks the behaviourState and overrideState,
	 * to set the swSwitch to the desired value:
	 * - if swSwitch doesn't allow switching, nothing happens, else,
	 * - when overrideState has a value, swSwitch is set to that value, else,
	 * - when behaviourState has a value, swSwitch is set to that value, else,
	 * - nothing happens.
	 *
	 * This method will clear the overrideState when it matches
	 * the behaviourState, unless the switch is locked or allowOverrideReset is false.
	 *
	 * (Disallowing override state to reset is used for commands that want to change
	 * the value and trigger a reset which are not initated through behaviour handlers)
	 */
	cs_ret_code_t updateState(bool allowOverrideReset, const cmd_source_with_counter_t& source);

	/**
	 * Calls update on the behaviour handlers and returns true
	 * if after the update is allowed to reset the overrideState.
	 *
	 * Returns true when behaviour value changed since last time it was called.
	 */
	bool updateBehaviourHandlers();

	// ================================== Event handling ==================================

	/**
	 * Triggers an updateState() call on all handled events and adjusts
	 * at least one of behaviourState or overrideState.
	 */
	bool handleStateIntentionEvents(event_t & evt);

	/**
	 * Tries to update [overrideState] to [value] and then calls updateState(false).
	 * If smartswitch disallows this operation at the end of updateState, revert back
	 * [overrideState] to the value it had before this function call.
	 */
	void executeStateIntentionUpdate(uint8_t value, cmd_source_with_counter_t& source);

	/**
	 * EVT_TICK, STATE_TIME and EVT_TIME_SET events possibly trigger
	 * a new aggregated state. This handling function takes care of that.
	 *
	 * returns true when the event should be considered 'consumed'.
	 * (which is when evt is of one of these types.)
	 */
	bool handleTimingEvents(event_t & evt);

	/**
	 * EVT_PRESENCE_MUTATION
	 *
	 * returns true when the event should be considered 'consumed'.
	 * (which is when evt is of one of these types.)
	 */
	bool handlePresenceEvents(event_t& evt);

	/**
	 * Clearing private variables, setting them to specific values, etc.
	 * Debug or power user features.
	 */
	bool handleSwitchAggregatorCommand(event_t& evt);

	void handleSwitchStateChange(uint8_t newIntensity);

	// ================================== Misc ==================================

	/**
	 * Aggregates the value of twilightHandler and behaviourHandler
	 * into a single intensity value.
	 *
	 * This will return the minimum of the respective handler values
	 * when both are defined, otherwise return the value of the one
	 * that is defined, otherwise 100.
	 */
	uint8_t aggregatedBehaviourIntensity();

	/**
	 * When override state is the special value 'translucent on'
	 * it should be interpreted according to the values of twilightHandler
	 * and behaviourHandler. This getter centralizes that.
	 *
	 * When override is 0xff:
	 * This method will return the minimum of respective handlers when both of them
	 * have a non-zero value, otherwise return the non-zero value of the
	 * handler that has a non-zero value (if there is such), otherwise 100.
	 */
	std::optional<uint8_t> resolveOverrideState();

	/**
	 * Tries to set source as owner of the switch.
	 * Returns true on success, false if switch is already owned by a different source, and given source does not overrule it.
	 */
	bool checkAndSetOwner(const cmd_source_with_counter_t& source);

	void handleGetBehaviourDebug(event_t& evt);

	void addToSwitchHistory(const cs_switch_history_item_t& cmd);
	void printSwitchHistory();

	void printStatus();
	void pushTestDataToHost();
};
