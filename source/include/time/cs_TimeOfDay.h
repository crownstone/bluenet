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
    uint32_t wrap(uint32_t t){ return t % (24*60*60); }

    public:
    TimeOfDay(uint32_t secondsSinceMidnight) : sec_since_midnight(wrap(secondsSinceMidnight)) { }
    TimeOfDay(uint32_t h, uint32_t m, uint32_t s) 
        : sec_since_midnight( wrap( s + 60*m + 60*60*h )) { 
    }

    uint8_t h() {return sec_since_midnight / (60*60);}
    uint8_t m() {return (sec_since_midnight % (60*60)) / 60;}
    uint8_t s() {return sec_since_midnight % 60;}

    /**
     * Implicit cast operators
     */
    operator uint32_t(){ return sec_since_midnight; }

};