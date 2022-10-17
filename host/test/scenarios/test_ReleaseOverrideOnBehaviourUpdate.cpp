/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 12, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <utils/cs_checks.h>

#include <boards/cs_HostBoardFullyFeatured.h>
#include <behaviour/cs_BehaviourStore.h>
#include <events/cs_EventDispatcher.h>
#include <logging/cs_Logger.h>
#include <presence/cs_PresenceCondition.h>
#include <presence/cs_PresenceHandler.h>
#include <switch/cs_SwitchAggregator.h>
#include <testaccess/cs_PresenceDescription.h>
#include <testaccess/cs_BehaviourHandler.h>
#include <testaccess/cs_SwitchBehaviour.h>
#include <testaccess/cs_SwitchAggregator.h>
#include <testaccess/cs_Crownstone.h>
#include <testaccess/cs_SystemTime.h>
#include <testaccess/cs_Storage.h>

#include <utils/cs_iostream.h>
#include <utils/cs_PresenceBuilder.h>

#include <iostream>
#include <drivers/cs_PWM.h>

#include <logging/cs_Logger.h>
#include <events/cs_SwitchCommands.h>
#include <storage/cs_State.h>


void fastForwardS(int timeS) {
	// (SystemTime::tick and Crownstone::tick are not the same.)
	TestAccess<SystemTime>::fastForwardS(timeS);
	TestAccess<Crownstone>::tick();
}

int main() {
	// firmware components
    SwitchAggregator _switchAggregator;
    BehaviourStore _behaviourStore;
    BehaviourHandler _behaviourHandler;
    PresenceHandler _presenceHandler;
    SystemTime _systemTime;
    EventDispatcher &_eventDispatcher = EventDispatcher::getInstance();
	Storage& storage = Storage::getInstance();
	State& state = State::getInstance();

    // test objects with private access
	TestAccess<Storage> testStorage(storage);
	TestAccess<SwitchBehaviour> switchBehaviourFactory;
	TestAccess<SwitchAggregator> testSwitchAggregator(_switchAggregator);

	// initialization
    boards_config_t board;
    init(&board);
    asHostFullyFeatured(&board);

    storage.init();
    state.init(&board);

    uint8_t stateDimmingAllowed = 1;
    state.set(CS_TYPE::CONFIG_DIMMING_ALLOWED, &stateDimmingAllowed, sizeof(stateDimmingAllowed));

	uint8_t stateSwitchLocked = 0;
	state.set(CS_TYPE::CONFIG_SWITCH_LOCKED, &stateSwitchLocked, sizeof(stateSwitchLocked));

	uint8_t stateSwitchState = 0;
	state.set(CS_TYPE::STATE_SWITCH_STATE, &stateSwitchState, sizeof(stateSwitchState));

	TYPIFY(STATE_OPERATION_MODE) mode = static_cast<uint8_t>(OperationMode::OPERATION_MODE_NORMAL);
	state.set(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));

	TestAccess<BehaviourHandler>::setup(_behaviourHandler, &_presenceHandler, &_behaviourStore);

    _systemTime.init();
    TestAccess<SystemTime>::fastForwardS(120);
    TestAccess<SystemTime>::setTime(Time(DayOfWeek::Tuesday, 12,30));
    SystemTime::setSunTimes(sun_time_t{});

	switchBehaviourFactory.from = TimeOfDay(9,0,0);
	switchBehaviourFactory.until = TimeOfDay(19,0,0);
	switchBehaviourFactory.intensity = 90;

	TYPIFY(STATE_OPERATION_MODE) modeStored;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &modeStored, sizeof(modeStored));

	_switchAggregator.init(board);
	_switchAggregator.switchPowered();

	// construct switch behaviours for the test

	// to keep track of used indices.
	uint8_t indexer = 0;

	switchBehaviourFactory.presencecondition.predicate._condition = PresencePredicate::Condition::VacuouslyTrue;
	typename TestAccess<SwitchBehaviour>::BehaviourStoreItem noPresenceRequired = switchBehaviourFactory.getItem(indexer++);

	switchBehaviourFactory.presencecondition.predicate._condition = PresencePredicate::Condition::AnyoneInSphere;
	switchBehaviourFactory.presencecondition.predicate._presence._bitmask = ~0b00; // all ones
	auto requiresAnyoneInSphere = switchBehaviourFactory.getItem(indexer++);

	switchBehaviourFactory.presencecondition.predicate._condition = PresencePredicate::Condition::NooneInSphere;
	switchBehaviourFactory.presencecondition.predicate._presence._bitmask = ~0b00; // all ones
	auto requiresNooneInSphere = switchBehaviourFactory.getItem(indexer++);

	switchBehaviourFactory.presencecondition.predicate._condition = PresencePredicate::Condition::AnyoneInSelectedRooms;
	switchBehaviourFactory.presencecondition.predicate._presence._bitmask = 0b01; // only in room 0.
	auto requiresAnyoneInRoom = switchBehaviourFactory.getItem(indexer++);

	// tests

    // add behaviour that doesn't have presence requirement,
	// check expected resolution by behaviourhandler.
	_behaviourStore.replaceBehaviour(noPresenceRequired._index, noPresenceRequired._behaviour);

	if(!checkCase(_behaviourHandler, _behaviourStore, noPresenceRequired._index, absent(), __LINE__)) return 1;
	if(!checkCase(_behaviourHandler, _behaviourStore, noPresenceRequired._index, present(), __LINE__)) return 1;

    // add behaviour which requires presence,
	// check expected resolution by behaviourhandler.
	_behaviourStore.replaceBehaviour(requiresAnyoneInSphere._index, requiresAnyoneInSphere._behaviour);
	if(!checkCase(_behaviourHandler, _behaviourStore, noPresenceRequired._index, absent(), __LINE__)) return 1;
	if(!checkCase(_behaviourHandler, _behaviourStore, requiresAnyoneInSphere._index, present(), __LINE__)) return 1;

	// set override and presence
	_presenceHandler.registerPresence(PresenceHandler::ProfileLocation{.profile = 0, .location = 0});
	testSwitchAggregator.setOverrideState({50});
	fastForwardS(1);

    // add behaviour which requires absence
	_behaviourStore.replaceBehaviour(requiresNooneInSphere._index, requiresNooneInSphere._behaviour);
	if(!checkCase(_behaviourHandler, _behaviourStore, requiresNooneInSphere._index, absent(), __LINE__)) return 1;
	if(!checkCase(_behaviourHandler, _behaviourStore, requiresAnyoneInSphere._index, present(), __LINE__)) return 1;

	if(testSwitchAggregator.getOverrideState() != std::nullopt) {
		LOGw("uploading an immediately active behaviour should clear the override state.");
		return 1;
	}

    // add behaviour which requires presence (shouldn't release override?)

    return 0;
}
