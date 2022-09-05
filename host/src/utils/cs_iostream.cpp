/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <utils/cs_iostream.h>

#include <cstdio>
#include <iomanip>
#include <string>
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
    return out     << std::setw(2) << std::setfill('0') << +t.h()
                   << ":" << std::setw(2) << std::setfill('0') << +t.m()
                   << ":" << std::setw(2) << std::setfill('0') << +t.s();
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
