/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <utils/cs_checks.h>
#include <testaccess/cs_PresenceDescription.h>
#include <testaccess/cs_SwitchBehaviour.h>
#include <testaccess/cs_Behaviour.h>
#include <testaccess/cs_SystemTime.h>

#include <behaviour/cs_BehaviourStore.h>
#include <presence/cs_PresenceHandler.h>
#include <switch/cs_SwitchAggregator.h>
#include <events/cs_EventDispatcher.h>
#include <presence/cs_PresenceCondition.h>

#include <utils/cs_PresenceBuilder.h>
#include <utils/date.h>


SwitchBehaviour* getBehaviourPresent(PresencePredicate::Condition condition, bool multipleRooms = false){
    TestAccess<SwitchBehaviour> testAccessSwitchBehaviour;

	testAccessSwitchBehaviour.intensity                              = 95;
	testAccessSwitchBehaviour.presencecondition.predicate._condition = condition;
	testAccessSwitchBehaviour.presencecondition.predicate._presence._bitmask =
			multipleRooms ? roomBitmaskSingle() : roomBitmaskMulti();
	auto switchBehaviourInRoom = new SwitchBehaviour(testAccessSwitchBehaviour.get());
	return switchBehaviourInRoom;
}

SwitchBehaviour* getBehaviourNarrowTimesWithTrivialPresence() {
	TestAccess<SwitchBehaviour> testAccessSwitchBehaviour;

	testAccessSwitchBehaviour.from                                   = TimeOfDay(11, 59, 0);
	testAccessSwitchBehaviour.until                                  = TimeOfDay(12, 1, 0);
	testAccessSwitchBehaviour.intensity                              = 1;
	testAccessSwitchBehaviour.presencecondition.predicate._condition = PresencePredicate::Condition::VacuouslyTrue;
	auto switchBehaviourPresenceIrrelevant = new SwitchBehaviour(testAccessSwitchBehaviour.get());
	return switchBehaviourPresenceIrrelevant;
}



struct TestBehaviours {
	// the index in the behaviourstore where this behaviour will be put.
	static constexpr int verySpecific      = 0;
	static constexpr int vacuouslyTrue     = 1;
	static constexpr int anyoneInSphere    = 2;
	static constexpr int nooneInSphere     = 3;
	static constexpr int anyoneInRoom      = 4;
	static constexpr int nooneInRoom       = 5;
	static constexpr int anyoneInRoomMulti = 6;
	static constexpr int nooneInRoomMulti  = 7;

	/**
	 * this method allocates on the heap. no need to clean up, the behaviourstore will take
	 * ownership and delete items properly.
	 */
	static SwitchBehaviour* makeTestSwitchBehaviour(int index) {
		switch (index) {
			case verySpecific: return getBehaviourNarrowTimesWithTrivialPresence();
			case vacuouslyTrue: return getBehaviourPresent(PresencePredicate::Condition::VacuouslyTrue);
			case anyoneInSphere: return getBehaviourPresent(PresencePredicate::Condition::AnyoneInSphere);
			case nooneInSphere: return getBehaviourPresent(PresencePredicate::Condition::NooneInSphere);
			case anyoneInRoom: return getBehaviourPresent(PresencePredicate::Condition::AnyoneInSelectedRooms);
			case nooneInRoom: return getBehaviourPresent(PresencePredicate::Condition::NooneInSelectedRooms);
			case anyoneInRoomMulti:
				return getBehaviourPresent(PresencePredicate::Condition::AnyoneInSelectedRooms, true);
			case nooneInRoomMulti: return getBehaviourPresent(PresencePredicate::Condition::NooneInSelectedRooms, true);
			default: return nullptr;
		}
	}
};

/**
 * clears the store, then adds the behaviours specified in the indices vector constructing them
 * using TestBehaviours::makeTestSwitchBehaviour.
 */
void setupBehaviourStore(BehaviourStore& store, std::vector<int> indices) {
	TestAccess<BehaviourStore>::clearActiveBehavioursArray(store);
	store.replaceBehaviour(
			TestBehaviours::verySpecific, TestBehaviours::makeTestSwitchBehaviour(TestBehaviours::verySpecific));
	for (int index : indices) {
		store.replaceBehaviour(index, TestBehaviours::makeTestSwitchBehaviour(index));
	}
}

