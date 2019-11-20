/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <time/cs_TimeOfDay.h>
#include <time/cs_DayOfWeek.h>
#include <presence/cs_PresenceCondition.h>

#include <stdint.h>

/**
 * Object that defines when a state transition should occur.
 * 
 * It abstracts predicates such as:
 * "fade to 100% in 10 minutes, starting 30 minutes before sunrise, if anyone is in this room"
 */
class Behaviour {
    public:
    typedef std::array<uint8_t, 26> SerializedDataFormat;

    enum class Type : uint8_t {Switch = 0, Twilight = 1, Extended = 2};

    
    Behaviour() = default;
    Behaviour(
      uint8_t intensity,
      DayOfWeekBitMask activedaysofweek,
      TimeOfDay from, 
      TimeOfDay until, 
      PresenceCondition presencecondition
      );

    Behaviour(SerializedDataFormat arr);
    SerializedDataFormat serialize() const;

    void print() const;

    // =========== Getters ===========

    /**
     * Returns the intended state when this behaviour is valid.
     */
    uint8_t value() const;

    /**
     * Returns from (incl.) which time on this behaviour applies.
     */
    TimeOfDay from() const;

    /**
     * Returns until (excl.) which time on this behaviour applies.
     */
    TimeOfDay until() const;


    // =========== Semantics ===========

    /**
     * Does the behaviour apply to the current situation?
     * If from() == until() the behaviour isValid all day.
     **/
    bool isValid(TimeOfDay currenttime) const;
    bool isValid(PresenceStateDescription currentpresence) const;
    bool isValid(TimeOfDay currenttime, PresenceStateDescription currentpresence) const;

    private:
    uint8_t activeIntensity = 0;
    DayOfWeekBitMask activeDays;
    TimeOfDay behaviourAppliesFrom = TimeOfDay::Midnight();
    TimeOfDay behaviourAppliesUntil = TimeOfDay::Midnight();
    PresenceCondition presenceCondition;

};