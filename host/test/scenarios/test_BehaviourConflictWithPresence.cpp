/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_BehaviourStore.h>
#include <presence/cs_PresenceHandler.h>
#include <switch/cs_SwitchAggregator.h>
#include <events/cs_EventDispatcher.h>
#include <presence/cs_PresenceCondition.h>
#include <utils/date.h>

#include <testaccess/cs_SwitchBehaviour.h>


int main() {

    SwitchAggregator _switchAggregator;
    BehaviourStore _behaviourStore;
    BehaviourHandler _behaviourHandler;
    PresenceHandler _presenceHandler;
    SystemTime _systemTime;
    EventDispatcher& _eventDispatcher = EventDispatcher::getInstance();

    TestAccess<SwitchBehaviour> t;
    auto s0 = new SwitchBehaviour(t.get());
    std::cout << *s0 << std::endl;

    t.intensity = 90;
    auto s1 = new SwitchBehaviour(t.get());
    std::cout << *s1 << std::endl;
    _behaviourStore.addBehaviour(s0);
    _behaviourStore.addBehaviour(s1);

    return 0;
}