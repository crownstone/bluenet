/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <events/cs_EventDispatcher.h>
#include <events/cs_EventListener.h>
#include <logging/cs_Logger.h>
#include <presence/cs_PresenceHandler.h>
#include <switch/cs_SwitchAggregator.h>
#include <test/cs_Test.h>
#include <util/cs_Utils.h>

#include <optional>

#define LOGSwitchAggregatorDebug LOGvv
#define LOGSwitchAggregatorEvent LOGd
#define LOGSwitchHistory LOGvv

// ========================= Public ========================

SwitchAggregator::SwitchAggregator() : _switchHistory(_maxSwitchHistoryItems) {}

void SwitchAggregator::init(const boards_config_t& board) {
	LOGi("init");
	smartSwitch.onUnexpextedIntensityChange([&](uint8_t newState) -> void { handleSwitchStateChange(newState); });
	smartSwitch.init(board);

	// Allocate buffer.
	_switchHistory.init();

	listen();  // TODO: move to end of init.

	// TODO: only init twilightHandler and behaviourHandler in normal mode.
	twilightHandler.init();
	behaviourHandler.init();

	overrideState = smartSwitch.getIntendedState();
	pushTestDataToHost();

	initChildren();
}

void SwitchAggregator::switchPowered() {
	smartSwitch.start();
}

std::vector<Component*> SwitchAggregator::getChildren() {
	return {&behaviourHandler, &twilightHandler};
}

// ================================== State updaters ==================================

bool SwitchAggregator::updateBehaviourHandlers() {
	LOGSwitchAggregatorDebug("updateBehaviourHandlers");

	std::optional<uint8_t> prevBehaviourState = behaviourState;
	behaviourHandler.update();
	behaviourState = behaviourHandler.getValue();

	twilightHandler.update();
	twilightState = twilightHandler.getValue();

	if (!prevBehaviourState || !behaviourState) {
		// don't allow override resets when no values are changed.
		return false;
	}

	return (prevBehaviourState != behaviourState);
}

cs_ret_code_t SwitchAggregator::updateState(bool allowOverrideReset, const cmd_source_with_counter_t& source) {
	LOGSwitchAggregatorDebug("updateState allowOverrideReset=%u", allowOverrideReset);
	bool shouldResetOverrideState = false;

	if (overrideState && behaviourState && aggregatedState) {
		bool overrideStateIsOn   = (*overrideState != 0);
		bool aggregatedStateIsOn = (*aggregatedState != 0);
		bool behaviourStateIsOn  = (*behaviourState != 0);

		bool overrideMatchedAggregated   = (overrideStateIsOn == aggregatedStateIsOn);
		bool behaviourWantsToChangeState = (behaviourStateIsOn != aggregatedStateIsOn);

		if (overrideMatchedAggregated && behaviourWantsToChangeState && allowOverrideReset) {
			LOGd("Resetting override state overrideStateIsOn=%u aggregatedStateIsOn=%u behaviourStateIsOn=%u",
				 overrideStateIsOn,
				 aggregatedStateIsOn,
				 behaviourStateIsOn);
			shouldResetOverrideState = true;
		}
	}

	if (overrideState && !shouldResetOverrideState) {
		aggregatedState = resolveOverrideState();
	}
	else if (behaviourState) {
		// only use aggr. if no SwitchBehaviour conflict is found
		aggregatedState = aggregatedBehaviourIntensity();
	}
	// If override and behaviour don't have an opinion, keep previous value.

	LOGSwitchAggregatorDebug(
			"overrideState=%u, behaviourState=%u, twilightState=%u, aggregatedState=%u",
			overrideState ? overrideState.value() : CS_SWITCH_CMD_VAL_NONE,
			behaviourState ? behaviourState.value() : CS_SWITCH_CMD_VAL_NONE,
			twilightState ? twilightState.value() : CS_SWITCH_CMD_VAL_NONE,
			aggregatedState ? aggregatedState.value() : CS_SWITCH_CMD_VAL_NONE);

	// attempt to update smartSwitch value
	cs_ret_code_t retCode = ERR_SUCCESS_NO_CHANGE;
	if (aggregatedState) {
		retCode = smartSwitch.set(*aggregatedState);
		if (shouldResetOverrideState) {
			if (retCode == ERR_SUCCESS) {
				LOGSwitchAggregatorDebug("Reset override state");
				overrideState = {};
			}
			else {
				overrideState = smartSwitch.getCurrentIntensity();
				LOGSwitchAggregatorDebug("Failed to reset override state, overrideState=%u", *overrideState);
			}
		}
	}

	TYPIFY(EVT_BEHAVIOUR_OVERRIDDEN) eventData = overrideState.has_value();
	event_t overrideEvent(CS_TYPE::EVT_BEHAVIOUR_OVERRIDDEN, &eventData, sizeof(eventData));
	overrideEvent.dispatch();

	pushTestDataToHost();

	return retCode;
}

