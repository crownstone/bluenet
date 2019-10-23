/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <util/cs_Math.h>
#include <util/cs_WireFormat.h>

#include <array>
#include <stdint.h>

/**
 * Objects of this type represent a time of day. They are stored as time offset relative
 * to one of several key events like midnight and sunrise.
 */
class TimeOfDay {
    public:
    enum class BaseTime { Midnight = 1, Sundown = 2, Sunrise = 3};
    
    private:

    BaseTime base;
    int32_t sec_since_base;

    // ensure that sec_since_base is positive when base is equal to BaseTime::Midnight. 
    void wrap(){ 
        switch(base){
            case BaseTime::Midnight:
                sec_since_base = CsMath::mod(sec_since_base, 24*60*60);
                break;
            default:
                break;
        }
    }

    // currently hardcoded sunrise/down times.
    int32_t baseTimeSinceMidnight(BaseTime b){
        switch(b){
            case BaseTime::Midnight: return 0;
            case BaseTime::Sunrise: return 60*60*7;
            case BaseTime::Sundown: return 60*60*21;
        }

        return 0;
    }

    public:

    // ===================== Constructors =====================

    TimeOfDay(BaseTime basetime, int32_t secondsSinceBase) : 
            base(basetime), sec_since_base(secondsSinceBase) { 
        wrap(); 
    }
    
    TimeOfDay(uint32_t seconds_since_midnight) : TimeOfDay(BaseTime::Midnight, seconds_since_midnight) {
    }

    // H:M:S constructor, creates a Midnight based TimeOfDay.
    TimeOfDay(uint32_t h, uint32_t m, uint32_t s) : 
        TimeOfDay(BaseTime::Midnight, s + 60*m + 60*60*h ) { 
    }

    TimeOfDay(std::array<uint8_t,5> rawData) : TimeOfDay(
        static_cast<BaseTime>(rawData[0]), 
        WireFormat::deserialize<int32_t>(rawData.data() + 1, 4)) {

    }

    static TimeOfDay Midnight(){ return TimeOfDay(BaseTime::Midnight,0); }
    static TimeOfDay Sunrise(){ return TimeOfDay(BaseTime::Sunrise,0); }
    static TimeOfDay Sundown(){ return TimeOfDay(BaseTime::Sundown,0); }


    // ===================== Conversions =====================

    // converts to a different base time in order to compare.
    // note that this conversion is in principle dependent on the season
    // as sunrise/down are so too.
    TimeOfDay convert(BaseTime newBase){
        if (base == newBase){
            return *this;
        }

        if (base == BaseTime::Midnight){
            return TimeOfDay(newBase, sec_since_base - baseTimeSinceMidnight(newBase));
        }

        return convert(BaseTime::Midnight).convert(newBase);
    }

    uint8_t h() {return convert(BaseTime::Midnight).sec_since_base / (60*60);}
    uint8_t m() {return (convert(BaseTime::Midnight).sec_since_base % (60*60)) / 60;}
    uint8_t s() {return convert(BaseTime::Midnight).sec_since_base % 60;}

    /**
     * Implicit cast operators
     */
    operator uint32_t(){ return convert(BaseTime::Midnight).sec_since_base; }   

};
