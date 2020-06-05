/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <switch/cs_SwitchAggregator.h>
#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>
#include <presence/cs_PresenceHandler.h>
#include <util/cs_Utils.h>

#include <test/cs_Test.h>

#include <optional>

#define LOGSwitchAggregatorDebug LOGnone
#define LOGSwitchAggregator_Evt LOGd


// ========================= Public ========================

void SwitchAggregator::init(const boards_config_t& board) {
	smartSwitch.onUnexpextedIntensityChange([&](uint8_t newState) -> void {
		handleSwitchStateChange(newState);
	});
	smartSwitch.init(board);

	listen();

	twilightHandler.listen();
	behaviourHandler.listen();

	overrideState = smartSwitch.getIntendedState();
}

SwitchAggregator& SwitchAggregator::getInstance() {
	static SwitchAggregator instance;

	return instance;
}

void SwitchAggregator::switchPowered() {
	smartSwitch.start();
}

// ================================== State updaters ==================================

bool SwitchAggregator::updateBehaviourHandlers() {

	std::optional<uint8_t> prevBehaviourState = behaviourState;
	behaviourHandler.update();
	behaviourState = behaviourHandler.getValue();

	twilightHandler.update();
	twilightState = twilightHandler.getValue();

	if ( !prevBehaviourState || !behaviourState ) {
		// don't allow override resets when no values are changed.
		return false;
	}

	return (prevBehaviourState != behaviourState);
}

