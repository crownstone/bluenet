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
#include <storage/cs_State.h>
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
	_smartSwitch.onUnexpextedIntensityChange([&](uint8_t newState) -> void { handleSwitchStateChange(newState); });
	_smartSwitch.init(board);

	// Allocate buffer.
	_switchHistory.init();

	listen();  // TODO: move to end of init.

	// TODO: only init twilightHandler and behaviourHandler in normal mode.
	_twilightHandler.init();
	_behaviourHandler.init();

	_overrideState = _smartSwitch.getIntendedState();
	pushTestDataToHost();

	initChildren();
}

void SwitchAggregator::switchPowered() {
	_smartSwitch.start();
}

std::vector<Component*> SwitchAggregator::getChildren() {
	return {&_behaviourHandler, &_twilightHandler};
}

// ================================== State updaters ==================================

bool SwitchAggregator::updateBehaviourHandlers() {
	LOGSwitchAggregatorDebug("updateBehaviourHandlers");

	std::optional<uint8_t> prevBehaviourState = _behaviourState;
	_behaviourHandler.update();
	_behaviourState = _behaviourHandler.getValue();

	_twilightHandler.update();
	_twilightState = _twilightHandler.getValue();

	if (!prevBehaviourState || !_behaviourState) {
		// don't allow override resets when no values are changed.
		return false;
	}

	return (prevBehaviourState != _behaviourState);
}

