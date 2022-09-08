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
#include <testaccess/cs_SystemTime.h>

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

bool checkCase(BehaviourHandler& _behaviourHandler, BehaviourStore& _behaviourStore, int expectedBehaviourIndex, bool isPresent, int line) {
    Time testTime(DayOfWeek::Tuesday, 12, 0);
    auto presence = isPresent? present() : absent();
    SwitchBehaviour* expected = dynamic_cast<SwitchBehaviour*>(_behaviourStore.getBehaviour(expectedBehaviourIndex));
    SwitchBehaviour* resolved = TestAccess<BehaviourHandler>::resolveSwitchBehaviour(_behaviourHandler, testTime, presence);

    if (resolved == expected && expected != nullptr){
        return true;
    } else {
        std::cout << "FAILED test at line: " << line << std::endl;
        if(expected != nullptr) {
            std::cout << "expected:" << *expected << std::endl;
        } else {
            std::cout << "expected:" << "nullptr" << "(index " << expectedBehaviourIndex << ")" << std::endl;
        }
        if(resolved != nullptr) {
            std::cout << "resolved:" << *resolved << std::endl;
        } else {
            std::cout << "resolved:" << "nullptr" << std::endl;
        }
        std::cout << "time: " << testTime << std::endl;
        std::cout << "presence" << presence << std::endl;

        return false;
    }
}

/**
 * Clears the store from any previous behaviours. Then,
 * adds switchbehaviours with the given conditions into the store.
 *
 * behaviour index [0] will contain a switch behaviour constructed through getBehaviourNarrowTimesWithTrivialPresence().
 * conditions[i] will have behaviour index [i+1]
 */
//template<class T>
//void setupBehaviourStore(BehaviourStore& _behaviourStore, std::vector<PresencePredicate::Condition> conditions) {
//    TestAccess<BehaviourStore>::clearActiveBehavioursArray(_behaviourStore);
//
//    SwitchBehaviour* verySpecific = getBehaviourNarrowTimesWithTrivialPresence();
//    _behaviourStore.addBehaviour(verySpecific);
//
//    for(auto i{0}; i < conditions.length(); i++) {
//        _behaviourStore.replaceBehaviour(i, getBehaviourPresent(condition));
//    }
//}

struct TestBehaviours {
    static constexpr int verySpecific = 0;
    static constexpr int vacuouslyTrue = 1;
    static constexpr int anyoneInSphere = 2;
    static constexpr int nooneInSphere = 3;
    static constexpr int anyoneInRoom = 4;
    static constexpr int nooneInRoom = 5;

    static SwitchBehaviour* makeTestSwitchBehaviour(int index) {
        switch(index) {
            case verySpecific: return getBehaviourNarrowTimesWithTrivialPresence();
            case vacuouslyTrue: return getBehaviourPresent(PresencePredicate::Condition::VacuouslyTrue);
            case anyoneInSphere: return getBehaviourPresent(PresencePredicate::Condition::AnyoneInSphere);
            case nooneInSphere: return getBehaviourPresent(PresencePredicate::Condition::NooneInSphere);
            case anyoneInRoom: return getBehaviourPresent(PresencePredicate::Condition::AnyoneInSelectedRooms);
            case nooneInRoom: return getBehaviourPresent(PresencePredicate::Condition::NooneInSelectedRooms);
            default: return nullptr;
        }
    }
};

void setupBehaviourStore(BehaviourStore& store, std::vector<int> indices) {
    TestAccess<BehaviourStore>::clearActiveBehavioursArray(store);
    store.replaceBehaviour(TestBehaviours::verySpecific,
                           TestBehaviours::makeTestSwitchBehaviour(TestBehaviours::verySpecific));
    for(int index : indices) {
        store.replaceBehaviour(index,
                               TestBehaviours::makeTestSwitchBehaviour(index));
    }
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
    EventDispatcher &_eventDispatcher = EventDispatcher::getInstance();


    TestAccess<BehaviourHandler>::setup(_behaviourHandler, &_presenceHandler, &_behaviourStore);
    TestAccess<SystemTime>::fastForwardS(120);

    // common variables.
    Time testTimeActive(DayOfWeek::Tuesday, 12, 30);
    Time testTimeActiveSpecific(DayOfWeek::Tuesday, 12, 0);
    SwitchBehaviour *resolved = nullptr;

    // add behaviours in increasing priority, and check if the resolution picks the correct one.

    // any presence based behaviour must win from this specific behaviour when it is valid
    setupBehaviourStore(_behaviourStore, {
        // no presence behaviours added
    });
    if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, false, __LINE__)) return 1;
    if (!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, true, __LINE__)) return 1;

    // add unspecific, non-presence. should still resolve to the verySpecific one.
    setupBehaviourStore(_behaviourStore, {
        TestBehaviours::vacuouslyTrue
    });
    if(!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, false, __LINE__)) return 1;
    if(!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, true, __LINE__)) return 1;

    // should only resolve to the verySpecific one when not in sphere.
    setupBehaviourStore(_behaviourStore, {
        TestBehaviours::vacuouslyTrue,
        TestBehaviours::anyoneInSphere
    });
    if(!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::verySpecific, false, __LINE__)) return 1;
    if(!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInSphere, true, __LINE__)) return 1;

    // adding the opposite presence based behaviour, overriding verySpecific.
    setupBehaviourStore(_behaviourStore, {
            TestBehaviours::vacuouslyTrue,
            TestBehaviours::anyoneInSphere,
            TestBehaviours::nooneInSphere
    });
    if(!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::nooneInSphere, false, __LINE__)) return 1;
    if(!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInSphere, true, __LINE__)) return 1;


    setupBehaviourStore(_behaviourStore, {
            TestBehaviours::vacuouslyTrue,
            TestBehaviours::anyoneInSphere,
            TestBehaviours::nooneInSphere,
            TestBehaviours::anyoneInRoom
    });
    if(!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::nooneInSphere, false, __LINE__)) return 1;
    if(!checkCase(_behaviourHandler, _behaviourStore, TestBehaviours::anyoneInRoom, true, __LINE__)) return 1;

    //    SwitchBehaviour* nooneInRoom = getBehaviourPresent(PresencePredicate::Condition::NooneInSelectedRooms);
//    _behaviourStore.addBehaviour(nooneInRoom);
}