cs_ret_code_t SwitchAggregator::updateState(bool allowOverrideReset, const cmd_source_with_counter_t& source) {

	bool shouldResetOverrideState = false;

	if (overrideState && behaviourState && aggregatedState) {
		bool overrideStateIsOn =   (*overrideState   != 0);
		bool aggregatedStateIsOn = (*aggregatedState != 0);
		bool behaviourStateIsOn =  (*behaviourState  != 0);

		bool overrideMatchedAggregated =   (overrideStateIsOn  == aggregatedStateIsOn);
		bool behaviourWantsToChangeState = (behaviourStateIsOn != aggregatedStateIsOn);

		if (overrideMatchedAggregated && behaviourWantsToChangeState && allowOverrideReset) {
			LOGd("resetting overrideState overrideStateIsOn=%u aggregatedStateIsOn=%u behaviourStateIsOn=%u", overrideStateIsOn, aggregatedStateIsOn, behaviourStateIsOn);
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



	LOGSwitchAggregatorDebug("updateState");
	static uint8_t callcount = 0;
	if (++callcount > 10) {
		callcount = 0;
		printStatus();
	}

	// attempt to update smartSwitch value
	cs_ret_code_t retCode = ERR_SUCCESS_NO_CHANGE;
	if (aggregatedState) {
		retCode = smartSwitch.set(*aggregatedState);
		if (shouldResetOverrideState && retCode == ERR_SUCCESS) {
			LOGSwitchAggregatorDebug("Reset override state");
			overrideState = {};
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
		case CS_TYPE::CMD_SWITCH_AGGREGATOR_RESET: {
			overrideState.reset();
			behaviourState.reset();
			twilightState.reset();
			aggregatedState = 0;
			pushTestDataToHost();
			LOGd("handled switch aggregator reset command");
			break;
		}
		case CS_TYPE::CMD_GET_SWITCH_HISTORY: {
			cs_switch_history_header_t header;
			header.count = _switchHistory.size();
			cs_buffer_size_t requiredSize = sizeof(header) + header.count * sizeof(cs_switch_history_item_t);
			if (event.result.buf.len < requiredSize) {
				event.result.returnCode = ERR_BUFFER_TOO_SMALL;
				break;
			}
			memcpy(event.result.buf.data, &header, sizeof(header));
			int offset = sizeof(header);
			for (const auto& item : _switchHistory) {
				memcpy(event.result.buf.data + offset, &item, sizeof(item));
				offset += sizeof(item);
			}
			event.result.dataSize = requiredSize;
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
			break;
		}
		case CS_TYPE::STATE_TIME: {
			bool behaviourValueChanged = updateBehaviourHandlers();
			uint8_t behaviourRuleIndex = 255; // TODO: set correct value.
			cmd_source_with_counter_t source(cmd_source_t(CS_CMD_SOURCE_TYPE_BEHAVIOUR, behaviourRuleIndex));
			updateState(behaviourValueChanged, source);
			if (behaviourValueChanged) {
				addToSwitchHistory(cs_switch_history_item_t(SystemTime::posix(), *aggregatedState, smartSwitch.getActualState(), source.source));
			}
			break;
		}
		case CS_TYPE::EVT_TIME_SET: {
			bool behaviourValueChanged = updateBehaviourHandlers();
			uint8_t behaviourRuleIndex = 255; // TODO: set correct value.
			cmd_source_with_counter_t source(cmd_source_t(CS_CMD_SOURCE_TYPE_BEHAVIOUR, behaviourRuleIndex));
			updateState(behaviourValueChanged, source);
			if (behaviourValueChanged) {
				addToSwitchHistory(cs_switch_history_item_t(SystemTime::posix(), *aggregatedState, smartSwitch.getActualState(), source.source));
			}
			break;
		}
		default:{
			return false;
		}
	}
	return true;
}

bool SwitchAggregator::handlePresenceEvents(event_t& event) {
	if (event.type == CS_TYPE::EVT_PRESENCE_MUTATION) {
		PresenceHandler::MutationType mutationtype = *reinterpret_cast<PresenceHandler::MutationType*>(event.data);

		switch (mutationtype) {
			case PresenceHandler::MutationType::LastUserExitSphere: {
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
						uint8_t behaviourRuleIndex = 255; // TODO: set correct value.
						updateState(true, cmd_source_t(CS_CMD_SOURCE_TYPE_BEHAVIOUR, behaviourRuleIndex));
					}
				}
				break;
			}
			default:
				break;
		}

		LOGSwitchAggregator_Evt("SwitchAggregator::handlePresence");
		return true;
	}

	return false;
}

bool SwitchAggregator::handleStateIntentionEvents(event_t& event) {
	switch (event.type) {
		// ============== overrideState Events ==============
		case CS_TYPE::CMD_SWITCH_ON: {
			LOGSwitchAggregator_Evt("CMD_SWITCH_ON",__func__);

			executeStateIntentionUpdate(100, event.source);
			break;
		}
		case CS_TYPE::CMD_SWITCH_OFF: {
			LOGSwitchAggregator_Evt("CMD_SWITCH_OFF",__func__);

			executeStateIntentionUpdate(0, event.source);
			break;
		}
		case CS_TYPE::CMD_SWITCH: {
			LOGSwitchAggregator_Evt("CMD_SWITCH",__func__);

			TYPIFY(CMD_SWITCH)* packet = (TYPIFY(CMD_SWITCH)*) event.data;
			LOGd("packet intensity: %d, source: type=%u id=%u", packet->switchCmd, event.source.source.type, event.source.source.id);
			if (!checkAndSetOwner(event.source)) {
				LOGSwitchAggregatorDebug("not executing, checkAndSetOwner returned false");
				event.result.returnCode = ERR_NO_ACCESS;
				return false;
			}

			executeStateIntentionUpdate(packet->switchCmd, event.source);

			break;
		}
		case CS_TYPE::CMD_SWITCH_TOGGLE: {
			LOGSwitchAggregator_Evt("CMD_SWITCH_TOGGLE",__func__);
			executeStateIntentionUpdate(smartSwitch.getIntendedState() == 0 ? 255 : 0, event.source);
			break;
		}
		default:{
			// event not handled.
			return false;
		}
	}

	event.result.returnCode = ERR_SUCCESS;
	return true;
}

void SwitchAggregator::executeStateIntentionUpdate(uint8_t value, cmd_source_with_counter_t& source) {
	auto prev_overrideState = overrideState;

#ifdef DEBUG
	if (value == 0xFE) {
		overrideState.reset();
		LOGd("Resetting overrideState");
	} else if (value == 0xFD) {
		aggregatedState.reset();
		LOGd("Resetting aggregatedState");
	} else if (value == 0xFC) {
		overrideState.reset();
		aggregatedState.reset();
		LOGd("Resetting overrideState and aggregatedState");
	} else {
		overrideState = value;
	}
#else
	overrideState = value;
#endif

	// don't allow override reset in updateState, it has just been requested to be
	// set to `value`
	if (updateState(false, source) == ERR_NO_ACCESS) {
		// failure to set the smartswitch. It seems to be locked.
		LOGSwitchAggregatorDebug("Reverting to previous value, no access to smartswitch");
		overrideState = prev_overrideState;
	}
	addToSwitchHistory(cs_switch_history_item_t(SystemTime::posix(), value, smartSwitch.getActualState(), source.source));
}

void SwitchAggregator::handleSwitchStateChange(uint8_t newIntensity) {
	LOGi("handleSwitchStateChange %u", newIntensity);
	// TODO: 21-01-2020 This is not a user intent, so store in a different variable, and then figure out what to do with it.
	overrideState = newIntensity;
	// TODO: get the correct source.
	addToSwitchHistory(cs_switch_history_item_t(SystemTime::posix(), newIntensity, smartSwitch.getActualState(), cmd_source_t(CS_CMD_SOURCE_INTERNAL)));
}

// ========================= Misc =========================

uint8_t SwitchAggregator::aggregatedBehaviourIntensity() {
	LOGSwitchAggregatorDebug("aggregatedBehaviourIntensity called");

	if (behaviourState && twilightState) {
		LOGSwitchAggregatorDebug("returning min of behaviour(%d) and twilight(%d)", *behaviourState, *twilightState);
		return CsMath::min(*behaviourState, *twilightState);
	}

	if (behaviourState) {
		return *behaviourState;
	}

	if (twilightState) {
		return *twilightState;
	}

	return 100;
}

std::optional<uint8_t> SwitchAggregator::resolveOverrideState() {
	LOGSwitchAggregatorDebug("resolveOverrideState called");

	if (!overrideState || *overrideState != 0xff) {
		return overrideState;  // opaque or empty override is returned unchanged.
	}

	LOGSwitchAggregatorDebug("translucent override state");

	std::optional<uint8_t> opt0 = {0}; // to simplify following expressions.

	if (behaviourState > opt0 && twilightState > opt0) {
		// both exists and are positive
		return CsMath::min(*behaviourState, *twilightState);
	}

	if (behaviourState > opt0) {
		return behaviourState;
	}

	if (twilightState > opt0) {
		return twilightState;
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
		_source = source;
		_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
		return true;
	}

	if (_source.source.id != source.source.id) {
		// Switch is claimed by other source.
		LOGd("Already claimed by %u", _source.source.id);
		return false;
	}

	if (!BLEutil::isNewer(_source.count, source.count)) {
		// A command with newer counter has been received already.
		LOGd("Old command: %u, already got: %u", source.count, _source.count);
		return false;
	}

	_source = source;
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

	behaviourDebug->overrideState = overrideState ? overrideState.value() : 254;
	behaviourDebug->behaviourState = behaviourState ? behaviourState.value() : 254;
	behaviourDebug->aggregatedState = aggregatedState ? aggregatedState.value() : 254;
	//	behaviourDebug->dimmerPowered = (smartSwitch.isDimmerCircuitPowered());

	evt.result.dataSize = sizeof(behaviour_debug_t);
	evt.result.returnCode = ERR_SUCCESS;
}

void SwitchAggregator::addToSwitchHistory(const cs_switch_history_item_t& cmd) {
	LOGd("addToSwitchHistory val=%u state=%u srcType=%u, srcId=%u", cmd.value, cmd.state.asInt, cmd.source.type, cmd.source.id);
	if (_switchHistory.size() == _maxSwitchHistoryItems) {
		_switchHistory.erase(_switchHistory.begin());
	}
	_switchHistory.push_back(cmd);
	printSwitchHistory();
}

void SwitchAggregator::printSwitchHistory() {
	LOGd("Switch history:");
	// Queue doesn't have an iterator..
//	for (size_t i = 0; i < _switchHistory.size(); ++i) {
//		auto item = _switchHistory.front();
//		LOGd("  t=%u val=%u state=%u srcType=%u, srcId=%u", item.timestamp, item.value, item.state.asInt, item.source.type, item.source.id);
//		_switchHistory.pop();
//		_switchHistory.push(item);
//	}
	for (__attribute__((unused)) const auto& iter : _switchHistory) {
		LOGd("  t=%u val=%u state=%u srcType=%u, srcId=%u", iter.timestamp, iter.value, iter.state.asInt, iter.source.type, iter.source.id);
	}
}

void SwitchAggregator::printStatus() {
	LOGd("^ overrideState: %02d",   OptionalUnsignedToInt(overrideState));
	LOGd("| behaviourState: %02d",  OptionalUnsignedToInt(behaviourState));
	LOGd("| twilightState: %02d",   OptionalUnsignedToInt(twilightState));
	LOGd("v aggregatedState: %02d", OptionalUnsignedToInt(aggregatedState));
}

void SwitchAggregator::pushTestDataToHost() {
	TEST_PUSH_O(this, overrideState);
	TEST_PUSH_O(this, behaviourState);
	TEST_PUSH_O(this, twilightState);
	TEST_PUSH_O(this, aggregatedState);
}
