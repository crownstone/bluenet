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

int main() {
	SwitchAggregator _switchAggregator;
	BehaviourStore _behaviourStore;
	BehaviourHandler _behaviourHandler;
	PresenceHandler _presenceHandler;

	EventDispatcher& _eventDispatcher = EventDispatcher::getInstance();
	SystemTime::setTime(1661966240,true,false);
	std::cout << "uptime: " << SystemTime::now() << std::endl;

	return 0;
}
