/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <test/cs_TestAccess.h>
#include <testaccess/cs_PresenceCondition.h>

#include <behaviour/cs_SwitchBehaviour.h>
#include <utils/cs_iostream.h>
#include <bitset>
#include <iomanip>
#include <iostream>

template <>
class TestAccess<SwitchBehaviour> {
   public:
    uint8_t intensity;
    uint8_t profileId;
    DayOfWeekBitMask activedaysofweek;
    TimeOfDay from;
    TimeOfDay until;
    TestAccess<PresenceCondition> presencecondition;

    TestAccess() : from(TimeOfDay::Sunrise()), until(TimeOfDay::Sunset()) { reset(); }

    void reset() {
        intensity = 100;
        profileId = 0;
        activedaysofweek = 0b01111111;
        from = TimeOfDay::Sunrise();
        until = TimeOfDay::Sunset();
        presencecondition.reset();
    }

    SwitchBehaviour get() {
        return {intensity, profileId, activedaysofweek, from, until, presencecondition.get()};
    }

    static std::ostream& toStream(std::ostream& out, SwitchBehaviour& s) {
        return out << "{"
                   << "from: " << s.from() << ", "
                   << "until: " << s.until() << ", "
                   << "value: " << std::setw(3) << std::setfill(' ') << +s.value() << ", "
                   << "profileId:" << +s.profileId << ", "
                   << "activeDays: " << std::bitset<8>(s.activeDays) << ", "
                   << "presenceCondition: " << s.presenceCondition << "}"
                   << "[gracePeriodForPresenceIsActive(): " << s.gracePeriodForPresenceIsActive()
                   << "]";
    }
};

/**
 * Allows streaming SwitchBehaviour objects to std::cout and other streams.
 * global definition forwards to TestAccess to elevate access rights for inspection.
 */
std::ostream& operator<<(std::ostream& out, SwitchBehaviour& s) {
    return TestAccess<SwitchBehaviour>::toStream(out, s);
}
