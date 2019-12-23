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
#include <util/cs_WireFormat.h>

#include <optional>
#include <stdint.h>

class TwilightBehaviour : public Behaviour {
    public:
    typedef std::array<uint8_t, WireFormat::size<Behaviour>()> SerializedDataType;

    virtual ~TwilightBehaviour() = default;
    TwilightBehaviour(
      uint8_t intensity,
      uint8_t profileid,
      DayOfWeekBitMask activedaysofweek,
      TimeOfDay from, 
      TimeOfDay until
      );

    TwilightBehaviour(SerializedDataType arr);
    SerializedDataType serialize() const;

    virtual uint8_t* serialize(uint8_t* outbuff, size_t max_size) const override;
    virtual size_t serializedSize() const override;

    virtual void print() override;

    virtual Type getType() const override { return Type::Twilight; }

};
