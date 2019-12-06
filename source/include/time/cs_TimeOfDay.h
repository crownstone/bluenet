/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 24, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once


#include <util/cs_Math.h>

#include <array>
#include <stdint.h>

/**
 * Objects of this type represent a time of day. They are stored as time offset relative
 * to one of several key events like midnight and sunrise.
 */
class TimeOfDay {
    public:
    enum class BaseTime : uint8_t { Midnight = 0, Sunrise = 1, Sunset = 2};
    typedef std::array<uint8_t,5> SerializedDataType;
    
    private:

    BaseTime base;
    int32_t sec_since_base;

    // ensures that sec_since_base is positive when base is equal to BaseTime::Midnight
    // otherwise does nothing 
    void wrap();

    // currently hardcoded sunrise/down times.
    int32_t baseTimeSinceMidnight(BaseTime b);

    public:

    // ===================== Constructors =====================

    TimeOfDay(BaseTime basetime, int32_t secondsSinceBase);
    
    TimeOfDay(SerializedDataType rawData);

    // H:M:S constructor, creates a Midnight based TimeOfDay.
    TimeOfDay(uint32_t h, uint32_t m, uint32_t s);
    TimeOfDay(uint32_t seconds_since_midnight);


    // TimeOfDay describing the given 'event'
    static TimeOfDay Midnight();
    static TimeOfDay Sunrise();
    static TimeOfDay Sunset();

    // ===================== Conversions =====================

    SerializedDataType serialize() const;

    // converts to a different base time in order to compare.
    // note that this conversion is in principle dependent on the season
    // as sunrise/down are so too.
    TimeOfDay convert(BaseTime newBase);

    uint8_t h();
    uint8_t m();
    uint8_t s();

    /**
     * Implicit cast operators, returns seconds since midnight
     * (enables built in relational operators).
     */
    operator uint32_t();
};