/**
 * This test asserts correct conflict resolution when the only difference in stored behaviours
 * is the presence condition. And conflict resolution in favor of any non-trivial presence clause
 * over a behaviour with a more specific from-until interval.
 * @return
 */
int main() {
	SwitchAggregator _switchAggregator;
	BehaviourStore _behaviourStore;
	BehaviourHandler _behaviourHandler;
	PresenceHandler _presenceHandler;
	SystemTime _systemTime;
	EventDispatcher& _eventDispatcher = EventDispatcher::getInstance();

	TestAccess<BehaviourHandler>::setup(_behaviourHandler, &_presenceHandler, &_behaviourStore);
	TestAccess<SystemTime>::fastForwardS(120);

	// test cases

	// any presence based behaviour must win from this specific behaviour when it is valid
	setupBehaviourStore(
			_behaviourStore,
			{
					// no presence behaviours added
			});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, present(), __LINE__)) return 1;

	// add unspecific, non-presence. should still resolve to the verySpecific one.
	setupBehaviourStore(_behaviourStore, {TestBehaviours::vacuouslyTrue});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, present(), __LINE__)) return 1;

	// should only resolve to the verySpecific one when not in sphere.
	setupBehaviourStore(_behaviourStore, {TestBehaviours::vacuouslyTrue, TestBehaviours::anyoneInSphere});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInSphere, present(), __LINE__)) return 1;

	// adding the opposite presence based behaviour, overriding verySpecific.
	setupBehaviourStore(
			_behaviourStore,
			{TestBehaviours::vacuouslyTrue, TestBehaviours::anyoneInSphere, TestBehaviours::nooneInSphere});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::nooneInSphere, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInSphere, present(), __LINE__)) return 1;

	setupBehaviourStore(
			_behaviourStore,
			{TestBehaviours::vacuouslyTrue,
			 TestBehaviours::anyoneInSphere,
			 TestBehaviours::nooneInSphere,
			 TestBehaviours::anyoneInRoomMulti});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::nooneInSphere, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInRoomMulti, present(), __LINE__))
		return 1;

	setupBehaviourStore(
			_behaviourStore,
			{TestBehaviours::vacuouslyTrue,
			 TestBehaviours::anyoneInSphere,
			 TestBehaviours::nooneInSphere,
			 TestBehaviours::anyoneInRoomMulti,
			 TestBehaviours::nooneInRoomMulti});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::nooneInRoomMulti, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInRoomMulti, present(), __LINE__))
		return 1;

	setupBehaviourStore(
			_behaviourStore,
			{TestBehaviours::vacuouslyTrue,
			 TestBehaviours::anyoneInSphere,
			 TestBehaviours::nooneInSphere,
			 TestBehaviours::anyoneInRoomMulti,
			 TestBehaviours::nooneInRoomMulti,
			 TestBehaviours::anyoneInRoom});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::nooneInRoomMulti, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInRoom, present(), __LINE__)) return 1;

	setupBehaviourStore(
			_behaviourStore,
			{TestBehaviours::vacuouslyTrue,
			 TestBehaviours::anyoneInSphere,
			 TestBehaviours::nooneInSphere,
			 TestBehaviours::anyoneInRoom});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::nooneInSphere, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInRoom, present(), __LINE__)) return 1;

	setupBehaviourStore(
			_behaviourStore,
			{TestBehaviours::vacuouslyTrue,
			 TestBehaviours::anyoneInSphere,
			 TestBehaviours::nooneInSphere,
			 TestBehaviours::anyoneInRoomMulti,
			 TestBehaviours::nooneInRoomMulti,
			 TestBehaviours::anyoneInRoom,
			 TestBehaviours::nooneInRoom});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::nooneInRoom, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInRoom, present(), __LINE__)) return 1;

	setupBehaviourStore(
			_behaviourStore,
			{TestBehaviours::vacuouslyTrue,
			 TestBehaviours::anyoneInSphere,
			 TestBehaviours::nooneInSphere,
			 TestBehaviours::anyoneInRoom,
			 TestBehaviours::nooneInRoom});
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::nooneInRoom, absent(), __LINE__)) return 1;
	if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInRoom, present(), __LINE__)) return 1;
}
