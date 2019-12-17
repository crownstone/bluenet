/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 6, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <behaviour/cs_Behaviour.h>

#include <time/cs_TimeOfDay.h>
#include <time/cs_DayOfWeek.h>
#include <presence/cs_PresenceCondition.h>

#include <optional>
#include <stdint.h>

class TwilightBehaviour : public Behaviour {
    public:
    typedef std::array<uint8_t, 1+13> SerializedDataFormat;

    enum class Type : uint8_t {Switch = 0, Twilight = 1, Extended = 2};

    
    TwilightBehaviour(
      uint8_t intensity,
      DayOfWeekBitMask activedaysofweek,
      TimeOfDay from, 
      TimeOfDay until
      );

    TwilightBehaviour(SerializedDataFormat arr);
    SerializedDataFormat serialize() const;

    void print() const;

    virtual Type getType() const override { return Type::Twilight; }

    // =========== Semantics ===========

    /**
     * Does the behaviour apply to the current situation?
     * If from() == until() the behaviour isValid all day.
     **/
    bool isValid(TimeOfDay currenttime);
};
