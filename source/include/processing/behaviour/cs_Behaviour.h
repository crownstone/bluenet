/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <stdint.h>
#include <time/cs_TimeOfDay.h>

/**
 * Object that defines when a state transition should occur.
 * 
 * It abstracts predicates such as:
 * "fade to 100% in 10 minutes, starting 30 minutes before sunrise, if anyone is in this room"
 */
class Behaviour {
    // TODO(Arend, 23-09-2019): datatypes and implementation still susceptible to change.
    public:
    typedef TimeOfDay time_t; // let's say, seconds since midnight (00:00)
    typedef uint8_t presence_data_t;

    Behaviour() = default;
    Behaviour(time_t from, time_t until, 
      presence_data_t presencemask,
      uint8_t intendedStateWhenBehaviourIsValid);

    // =========== Getters ===========

    /**
     * Returns the intended state when this behaviour is valid.
     */
    uint8_t value() const;

    /**
     * Returns from (incl.) which time on this behaviour applies.
     */
    time_t from() const;

    /**
     * Returns until (excl.) which time on this behaviour applies.
     */
    time_t until() const;


    // =========== Semantics ===========

    /**
     * Does the behaviour apply to the current situation?
     * If from() == until() the behaviour isValid all day.
     **/
    bool isValid(time_t currenttime) const;
    bool isValid(presence_data_t currentpresence) const;
    bool isValid(time_t currenttime, presence_data_t currentpresence) const;

    private:
    time_t behaviourappliesfrom = 0x0000;
    time_t behaviourappliesuntil = 0x0000;
    presence_data_t requiredpresencebitmask = 0x00;

    uint8_t intendedStateWhenBehaviourIsValid = 0;
};