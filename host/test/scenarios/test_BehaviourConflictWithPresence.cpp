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


int main() {

    SwitchAggregator _switchAggregator;
    BehaviourStore _behaviourStore;
    BehaviourHandler _behaviourHandler;
    PresenceHandler _presenceHandler;
    SystemTime _systemTime;
    EventDispatcher& _eventDispatcher = EventDispatcher::getInstance();

    TestAccess<SwitchBehaviour> t;
    t.presencecondition.predicate._condition = PresencePredicate::Condition::AnyoneInSelectedRooms;
    auto s0 = new SwitchBehaviour(t.get());
    std::cout << *s0 << std::endl;

    t.intensity = 90;
    t.presencecondition.predicate._condition = PresencePredicate::Condition::VacuouslyTrue;
    auto s1 = new SwitchBehaviour(t.get());
    std::cout << *s1 << std::endl;
    _behaviourStore.addBehaviour(s0);
    _behaviourStore.addBehaviour(s1);

    TestAccess<PresenceStateDescription> t_presenceDesc;
    t_presenceDesc._bitmask |= 0b001;

    // Time(DayOfWeek day, uint8_t hours, uint8_t minutes, uint8_t seconds=0)
    Time testTime(DayOfWeek::Tuesday, 12, 30);
    // set time, check resolution
    TestAccess<BehaviourHandler>::setup(_behaviourHandler, &_presenceHandler, &_behaviourStore);
    SwitchBehaviour* resolved = TestAccess<BehaviourHandler>::resolveSwitchBehaviour(_behaviourHandler, testTime, t_presenceDesc.get());

    if(resolved)  {
        std::cout << "resolved! " << *resolved << std::endl;
    } else {
        std::cout << "nope :(" << std::endl;
    }

    return 0;
}