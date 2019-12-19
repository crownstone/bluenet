/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <behaviour/cs_Behaviour.h>

#include <presence/cs_PresenceCondition.h>

#include <util/cs_WireFormat.h>

#include <optional>
#include <stdint.h>

/**
 * Object that defines when a state transition should occur.
 * 
 * It abstracts predicates such as:
 * "fade to 100% in 10 minutes, starting 30 minutes before sunrise, if anyone is in this room"
 */
class SwitchBehaviour : public Behaviour{
    public:
    typedef std::array<uint8_t, 
      WireFormat::size<Behaviour>() + 
      WireFormat::size<PresenceCondition>()> SerializedDataType;

    virtual ~SwitchBehaviour() = default;
    
    SwitchBehaviour(
      uint8_t intensity,
      uint8_t profileid,
      DayOfWeekBitMask activedaysofweek,
      TimeOfDay from, 
      TimeOfDay until, 
      PresenceCondition presencecondition
      );

    SwitchBehaviour(SerializedDataType arr);
    SerializedDataType serialize() const;

    virtual uint8_t* serialize(uint8_t* outbuff, size_t max_size) const override;
    virtual size_t serializedSize() const override;

    virtual Type getType() const override { return Type::Switch; }

    void print() const;

    // =========== Semantics ===========

    virtual bool requiresPresence() override;
    virtual bool requiresAbsence() override;

    /**
     * Does the behaviour apply to the current situation?
     * If from() == until() the behaviour isValid all day.
     **/
    bool isValid(TimeOfDay currenttime, PresenceStateDescription currentpresence);

    bool isValid(TimeOfDay currenttime);

    // Presence description is cached in order to prevent
    // that the behaviour flickers when a user is on the border of two rooms.
    // (not there is a timeout in the presencehandler to check if the user hasn't disappeared,
    // but it tries to describe the location as accurately as possible. Thus, when a user is
    // detected in another room, the presence is immediately updated.)
    bool isValid(PresenceStateDescription currentpresence); // cached version
    
    private:

    bool _isValid(PresenceStateDescription currentpresence);  // uncached version


    // serialized fields (settings)
    PresenceCondition presenceCondition;

    // unserialized fields (runtime values)
    std::optional<uint32_t> prevIsValidTimeStamp = {}; // when was the last call to _isValid that returned true?

    // constants
    // after this amount of seconds an invalid presence condition will result in
    // the behaviour being invalidated.
    static constexpr uint32_t PresenceIsValidTimeOut_s = 5*60;
};
