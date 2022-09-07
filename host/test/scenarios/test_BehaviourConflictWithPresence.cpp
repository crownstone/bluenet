/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <testaccess/cs_PresenceDescription.h>
#include <testaccess/cs_SwitchBehaviour.h>
#include <testaccess/cs_BehaviourHandler.h>
#include <testaccess/cs_BehaviourStore.h>

#include <behaviour/cs_BehaviourStore.h>
#include <presence/cs_PresenceHandler.h>
#include <switch/cs_SwitchAggregator.h>
#include <events/cs_EventDispatcher.h>
#include <presence/cs_PresenceCondition.h>

#include <utils/date.h>
#include "../../../source/include/protocol/cs_Packets.h"

uint64_t roomBitmask() {
    return 0b001;
}

PresenceStateDescription present() {
    TestAccess<PresenceStateDescription> testAccessPresenceDesc;
    testAccessPresenceDesc._bitmask |= roomBitmask();
    return testAccessPresenceDesc.get();
}

PresenceStateDescription absent() {
    TestAccess<PresenceStateDescription> testAccessPresenceDesc;
    testAccessPresenceDesc._bitmask = 0;
    return testAccessPresenceDesc.get();
}

SwitchBehaviour* getBehaviourPresent(PresencePredicate::Condition condition){
    TestAccess<SwitchBehaviour> testAccessSwitchBehaviour;

    testAccessSwitchBehaviour.intensity = 95;
    testAccessSwitchBehaviour.presencecondition.predicate._condition = condition;
    testAccessSwitchBehaviour.presencecondition.predicate._presence._bitmask = roomBitmask();
    auto switchBehaviourInRoom = new SwitchBehaviour(testAccessSwitchBehaviour.get());
    //std::cout << "switchBehaviourInRoom: " << *switchBehaviourInRoom << std::endl;
    return switchBehaviourInRoom;
}

SwitchBehaviour* getBehaviourNarrowTimesWithTrivialPresence() {
    TestAccess<SwitchBehaviour> testAccessSwitchBehaviour;

    testAccessSwitchBehaviour.from = TimeOfDay(11,59,0);
    testAccessSwitchBehaviour.until = TimeOfDay(12,1,0);
    testAccessSwitchBehaviour.intensity = 1;
    testAccessSwitchBehaviour.presencecondition.predicate._condition = PresencePredicate::Condition::VacuouslyTrue;
    auto switchBehaviourPresenceIrrelevant = new SwitchBehaviour(testAccessSwitchBehaviour.get());
    //std::cout << "getBehaviourNarrowTimesWithTrivialPresence: " << *switchBehaviourPresenceIrrelevant << std::endl;
    return switchBehaviourPresenceIrrelevant;
}

bool checkCase(BehaviourHandler& _behaviourHandler, SwitchBehaviour* expected, bool isPresent, int line) {
    Time testTime(DayOfWeek::Tuesday, 12, 0);
    auto presence = isPresent? present() : absent();
    SwitchBehaviour* resolved = TestAccess<BehaviourHandler>::resolveSwitchBehaviour(_behaviourHandler, testTime, presence);
    if (resolved != expected) {
        std::cout << "FAILED test at line: " << line << std::endl;
        std::cout << "expected:" << *expected << std::endl;
        std::cout << "resolved:" << *resolved << std::endl;
        std::cout << "time: " << testTime << std::endl;
        std::cout << "presence" << presence << std::endl;
        return false;
    }
    return true;
}

/**
 * This test asserts correct conflict resolution when the only difference in stored behaviours
 * is the presence condition. And conflict resolution in favor of any non-trivial presence clause over
 * a behaviour with a more specific from-until interval.
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

    // common variables.
    Time testTimeActive(DayOfWeek::Tuesday, 12, 30);
    Time testTimeActiveSpecific(DayOfWeek::Tuesday, 12, 0);
    SwitchBehaviour* resolved = nullptr;

    // any presence based behaviour must win from this specific behaviour when it is valid
    SwitchBehaviour* verySpecific = getBehaviourNarrowTimesWithTrivialPresence();
    _behaviourStore.addBehaviour(verySpecific);

    if(!checkCase(_behaviourHandler, verySpecific, false, __LINE__)) return 1;
    if(!checkCase(_behaviourHandler, verySpecific, true, __LINE__)) return 1;

    // add behaviours in increasing priority, and check if the resolution picks the last one added.

    // add unspecific, non-presence. should still resolve to the verySpecific one.
    SwitchBehaviour* vacuuslyTrue = getBehaviourPresent(PresencePredicate::Condition::VacuouslyTrue);
    _behaviourStore.addBehaviour(vacuuslyTrue);
    if(!checkCase(_behaviourHandler, verySpecific, false, __LINE__)) return 1;
    if(!checkCase(_behaviourHandler, verySpecific, true, __LINE__)) return 1;

    // add unspecific, presence based. should only resolve to the verySpecific one when not in sphere.
    SwitchBehaviour* anyoneInSphere = getBehaviourPresent(PresencePredicate::Condition::AnyoneInSphere);
    _behaviourStore.addBehaviour(anyoneInSphere);
    if(!checkCase(_behaviourHandler, verySpecific, false, __LINE__)) return 1;
    if(!checkCase(_behaviourHandler, anyoneInSphere, true, __LINE__)) return 1;

    // adding the opposite presence based behaviour, overriding verySpecific.
    SwitchBehaviour* nooneInSphere = getBehaviourPresent(PresencePredicate::Condition::NooneInSphere);
    _behaviourStore.addBehaviour(nooneInSphere);
    if(!checkCase(_behaviourHandler, nooneInSphere, false, __LINE__)) return 1;
    if(!checkCase(_behaviourHandler, anyoneInSphere, true, __LINE__)) return 1;

    //
    SwitchBehaviour* anyoneInRoom = getBehaviourPresent(PresencePredicate::Condition::AnyoneInSelectedRooms);
    _behaviourStore.addBehaviour(anyoneInRoom);
    if(!checkCase(_behaviourHandler, nooneInSphere, false, __LINE__)) {
        std::cout << "store: " << _behaviourStore << std::endl;
        return 1;
    }
    if(!checkCase(_behaviourHandler, anyoneInRoom, true, __LINE__)) return 1;

    SwitchBehaviour* nooneInRoom = getBehaviourPresent(PresencePredicate::Condition::NooneInSelectedRooms);
    _behaviourStore.addBehaviour(nooneInRoom);




    return 0;
}
