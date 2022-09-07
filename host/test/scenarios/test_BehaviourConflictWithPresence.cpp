/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <testaccess/cs_SwitchBehaviour.h>
#include <testaccess/cs_BehaviourHandler.h>

#include <behaviour/cs_BehaviourStore.h>
#include <presence/cs_PresenceHandler.h>
#include <switch/cs_SwitchAggregator.h>
#include <events/cs_EventDispatcher.h>
#include <presence/cs_PresenceCondition.h>

#include <utils/date.h>

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

SwitchBehaviour* getBehaviourPresent(){
    TestAccess<SwitchBehaviour> testAccessSwitchBehaviour;

    testAccessSwitchBehaviour.intensity = 95;
    testAccessSwitchBehaviour.presencecondition.predicate._condition = PresencePredicate::Condition::AnyoneInSelectedRooms;
    testAccessSwitchBehaviour.presencecondition.predicate._presence._bitmask = roomBitmask();
    auto switchBehaviourInRoom = new SwitchBehaviour(testAccessSwitchBehaviour.get());
    std::cout << "switchBehaviourInRoom: " << *switchBehaviourInRoom << std::endl;
    return switchBehaviourInRoom;
}

SwitchBehaviour* getBehaviourAbsent() {
    TestAccess<SwitchBehaviour> testAccessSwitchBehaviour;

    testAccessSwitchBehaviour.intensity = 90;
    testAccessSwitchBehaviour.presencecondition.predicate._condition = PresencePredicate::Condition::VacuouslyTrue;
    auto switchBehaviourPresenceIrrelevant = new SwitchBehaviour(testAccessSwitchBehaviour.get());
    std::cout << "switchBehaviourPresenceIrrelevant: " << *switchBehaviourPresenceIrrelevant << std::endl;
    return switchBehaviourPresenceIrrelevant;
}


int main() {
    SwitchAggregator _switchAggregator;
    BehaviourStore _behaviourStore;
    BehaviourHandler _behaviourHandler;
    PresenceHandler _presenceHandler;
    SystemTime _systemTime;
    EventDispatcher& _eventDispatcher = EventDispatcher::getInstance();

    TestAccess<BehaviourHandler>::setup(_behaviourHandler, &_presenceHandler, &_behaviourStore);

    SwitchBehaviour* behaviourPresent = getBehaviourPresent();
    SwitchBehaviour* behaviourAbsent = getBehaviourAbsent();
    _behaviourStore.addBehaviour(behaviourPresent);
    _behaviourStore.addBehaviour(behaviourAbsent);

    for (auto storedBehaviour : _behaviourStore.getActiveBehaviours()) {
        if(auto sBehaviour = dynamic_cast<SwitchBehaviour*>(storedBehaviour)) {
            std::cout << "stored behaviour: " << *sBehaviour
                      << " condition: " << +static_cast<uint8_t>(sBehaviour->currentPresenceCondition())
                      << std::endl;
        }
    }

    Time testTimeActive(DayOfWeek::Tuesday, 12, 30);

    SwitchBehaviour* resolved = nullptr;

    // if absent there's only one active behaviour.
    resolved = TestAccess<BehaviourHandler>::resolveSwitchBehaviour(_behaviourHandler, testTimeActive, absent());
    if(resolved != behaviourAbsent) {
        std::cout << "FAILED: should resolve to non-presence behaviour. It is the only active one" << std::endl;
        return 1;
    }

    // if present, there are two behaviours -- one of which is more time-specific, one is more presence-specific.
    resolved = TestAccess<BehaviourHandler>::resolveSwitchBehaviour(_behaviourHandler, testTimeActive, present());
    if(resolved != behaviourPresent) {
        std::cout << "FAILED: should resolve to presence behaviour. It has better presence specificity." << std::endl;
        return 1;
    }

    return 0;
}
