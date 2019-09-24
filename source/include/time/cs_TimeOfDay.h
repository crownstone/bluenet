/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <stdint.h>

class TimeOfDay{
    private:
    uint32_t sec_since_midnight;

    public:
    TimeOfDay(uint32_t secondsSinceMidnight) : sec_since_midnight(secondsSinceMidnight % (24*60*60)){}

    uint8_t h() {return sec_since_midnight / (60*60);}
    uint8_t m() {return (sec_since_midnight % (60*60)) / 60;}
    uint8_t s() {return sec_since_midnight % 60;}

    /**
     * Implicit cast operators
     */
    operator uint32_t(){ return sec_since_midnight; }

};