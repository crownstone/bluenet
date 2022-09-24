/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 12, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_BehaviourStore.h>
#include <events/cs_EventDispatcher.h>
#include <logging/cs_Logger.h>
#include <presence/cs_PresenceCondition.h>
#include <presence/cs_PresenceHandler.h>
#include <switch/cs_SwitchAggregator.h>
#include <testaccess/cs_PresenceDescription.h>
#include <testaccess/cs_SystemTime.h>
#include <utils/cs_iostream.h>
#include <utils/date.h>

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>
#include <thread>

using namespace date;
auto now() {
	return std::chrono::high_resolution_clock::now();
}

int main() {
	LOGd("hello world");

	SwitchAggregator _switchAggregator;
	BehaviourStore _behaviourStore;
	BehaviourHandler _behaviourHandler;
	PresenceHandler _presenceHandler;
	SystemTime _systemTime;
	EventDispatcher& _eventDispatcher = EventDispatcher::getInstance();
	std::printf("hi");

	_systemTime.init();
	SystemTime::setTime(1661966240, true, false);
	TestAccess<SystemTime>::setTime(Time(DayOfWeek::Tuesday, 15, 30));
	SystemTime::setSunTimes(sun_time_t{});

	TestAccess<SystemTime>::fastForwardS(60);
	std::cout << "uptime: " << SystemTime::up() << " now: " << SystemTime::now() << std::endl;

	auto predicate   = PresencePredicate(PresencePredicate::Condition::VacuouslyTrue, PresenceStateDescription());
	uint32_t timeOut = 12345;
	PresenceCondition presenceCondition(predicate, timeOut);

	auto sBehaviour =
			new SwitchBehaviour(100, 0xff, 0b11111110, TimeOfDay::Sunrise(), TimeOfDay::Sunset(), presenceCondition);
	_behaviourStore.addBehaviour(sBehaviour);

	Behaviour* bP = _behaviourStore.getBehaviour(0);
	if (bP == nullptr) {
		return 1;  // fail
	}

	std::cout << "from: " << bP->from() << " until: " << bP->until() << std::endl;

	PresenceHandler::ProfileLocation profLoc{.profile = 1, .location = 2};
	_presenceHandler.registerPresence(profLoc);
	auto desc         = _presenceHandler.getCurrentPresenceDescription();

	auto presenceDesc = PresenceStateDescription(0b111);
	std::cout << presenceDesc << " " << desc << std::endl;
	return 0;
}
