/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <time/cs_TimeOfDay.h>
#include <time/cs_DayOfWeek.h>

#include <stdint.h>

class Time {
    private:
    uint32_t posixTimeStamp;
    
    public:
    Time(uint32_t posixTime)  : posixTimeStamp(posixTime){}

    // --- Implicit cast operators ---

    operator uint32_t(){ return posixTimeStamp; }
    operator TimeOfDay(){ return TimeOfDay(TimeOfDay::BaseTime::Midnight,posixTimeStamp); }

    /**
     * See: http://stackoverflow.com/questions/36357013/day-of-week-from-seconds-since-epoch
	 * With timestamp=0 = Thursday 1-January-1970 00:00:00
     */
    operator DayOfWeek(){ return DayOfWeek(posixTimeStamp / 60*60*24 + 4 % 7);}

};
