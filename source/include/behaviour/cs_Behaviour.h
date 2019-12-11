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
    public:
    enum class Type : uint8_t {Switch = 0, Twilight = 1, Extended = 2, Undefined = 0xff};
    typedef std::array<uint8_t, 1+13> SerializedDataType;
    
    protected:
    Type typ;
    uint8_t activeIntensity = 0;
    uint8_t profileId = 0;
    DayOfWeekBitMask activeDays;
    TimeOfDay behaviourAppliesFrom = TimeOfDay::Midnight();
    TimeOfDay behaviourAppliesUntil = TimeOfDay::Midnight();

    public:
    
    virtual ~Behaviour() = default; // (to prevent object slicing from leaking memory.)

    Behaviour(
      Type typ,
      uint8_t intensity,
      uint8_t profileid,
      DayOfWeekBitMask activedaysofweek,
      TimeOfDay from, 
      TimeOfDay until
      );

    Behaviour(SerializedDataType arr);
    SerializedDataType serialize() const;
    
    // return value: number of bytes written, or pointer to next empty val in array?
    // virtual size_t serialize(uint8_t* outbuff, size_t max_size);

    void print() const;

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