cs_ret_code_t SwitchAggregator::updateState(bool allowOverrideReset, const cmd_source_with_counter_t& source) {
	LOGSwitchAggregatorDebug("updateState allowOverrideReset=%u", allowOverrideReset);
	bool shouldResetOverrideState = false;

	if (_overrideState && _behaviourState && _aggregatedState) {
		bool overrideStateIsOn   = (*_overrideState != 0);
		bool aggregatedStateIsOn = (*_aggregatedState != 0);
		bool behaviourStateIsOn  = (*_behaviourState != 0);

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

	if (_overrideState && !shouldResetOverrideState) {
		_aggregatedState = resolveOverrideState(*_overrideState);
	}
	else if (_behaviourState) {
		// only use aggr. if no SwitchBehaviour conflict is found
		_aggregatedState = aggregatedBehaviourIntensity();
	}
	// If override and behaviour don't have an opinion, keep previous value.

	LOGSwitchAggregatorDebug(
			"overrideState=%u, behaviourState=%u, twilightState=%u, aggregatedState=%u",
			_overrideState ? _overrideState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE),
			_behaviourState ? _behaviourState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE),
			_twilightState ? _twilightState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE),
			_aggregatedState ? _aggregatedState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE));

	// attempt to update smartSwitch value
	cs_ret_code_t retCode = ERR_SUCCESS_NO_CHANGE;
	if (_aggregatedState) {
		retCode = _smartSwitch.set(*_aggregatedState);
		if (shouldResetOverrideState) {
			if (retCode == ERR_SUCCESS) {
				LOGSwitchAggregatorDebug("Reset override state");
				_overrideState = {};
			}
			else {
				_overrideState = _smartSwitch.getCurrentIntensity();
				LOGSwitchAggregatorDebug("Failed to reset override state, overrideState=%u", *_overrideState);
			}
		}
	}

	TYPIFY(EVT_BEHAVIOUR_OVERRIDDEN) eventData = _overrideState.has_value();
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
			if (_ownerTimeoutCountdown == 0) {
				--_ownerTimeoutCountdown;
			}

			if (_switchcraftDoubleTapCountdown) {
				--_switchcraftDoubleTapCountdown;
			}

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
						timestamp, *_aggregatedState, _smartSwitch.getActualState(), source.source));
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
				if (_overrideState) {
					Time now = SystemTime::now();
					LOGd("SwitchAggregator LastUserExit override state true");

					if (_behaviourHandler.requiresPresence(now)) {
						// if there exists a behaviour which is active at given time and
						//      	and it has a non-negated presence clause (that may not be satisfied)
						//      		clear override
						LOGd("clearing override state because last user exited sphere");
						_overrideState = {};
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
			_overrideState.reset();
			_behaviourState.reset();
			_twilightState.reset();
			_aggregatedState = 0;
			LOGd("Reset all");
			break;
		case CS_SWITCH_CMD_VAL_DEBUG_RESET_AGG:
			_aggregatedState.reset();
			LOGd("Reset aggregatedState");
			break;
		case CS_SWITCH_CMD_VAL_DEBUG_RESET_OVERRIDE:
			_overrideState.reset();
			LOGd("Reset overrideState");
			break;
		case CS_SWITCH_CMD_VAL_DEBUG_RESET_AGG_OVERRIDE:
			_overrideState.reset();
			_aggregatedState.reset();
			LOGd("Reset overrideState and aggregatedState");
			break;
	}
#endif

	switch (value) {
		case CS_SWITCH_CMD_VAL_TOGGLE: {
			// Toggle between 0 and on.
			uint8_t currentValue = _smartSwitch.getIntendedState();
			uint8_t newValue = 0;
			if (source.source.type == CS_CMD_SOURCE_TYPE_ENUM && source.source.id == CS_CMD_SOURCE_SWITCHCRAFT) {
				bool doubleTap = registerSwitchcraftEvent(currentValue);
				newValue = getStateIntentionSwitchcraft(currentValue, doubleTap);
			}
			else if (currentValue == 0) {
				// Switch is currently off, so switch on.
				newValue = CS_SWITCH_CMD_VAL_SMART_ON;
			}
			executeStateIntentionUpdate(newValue, source);
			return;
		}
		case CS_SWITCH_CMD_VAL_BEHAVIOUR: {
			_overrideState.reset();
			break;
		}
		case CS_SWITCH_CMD_VAL_SMART_ON: {
			_overrideState = value;
			break;
		}
		default: {
			if (value <= CS_SWITCH_CMD_VAL_FULLY_ON) {
				_overrideState = value;
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
			_overrideState = _smartSwitch.getCurrentIntensity();
		}
	}

	addToSwitchHistory(
			cs_switch_history_item_t(SystemTime::posix(), value, _smartSwitch.getActualState(), source.source));
	pushTestDataToHost();
}

bool SwitchAggregator::registerSwitchcraftEvent(uint8_t currentValue) {
	if (currentValue > 0) {
		_lastSwitchcraftOnValue = currentValue;
	}

	bool doubleTap = false;
	if (_switchcraftDoubleTapCountdown) {
		// TODO: cache this value?
		TYPIFY(STATE_SWITCHCRAFT_DOUBLE_TAP_ENABLED) doubleTapEnabled;
		State::getInstance().get(CS_TYPE::STATE_SWITCHCRAFT_DOUBLE_TAP_ENABLED, &doubleTapEnabled, sizeof(doubleTapEnabled));
		if (doubleTapEnabled) {
			LOGi("Double tap switchcraft");
			doubleTap = true;
		}
	}
	_switchcraftDoubleTapCountdown = SWITCHCRAFT_DOUBLE_TAP_TIME_MS / TICK_INTERVAL_MS;
	return doubleTap;
}

uint8_t SwitchAggregator::getStateIntentionSwitchcraft(uint8_t currentValue, bool doubleTap) {
	if (currentValue != 0) {
		// Switch is currently on, so switch off.
		return 0;
	}

	if (!doubleTap || _lastSwitchcraftOnValue == 0) {
		// No double tap: just turn on (according to behaviour).
		return CS_SWITCH_CMD_VAL_SMART_ON;
	}

	// Double tap: on value toggles between fully on, and a dimmed value.
	if (_lastSwitchcraftOnValue != CS_SWITCH_CMD_VAL_FULLY_ON) {
		// Treat it the same way as manually setting the switch fully on.
		return CS_SWITCH_CMD_VAL_FULLY_ON;
	}
	LOGSwitchAggregatorDebug("Resolve double tap dim value.");

	// Check if any active behaviour has a dimmed value, and if so, use that.
	uint8_t resolved = resolveOverrideState(CS_SWITCH_CMD_VAL_SMART_ON);
	if (0 < resolved && resolved < CS_SWITCH_CMD_VAL_FULLY_ON) {
		// This is not an override state.
		return CS_SWITCH_CMD_VAL_SMART_ON;
	}

	// TODO: cache this value?
	TYPIFY(STATE_DEFAULT_DIM_VALUE) defaultDimValue;
	State::getInstance().get(CS_TYPE::STATE_DEFAULT_DIM_VALUE, &defaultDimValue, sizeof(defaultDimValue));

	// Use the configured default dim value if it's set.
	if (defaultDimValue != 0) {
		// Treat it the same way as manually setting the dim value.
		defaultDimValue = std::min(defaultDimValue, static_cast<uint8_t>(CS_SWITCH_CMD_VAL_FULLY_ON));
		return defaultDimValue;
	}

	// Use the default dim value.
	// Treat it the same way as manually setting the dim value.
	return DEFAULT_DIM_VALUE;
}

void SwitchAggregator::handleSwitchStateChange(uint8_t newIntensity) {
	LOGi("handleSwitchStateChange %u", newIntensity);
	// TODO: 21-01-2020 This is not a user intent, so store in a different variable, and then figure out what to do with
	// it.
	_overrideState = newIntensity;
	pushTestDataToHost();
	// TODO: get the correct source.
	addToSwitchHistory(cs_switch_history_item_t(
			SystemTime::posix(), newIntensity, _smartSwitch.getActualState(), cmd_source_t(CS_CMD_SOURCE_INTERNAL)));
}

// ========================= Misc =========================

uint8_t SwitchAggregator::aggregatedBehaviourIntensity() {
	LOGSwitchAggregatorDebug("aggregatedBehaviourIntensity");

	if (_behaviourState && _twilightState) {
		LOGSwitchAggregatorDebug("Returning min of behaviour(%u) and twilight(%u)", *_behaviourState, *_twilightState);
		return CsMath::min(*_behaviourState, *_twilightState);
	}

	if (_behaviourState) {
		return *_behaviourState;
	}

	if (_twilightState) {
		return *_twilightState;
	}

	return CS_SWITCH_CMD_VAL_FULLY_ON;
}

uint8_t SwitchAggregator::resolveOverrideState(uint8_t overrideState) {
	if (overrideState != CS_SWITCH_CMD_VAL_SMART_ON) {
		return overrideState;
	}

	LOGSwitchAggregatorDebug("Override is smart on");
	// To simplify comparisons with optional values, create an optional variable with value 0.
	std::optional<uint8_t> opt0 = {0};

	// Only use behaviour and twilight state only when it has a value and when that value is not 0.
	if (_behaviourState > opt0 && _twilightState > opt0) {
		return CsMath::min(*_behaviourState, *_twilightState);
	}

	if (_behaviourState > opt0) {
		return *_behaviourState;
	}

	if (_twilightState > opt0) {
		return *_twilightState;
	}

	return CS_SWITCH_CMD_VAL_FULLY_ON;
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
			_overrideState ? _overrideState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE);
	behaviourDebug->behaviourState =
			_behaviourState ? _behaviourState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE);
	behaviourDebug->aggregatedState =
			_aggregatedState ? _aggregatedState.value() : static_cast<uint8_t>(CS_SWITCH_CMD_VAL_NONE);
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
	TEST_PUSH_O(this, _overrideState);
	TEST_PUSH_O(this, _behaviourState);
	TEST_PUSH_O(this, _twilightState);
	TEST_PUSH_O(this, _aggregatedState);
}
