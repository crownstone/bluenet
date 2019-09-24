/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <time/cs_TimeOfDay.h>

#include <stdint.h>

class Time {
    private:
    uint32_t posixTimeStamp;
    
    public:
    Time(uint32_t posixTime)  : posixTimeStamp(posixTime){}

    /**
     * Implicit cast operators
     */
    operator uint32_t(){ return posixTimeStamp; }
    operator TimeOfDay(){ return posixTimeStamp % (24*60*60); }

};