/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 6, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once


#include <time/cs_TimeOfDay.h>
#include <time/cs_DayOfWeek.h>

/**
 * Class to derrive behaviours from, centralizing common variables
 * such as from and until times.
 */
class Behaviour {
    protected:
    uint8_t activeIntensity = 0;
    DayOfWeekBitMask activeDays;
    TimeOfDay behaviourAppliesFrom = TimeOfDay::Midnight();
    TimeOfDay behaviourAppliesUntil = TimeOfDay::Midnight();

    public:
    enum class Type : uint8_t {Switch = 0, Twilight = 1, Extended = 2, Undefined = 0xff};
    typedef std::array<uint8_t, 1+13> SerializedDataType;

    Behaviour(
      uint8_t intensity,
      DayOfWeekBitMask activedaysofweek,
      TimeOfDay from, 
      TimeOfDay until
      );

    Behaviour(SerializedDataType arr);
    SerializedDataType serialize() const;

    // implementations of behaviours have a class specific identifier
    // which is defined by overriding this method.
    virtual Type getType() const { return Type::Undefined; }

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
};