// ========================= Event handling =========================

void SwitchAggregator::handleEvent(event_t& event) {
	if (handleSwitchAggregatorCommand(event)) {
		return;
	}

	if (handleTimingEvents(event)) {
		return;
	}

	if (handlePresenceEvents(event)) {
		return;
	}

	handleStateIntentionEvents(event);

	if (event.type == CS_TYPE::CMD_GET_BEHAVIOUR_DEBUG) {
		handleGetBehaviourDebug(event);
	}
}

bool SwitchAggregator::handleSwitchAggregatorCommand(event_t& event) {
	switch (event.type) {
		case CS_TYPE::CMD_GET_SWITCH_HISTORY: {
			cs_switch_history_header_t header;
			header.count                  = _switchHistory.size();
			cs_buffer_size_t requiredSize = sizeof(header) + header.count * sizeof(cs_switch_history_item_t);
			if (event.result.buf.len < requiredSize) {
				event.result.returnCode = ERR_BUFFER_TOO_SMALL;
				break;
			}
			memcpy(event.result.buf.data, &header, sizeof(header));
			int offset = sizeof(header);
			for (uint16_t i = 0; i < _switchHistory.size(); ++i) {
				const auto& item = _switchHistory[i];
				memcpy(event.result.buf.data + offset, &item, sizeof(item));
				offset += sizeof(item);
			}
			event.result.dataSize   = requiredSize;
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		default: {
			return false;
		}
	}
	return true;
}

bool SwitchAggregator::handleTimingEvents(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_TICK: {
			// decrement until 0
			_ownerTimeoutCountdown == 0 || _ownerTimeoutCountdown--;

			// Execute code following this if statement, only once a second.
			// But since we poll every tick, we are pretty close to the moment the posix seconds increase.
			uint32_t timestamp = SystemTime::posix();
			if (timestamp == _lastTimestamp) {
				break;
			}
			_lastTimestamp = timestamp;

			// Update switch value.
			bool behaviourValueChanged = updateBehaviourHandlers();
			uint8_t behaviourRuleIndex = 255;  // TODO: set correct value.
			cmd_source_with_counter_t source(cmd_source_t(CS_CMD_SOURCE_TYPE_BEHAVIOUR, behaviourRuleIndex));
			updateState(behaviourValueChanged, source);
			if (behaviourValueChanged) {
				addToSwitchHistory(cs_switch_history_item_t(
						timestamp, *aggregatedState, smartSwitch.getActualState(), source.source));
			}
			break;
		}
		default: {
			return false;
		}
	}
	return true;
}

bool SwitchAggregator::handlePresenceEvents(event_t& event) {
	if (event.type == CS_TYPE::EVT_PRESENCE_MUTATION) {
		LOGSwitchAggregatorEvent("handlePresence");

		PresenceMutation mutationtype = *reinterpret_cast<PresenceMutation*>(event.data);

		switch (mutationtype) {
			case PresenceMutation::LastUserExitSphere: {
				LOGd("SwitchAggregator LastUserExit");
				if (overrideState) {
					Time now = SystemTime::now();
					LOGd("SwitchAggregator LastUserExit override state true");

					if (behaviourHandler.requiresPresence(now)) {
						// if there exists a behaviour which is active at given time and
						//      	and it has a non-negated presence clause (that may not be satisfied)
						//      		clear override
						LOGd("clearing override state because last user exited sphere");
						overrideState = {};
						updateBehaviourHandlers();
						uint8_t behaviourRuleIndex = 255;  // TODO: set correct value.
						updateState(true, cmd_source_t(CS_CMD_SOURCE_TYPE_BEHAVIOUR, behaviourRuleIndex));
					}
				}
				break;
			}
			default: break;
		}
		return true;
	}

	return false;
}

