/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 12, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


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

#include <utils/cs_iostream.h>
#include <iostream>
#include <drivers/cs_PWM.h>

#include <logging/cs_Logger.h>


int main() {
    SwitchAggregator _switchAggregator;
    BehaviourStore _behaviourStore;
    BehaviourHandler _behaviourHandler;
    PresenceHandler _presenceHandler;
    SystemTime _systemTime;
    EventDispatcher &_eventDispatcher = EventDispatcher::getInstance();

    _systemTime.init();
    TestAccess<SystemTime>::fastForwardS(120);
    TestAccess<SystemTime>::setTime(Time(DayOfWeek::Tuesday, 12,30));
    SystemTime::setSunTimes(sun_time_t{});

    boards_config_t board;
    init(&board);
    asHostFullyFeatured(&board);

    TestAccess<BehaviourHandler>::setup(_behaviourHandler, &_presenceHandler, &_behaviourStore);
    _switchAggregator.init(board);

	TestAccess<SwitchBehaviour> switchBehaviourFactory;
    switchBehaviourFactory.from = TimeOfDay(9,0,0);
	switchBehaviourFactory.until = TimeOfDay(19,0,0);
	switchBehaviourFactory.intensity = 90;

	switchBehaviourFactory.presencecondition.predicate._condition = PresencePredicate::Condition::VacuouslyTrue;
	SwitchBehaviour* noPresenceRequired = switchBehaviourFactory.getNew();

	switchBehaviourFactory.presencecondition.predicate._condition = PresencePredicate::Condition::AnyoneInSphere;
	switchBehaviourFactory.presencecondition.predicate._presence._bitmask = ~0b00; // all ones
	SwitchBehaviour* requiresAnyoneInSphere = switchBehaviourFactory.getNew();

	switchBehaviourFactory.presencecondition.predicate._condition = PresencePredicate::Condition::NooneInSphere;
	switchBehaviourFactory.presencecondition.predicate._presence._bitmask = ~0b00; // all ones
	SwitchBehaviour* requiresNooneInSphere = switchBehaviourFactory.getNew();

	switchBehaviourFactory.presencecondition.predicate._condition = PresencePredicate::Condition::AnyoneInSelectedRooms;
	switchBehaviourFactory.presencecondition.predicate._presence._bitmask = 0b01; // only in room 0.
	SwitchBehaviour* requiresAnyoneInRoom = switchBehaviourFactory.getNew();


    // base case
	_behaviourStore.addBehaviour(noPresenceRequired);
	TestAccess<SwitchAggregator> testSwitchAggregator(_switchAggregator);


    // add behaviour which requires presence
	// TODO
	return 1;

    // add behaviour which requires absence

    // add behaviour which requires presence (shouldn't release override?)

	std::cout << "end of test" << std::endl;
    return 0;
}
