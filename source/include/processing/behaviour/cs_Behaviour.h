/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <stdint.h>

/**
 * Object that defines when a state transition should occur.
 * 
 * It abstracts predicates such as:
 * "fade to 100% in 10 minutes, starting 30 minutes before sunrise, if anyone is in this room"
 */
class Behaviour {
    // TODO(Arend, 23-09-2019): datatypes and implementation still susceptible to change.
    public:
    typedef uint16_t time_t;
    typedef uint8_t presence_data_t;

    Behaviour(time_t from, time_t until, 
      presence_data_t presencemask,
      bool intendedStateWhenBehaviourIsValid);

    /**
     * Returns the intended state when this behaviour is valid.
     */
    bool value() const;

    /**
     * Does the behaviour apply to the current situation?
     */
    bool isValid(time_t currenttime, presence_data_t currentpresence) const;

    private:
    bool isValid(time_t currenttime) const;
    bool isValid(presence_data_t currentpresence) const;

    time_t behaviourappliesfrom;
    time_t behaviourappliesuntil;
    presence_data_t requiredpresencebitmask;

    // true if on, false if off.
    bool intendedStateWhenBehaviourIsValid;
};