/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <test/cs_TestAccess.h>

template<>
class TestAccess<SystemTime> {
public:
    static void tick(void*) { SystemTime::tick(nullptr); }

    static void fastForwardS(int seconds) {
        for(auto i{0}; i < seconds; i++) {
            RTC::offsetMs(1000);
            tick(nullptr);
            tick(nullptr);
            std::cout << "." << std::flush;
        }
    }

    static void setTime(Time t) {
        SystemTime::setTime(t.timestamp(), false, false);
    }
};