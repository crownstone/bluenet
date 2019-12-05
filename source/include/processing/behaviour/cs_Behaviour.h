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

#include <optional>
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
    bool isValid(TimeOfDay currenttime, PresenceStateDescription currentpresence);

    private:
    bool isValid(TimeOfDay currenttime);

    // Presence description is cached in order to prevent
    // that the behaviour flickers when a user is on the border of two rooms.
    // (not there is a timeout in the presencehandler to check if the user hasn't disappeared,
    // but it tries to describe the location as accurately as possible. Thus, when a user is
    // detected in another room, the presence is immediately updated.)
    bool isValid(PresenceStateDescription currentpresence); // cached version
    bool _isValid(PresenceStateDescription currentpresence);  // uncached version


    // serialized fields (settings)
    uint8_t activeIntensity = 0;
    DayOfWeekBitMask activeDays;
    TimeOfDay behaviourAppliesFrom = TimeOfDay::Midnight();
    TimeOfDay behaviourAppliesUntil = TimeOfDay::Midnight();
    PresenceCondition presenceCondition;

    // unserialized fields (runtime values)
    std::optional<uint32_t> prevIsValidTimeStamp = {}; // when was the last call to _isValid that returned true?

    // constants
    // after this amount of seconds an invalid presence condition will result in
    // the behaviour being invalidated.
    static constexpr uint32_t PresenceIsValidTimeOut_s = 5*60;
};