bool SwitchAggregator::handleStateIntentionEvents(event_t& event) {
	switch (event.type) {
		// ============== overrideState Events ==============
		case CS_TYPE::CMD_SWITCH_ON: {
			LOGSwitchAggregatorEvent("CMD_SWITCH_ON");

			executeStateIntentionUpdate(100, event.source);
			break;
		}
		case CS_TYPE::CMD_SWITCH_OFF: {
			LOGSwitchAggregatorEvent("CMD_SWITCH_OFF");

			executeStateIntentionUpdate(0, event.source);
			break;
		}
		case CS_TYPE::CMD_SWITCH: {
			//			LOGSwitchAggregatorEvent("CMD_SWITCH");

			TYPIFY(CMD_SWITCH)* packet = (TYPIFY(CMD_SWITCH)*)event.data;
			if (!checkAndSetOwner(event.source)) {
				LOGSwitchAggregatorDebug("not executing, checkAndSetOwner returned false");
				event.result.returnCode = ERR_NO_ACCESS;
				return false;
			}
			LOGd("packet intensity: %u, source: type=%u id=%u",
				 packet->switchCmd,
				 event.source.source.type,
				 event.source.source.id);
			executeStateIntentionUpdate(packet->switchCmd, event.source);

			break;
		}
		case CS_TYPE::CMD_SWITCH_TOGGLE: {
			LOGSwitchAggregatorEvent("CMD_SWITCH_TOGGLE");
			executeStateIntentionUpdate(CS_SWITCH_CMD_VAL_TOGGLE, event.source);
			break;
		}
		default: {
			// event not handled.
			return false;
		}
	}

	event.result.returnCode = ERR_SUCCESS;
	return true;
}

void SwitchAggregator::executeStateIntentionUpdate(uint8_t value, cmd_source_with_counter_t& source) {

#ifdef DEBUG
	switch (value) {
		case CS_SWITCH_CMD_VAL_DEBUG_RESET_ALL:
			overrideState.reset();
			behaviourState.reset();
			twilightState.reset();
			aggregatedState = 0;
			LOGd("Reset all");
			break;
		case CS_SWITCH_CMD_VAL_DEBUG_RESET_AGG:
			aggregatedState.reset();
			LOGd("Reset aggregatedState");
			break;
		case CS_SWITCH_CMD_VAL_DEBUG_RESET_OVERRIDE:
			overrideState.reset();
			LOGd("Reset overrideState");
			break;
		case CS_SWITCH_CMD_VAL_DEBUG_RESET_AGG_OVERRIDE:
			overrideState.reset();
			aggregatedState.reset();
			LOGd("Reset overrideState and aggregatedState");
			break;
	}
#endif

	switch (value) {
		case CS_SWITCH_CMD_VAL_TOGGLE: {
			uint8_t newValue = smartSwitch.getIntendedState() == 0 ? CS_SWITCH_CMD_VAL_SMART_ON : 0;
			executeStateIntentionUpdate(newValue, source);
			return;
		}
		case CS_SWITCH_CMD_VAL_BEHAVIOUR: {
			overrideState.reset();
			break;
		}
		case CS_SWITCH_CMD_VAL_SMART_ON: {
			overrideState = value;
			break;
		}
		default: {
			if (value <= CS_SWITCH_CMD_VAL_FULLY_ON) {
				overrideState = value;
			}
			break;
		}
	}

	// Don't allow override reset in updateState, it has just been requested to be set to `value`
	cs_ret_code_t retCode = updateState(false, source);

	switch (retCode) {
		case ERR_SUCCESS: {
			[[fallthrough]];
		}
		case ERR_SUCCESS_NO_CHANGE: {
			[[fallthrough]];
		}
		case ERR_NOT_POWERED: {
			break;
		}
		default: {
			// Only when the dimmer isn't powered yet, we want to keep the given overrideState,
			// because it should be set once the dimmer is powered.
			// In all other cases, we want to copy the current intensity, so that the intensity
			// doesn't change when some configuration is changed.
			LOGSwitchAggregatorDebug("Copy current intensity to overrideState");
			overrideState = smartSwitch.getCurrentIntensity();
		}
	}

	addToSwitchHistory(
			cs_switch_history_item_t(SystemTime::posix(), value, smartSwitch.getActualState(), source.source));
	pushTestDataToHost();
}

void SwitchAggregator::handleSwitchStateChange(uint8_t newIntensity) {
	LOGi("handleSwitchStateChange %u", newIntensity);
	// TODO: 21-01-2020 This is not a user intent, so store in a different variable, and then figure out what to do with
	// it.
	overrideState = newIntensity;
	pushTestDataToHost();
	// TODO: get the correct source.
	addToSwitchHistory(cs_switch_history_item_t(
			SystemTime::posix(), newIntensity, smartSwitch.getActualState(), cmd_source_t(CS_CMD_SOURCE_INTERNAL)));
}

// ========================= Misc =========================

