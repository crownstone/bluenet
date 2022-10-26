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
#include <testaccess/cs_BehaviourStore.h>
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


#include <common/cs_Component.h>

class MockCrownstone : public Component {
public:
	SwitchAggregator _switchAggregator;
	BehaviourStore _behaviourStore;
	BehaviourHandler _behaviourHandler;
	PresenceHandler _presenceHandler;
	SystemTime _systemTime;

	EventDispatcher& _eventDispatcher = EventDispatcher::getInstance();
	Storage& _storage                 = Storage::getInstance();
	State& _state                     = State::getInstance();

	virtual std::vector<Component*> getChildren() override {
		return {&_switchAggregator, &_behaviourStore, &_behaviourHandler,  & _presenceHandler};
	}

	MockCrownstone() {
		TestAccess<BehaviourHandler>::setup(_behaviourHandler, &_presenceHandler, &_behaviourStore);
	}

	virtual ~MockCrownstone() { LOGi("MockCrownstone::~MockCrownstone()");}
};

class TestCrownstone {
public:
	TestAccess<Storage> _storage;
	TestAccess<SwitchBehaviour> _switchBehaviour;
	TestAccess<SwitchAggregator> _switchAggregator;
	TestAccess<BehaviourStore> _behaviourStore;

	TestCrownstone(MockCrownstone& mock)
			: _storage(mock._storage), _switchBehaviour(), _switchAggregator(mock._switchAggregator), _behaviourStore() {
	}

	~TestCrownstone() {LOGi("TestCrownstone::~TestCrownstone()"); }
};

void fastForwardS(int timeS) {
	// (SystemTime::tick and Crownstone::tick are not the same.)
	for (int i = 0 ; i < timeS; i++ ) {
		TestAccess<SystemTime>::fastForwardS(1);
		TestAccess<Crownstone>::tick();
	}
}

int main() {
	// firmware components
	MockCrownstone crownstone;
	crownstone.parentAllChildren();

    // test objects with private access
	TestCrownstone test(crownstone);

	// initialization
    boards_config_t board;
    init(&board);
    asHostFullyFeatured(&board);

    crownstone._storage.init();
    crownstone._state.init(&board);

    uint8_t stateDimmingAllowed = 1;
    crownstone._state.set(CS_TYPE::CONFIG_DIMMING_ALLOWED, &stateDimmingAllowed, sizeof(stateDimmingAllowed));

	uint8_t stateSwitchLocked = 0;
	crownstone._state.set(CS_TYPE::CONFIG_SWITCH_LOCKED, &stateSwitchLocked, sizeof(stateSwitchLocked));

	uint8_t stateSwitchState = 0;
	crownstone._state.set(CS_TYPE::STATE_SWITCH_STATE, &stateSwitchState, sizeof(stateSwitchState));

	TYPIFY(STATE_OPERATION_MODE) mode = static_cast<uint8_t>(OperationMode::OPERATION_MODE_NORMAL);
	crownstone._state.set(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));


    crownstone._systemTime.init();
    TestAccess<SystemTime>::fastForwardS(120);
    TestAccess<SystemTime>::setTime(Time(DayOfWeek::Tuesday, 12,30));
    SystemTime::setSunTimes(sun_time_t{});

	test._switchBehaviour.from = TimeOfDay(9,0,0);
	test._switchBehaviour.until = TimeOfDay(19,0,0);
	test._switchBehaviour.intensity = 90;

	TYPIFY(STATE_OPERATION_MODE) modeStored;
	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, &modeStored, sizeof(modeStored));

	crownstone._switchAggregator.init(board);
	crownstone._switchAggregator.switchPowered();

	crownstone._behaviourStore.init();
	crownstone._behaviourStore.listen();

	crownstone._presenceHandler.init();


	// construct switch behaviours for the test

	// to keep track of used indices.
	uint8_t indexer = 0;

	test._switchBehaviour.presencecondition.predicate._condition = PresencePredicate::Condition::VacuouslyTrue;
	typename TestAccess<SwitchBehaviour>::BehaviourStoreItem noPresenceRequired = test._switchBehaviour.getItem(indexer++);

	test._switchBehaviour.presencecondition.predicate._condition = PresencePredicate::Condition::AnyoneInSphere;
	test._switchBehaviour.presencecondition.predicate._presence._bitmask = ~0b00; // all ones
	auto requiresAnyoneInSphere = test._switchBehaviour.getItem(indexer++);

	test._switchBehaviour.presencecondition.predicate._condition = PresencePredicate::Condition::NooneInSphere;
	test._switchBehaviour.presencecondition.predicate._presence._bitmask = ~0b00; // all ones
	auto requiresNooneInSphere = test._switchBehaviour.getItem(indexer++);

	test._switchBehaviour.presencecondition.predicate._condition = PresencePredicate::Condition::AnyoneInSelectedRooms;
	test._switchBehaviour.presencecondition.predicate._presence._bitmask = 0b01; // only in room 0.
	auto requiresAnyoneInRoom = test._switchBehaviour.getItem(indexer++);

	// tests

    // add behaviour that doesn't have presence requirement,
	// check expected resolution by behaviourhandler.
	test._behaviourStore.replaceBehaviour(noPresenceRequired._index, noPresenceRequired._behaviour);

	if(!checkCase(crownstone._behaviourHandler, crownstone._behaviourStore, noPresenceRequired._index, absent(), __LINE__)) return 1;
	if(!checkCase(crownstone._behaviourHandler, crownstone._behaviourStore, noPresenceRequired._index, present(), __LINE__)) return 1;

    // add behaviour which requires presence,
	// check expected resolution by behaviourhandler.
	test._behaviourStore.replaceBehaviour(requiresAnyoneInSphere._index, requiresAnyoneInSphere._behaviour);
	if(!checkCase(crownstone._behaviourHandler, crownstone._behaviourStore, noPresenceRequired._index, absent(), __LINE__)) return 1;
	if(!checkCase(crownstone._behaviourHandler, crownstone._behaviourStore, requiresAnyoneInSphere._index, present(), __LINE__)) return 1;

	// set override and presence
	std::cout << __LINE__ << " current presence: " << crownstone._presenceHandler.getCurrentPresenceDescription() << std::endl;
	crownstone._presenceHandler.registerPresence(PresenceHandler::ProfileLocation{.profile = 0, .location = 0});
	test._switchAggregator.setOverrideState({50});
	crownstone._behaviourHandler.update();
	fastForwardS(1000);
	crownstone._behaviourHandler.update();

	std::cout << __LINE__ << " current presence: " << crownstone._presenceHandler.getCurrentPresenceDescription() << std::endl;

	// NOTE: gracePeriodForPresenceIsActive is making trouble here

    // add behaviour which requires absence
	test._behaviourStore.replaceBehaviour(requiresNooneInSphere._index, requiresNooneInSphere._behaviour);
	if(!checkCase(crownstone._behaviourHandler, crownstone._behaviourStore, requiresNooneInSphere._index, absent(), __LINE__)) return 1;
	if(!checkCase(crownstone._behaviourHandler, crownstone._behaviourStore, requiresAnyoneInSphere._index, present(), __LINE__)) return 1;

	if(test._switchAggregator.getOverrideState() != std::nullopt) {
		LOGw("FAILED: uploading an immediately active behaviour should clear the override state.");
		return 1;
	}

    // add behaviour which requires presence (shouldn't release override?)

    return 0;
}
