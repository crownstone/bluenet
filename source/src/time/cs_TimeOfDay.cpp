/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <storage/cs_State.h>
#include <common/cs_Types.h>
#include <time/cs_TimeOfDay.h>
#include <util/cs_WireFormat.h>

#include <drivers/cs_Serial.h>

void TimeOfDay::wrap(){ 
    switch(base){
        case BaseTime::Midnight:
            sec_since_base = CsMath::mod(sec_since_base, 24*60*60);
            break;
        default:
            break;
    }
}

int32_t TimeOfDay::baseTimeSinceMidnight(BaseTime b){
    switch(b){
        case BaseTime::Midnight: return 0;
        case BaseTime::Sunrise: {
            TYPIFY(STATE_SUN_TIME) suntime;
            State::getInstance().get(CS_TYPE::STATE_SUN_TIME, &suntime, sizeof(suntime));
            return suntime.sunrise;
        }
        case BaseTime::Sunset: {
            TYPIFY(STATE_SUN_TIME) suntime;
            State::getInstance().get(CS_TYPE::STATE_SUN_TIME, &suntime, sizeof(suntime));
            return suntime.sunset;
        }
    }

    return 0;
}

// ===================== Constructors =====================

TimeOfDay::TimeOfDay(BaseTime basetime, int32_t secondsSinceBase) : 
        base(basetime), sec_since_base(secondsSinceBase) { 
    wrap(); 
}

TimeOfDay::TimeOfDay(uint32_t seconds_since_midnight) : TimeOfDay(BaseTime::Midnight, seconds_since_midnight) {
}

TimeOfDay::TimeOfDay(uint32_t h, uint32_t m, uint32_t s) : 
    TimeOfDay(BaseTime::Midnight, s + 60*m + 60*60*h ) { 
}

TimeOfDay::TimeOfDay(SerializedDataType rawData) : TimeOfDay(
    static_cast<BaseTime>(rawData[0]), 
    WireFormat::deserialize<int32_t>(rawData.data() + 1, 4)) {
    bool basevalue_isok = false;
    switch(base){
        case BaseTime::Midnight:
        case BaseTime::Sunrise:
        case BaseTime::Sunset:
            basevalue_isok = true;
            break;
    }

    if(!basevalue_isok){
        base = BaseTime::Midnight;
    }
}

TimeOfDay TimeOfDay::Midnight(){ 
    return TimeOfDay(BaseTime::Midnight,0); 
}

TimeOfDay TimeOfDay::Sunrise(){ 
    return TimeOfDay(BaseTime::Sunrise,0); 
}

TimeOfDay TimeOfDay::Sunset(){ 
    return TimeOfDay(BaseTime::Sunset,0); 
}


// ===================== Conversions =====================

TimeOfDay::SerializedDataType TimeOfDay::serialize() const {
    SerializedDataType result;
    std::copy_n(std::begin(WireFormat::serialize(static_cast<uint8_t>(base))),  1, std::begin(result) + 0);
    std::copy_n(std::begin(WireFormat::serialize(sec_since_base)),              4, std::begin(result) + 1);
    return result;
}

TimeOfDay TimeOfDay::convert(BaseTime newBase){
    return TimeOfDay(newBase, sec_since_base + baseTimeSinceMidnight(base) - baseTimeSinceMidnight(newBase));
}

uint8_t TimeOfDay::h() {
    return convert(BaseTime::Midnight).sec_since_base / (60*60);
}

uint8_t TimeOfDay::m() {
    return (convert(BaseTime::Midnight).sec_since_base % (60*60)) / 60;
}

uint8_t TimeOfDay::s() {
    return convert(BaseTime::Midnight).sec_since_base % 60;
}

TimeOfDay::operator uint32_t(){ 
     return convert(BaseTime::Midnight).sec_since_base; 
}   

