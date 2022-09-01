/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 12, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <behaviour/cs_BehaviourStore.h>
#include <presence/cs_PresenceHandler.h>
#include <switch/cs_SwitchAggregator.h>
#include <events/cs_EventDispatcher.h>
#include <presence/cs_PresenceCondition.h>

#include <iostream>
#include <string>
#include <sstream>
#include <ostream>

std::ostream& operator<<(std::ostream& out, DayOfWeek t) {
    switch (t) {
    	case DayOfWeek::Sunday: return out << "Sunday";
    	case DayOfWeek::Monday: return out << "Monday";
    	case DayOfWeek::Tuesday: return out << "Tuesday";
    	case DayOfWeek::Wednesday: return out << "Wednesday";
    	case DayOfWeek::Thursday: return out << "Thursday";
    	case DayOfWeek::Friday: return out << "Friday";
    	case DayOfWeek::Saturday: return out << "Saturday";
    }
    return out << "UnknownDay";
}

std::ostream & operator<< (std::ostream &out, TimeOfDay t){
	return out << +t.h() << ":" << +t.m() << ":" << +t.s();
}

std::ostream & operator<< (std::ostream &out, Time t){
	return out << t.dayOfWeek() << " " << t.timeOfDay();
}

std::ostream & operator<< (std::ostream &out, PresenceStateDescription p){
	int i = 0;
	out << "rooms: {";
	for (auto bitmask = p.getBitmask(); bitmask != 0; bitmask >>= 1) {
		out << (i?", ":"") << i;
		i++;
	}
	out << "}";
	return out;
}

int main() {
    SwitchAggregator _switchAggregator;
    BehaviourStore _behaviourStore;
    BehaviourHandler _behaviourHandler;
    PresenceHandler _presenceHandler;

    EventDispatcher& _eventDispatcher = EventDispatcher::getInstance();
    SystemTime::setTime(1661966240, true, false);

    SystemTime::setSunTimes(sun_time_t{});

    std::cout << "uptime: " << SystemTime::now() << std::endl;

    auto predicate =
        PresencePredicate(PresencePredicate::Condition::VacuouslyTrue, PresenceStateDescription());
    uint32_t timeOut = 12345;
    PresenceCondition presenceCondition(predicate, timeOut);

    auto sBehaviour = new SwitchBehaviour(
        100, 0xff, 0b11111110, TimeOfDay::Sunrise(), TimeOfDay::Sunset(), presenceCondition);
    _behaviourStore.addBehaviour(sBehaviour);
    //	CommandSetTime c(10,0);
    //	c.dispatch();

    Behaviour* bP = _behaviourStore.getBehaviour(0);
    if(bP == nullptr) {
    	return 1; // fail
    }

    std::cout << "from: " << bP->from() << " until: " << bP->until() << std::endl;

    PresenceHandler::ProfileLocation profLoc{.profile = 1, .location = 2};
    _presenceHandler.registerPresence(profLoc);
    auto desc = _presenceHandler.getCurrentPresenceDescription();

    auto presenceDesc = PresenceStateDescription(0b111);
	std::cout << presenceDesc << std::endl;
    return 0;
}