uint8_t SwitchAggregator::aggregatedBehaviourIntensity() {
	LOGSwitchAggregatorDebug("aggregatedBehaviourIntensity");

	if (behaviourState && twilightState) {
		LOGSwitchAggregatorDebug("Returning min of behaviour(%u) and twilight(%u)", *behaviourState, *twilightState);
		return CsMath::min(*behaviourState, *twilightState);
	}

	if (behaviourState) {
		return *behaviourState;
	}

	if (twilightState) {
		return *twilightState;
	}

	return CS_SWITCH_CMD_VAL_FULLY_ON;
}

uint8_t SwitchAggregator::resolveOverrideState() {
	if (!overrideState || *overrideState != CS_SWITCH_CMD_VAL_SMART_ON) {
		return *overrideState;  // opaque or empty override is returned unchanged.
	}

	LOGSwitchAggregatorDebug("Override is smart on");
	std::optional<uint8_t> opt0 = {0};  // to simplify following expressions.

	// Only use behaviour and twilight state only when it has a value and when that value is not 0.
	if (behaviourState > opt0 && twilightState > opt0) {
		return CsMath::min(*behaviourState, *twilightState);
	}

	if (behaviourState > opt0) {
		return *behaviourState;
	}

	if (twilightState > opt0) {
		return *twilightState;
	}

	return 100;
}

bool SwitchAggregator::checkAndSetOwner(const cmd_source_with_counter_t& source) {
	if (source.source.type != CS_CMD_SOURCE_TYPE_BROADCAST) {
		// Non broadcast command can always set the switch.
		return true;
	}

	if (_ownerTimeoutCountdown == 0) {
		// Switch isn't claimed yet.
		_source                = source;
		_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
		return true;
	}

	if (_source.source.id != source.source.id) {
		// Switch is claimed by other source.
		LOGd("Already claimed by %u", _source.source.id);
		return false;
	}

	if (!CsUtils::isNewer(_source.count, source.count)) {
		// A command with newer counter has been received already.
		LOGSwitchAggregatorDebug("Old command count: %u, already got: %u", source.count, _source.count);
		return false;
	}

	_source                = source;
	_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
	return true;
}

void SwitchAggregator::handleGetBehaviourDebug(event_t& evt) {
	LOGd("handleGetBehaviourDebug");
	if (evt.result.buf.data == nullptr) {
		LOGd("ERR_BUFFER_UNASSIGNED");
		evt.result.returnCode = ERR_BUFFER_UNASSIGNED;
		return;
	}
	if (evt.result.buf.len < sizeof(behaviour_debug_t)) {
		LOGd("ERR_BUFFER_TOO_SMALL");
		evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}
	behaviour_debug_t* behaviourDebug = (behaviour_debug_t*)(evt.result.buf.data);

	behaviourDebug->overrideState =
			overrideState ? overrideState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE);
	behaviourDebug->behaviourState =
			behaviourState ? behaviourState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE);
	behaviourDebug->aggregatedState =
			aggregatedState ? aggregatedState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE);
	//	behaviourDebug->dimmerPowered = (smartSwitch.isDimmerCircuitPowered());

	evt.result.dataSize   = sizeof(behaviour_debug_t);
	evt.result.returnCode = ERR_SUCCESS;
}

void SwitchAggregator::addToSwitchHistory(const cs_switch_history_item_t& cmd) {
	LOGd("addToSwitchHistory val=%u state=%u srcType=%u srcId=%u",
		 cmd.value,
		 cmd.state.asInt,
		 cmd.source.type,
		 cmd.source.id);
	if (!_switchHistory.empty()) {
		auto prevEntry = _switchHistory[_switchHistory.size() - 1];
		if (memcmp(&prevEntry.source, &cmd.source, sizeof(cmd.source)) == 0
			&& (prevEntry.state.asInt == cmd.state.asInt) && (prevEntry.value == cmd.value)) {
			LOGd("Avoid duplicate entry");
			return;
		}
	}
	_switchHistory.push(cmd);
	printSwitchHistory();
}

void SwitchAggregator::printSwitchHistory() {
	LOGSwitchHistory("Switch history:");
	for (uint16_t i = 0; i < _switchHistory.size(); ++i) {
		__attribute__((unused)) const auto& iter = _switchHistory[i];
		LOGSwitchHistory(
				"  t=%u val=%u state=%u srcType=%u, srcId=%u",
				iter.timestamp,
				iter.value,
				iter.state.asInt,
				iter.source.type,
				iter.source.id);
	}
}

void SwitchAggregator::pushTestDataToHost() {
	TEST_PUSH_O(this, overrideState);
	TEST_PUSH_O(this, behaviourState);
	TEST_PUSH_O(this, twilightState);
	TEST_PUSH_O(this, aggregatedState);
}
