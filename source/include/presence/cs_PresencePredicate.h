/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <presence/cs_PresenceDescription.h>
#include <drivers/cs_Serial.h>

#include <array>


class PresencePredicate{
    public:
    typedef std::array<uint8_t, 9> SerializedDataType;

    // user id restrictions?
    enum class Condition : uint8_t { 
        VacuouslyTrue  = 0, 
        AnyoneInSelectedRooms  = 1, 
        NooneInSelectedRooms   = 2, 
        AnyoneInSphere = 3, 
        NooneInSphere  = 4
    };

    
    // private: DEBUG
    Condition cond;
    PresenceStateDescription RoomsBitMask;

    public:
    PresencePredicate(Condition c, PresenceStateDescription roomsMask);

    PresencePredicate(SerializedDataType arr);

    bool requiresPresence() const;
    bool requiresAbsence() const;
    

    SerializedDataType serialize();
    
    // parameter bit i is 1 whenever there is presence detected in the
    // room with index i.
    bool operator()(PresenceStateDescription currentroomspresencebitmask);

    void print(){
        uint64_t roommask = RoomsBitMask;

        [[maybe_unused]] uint32_t rooms[2] = {
            static_cast<uint32_t>(roommask >> 0 ),
            static_cast<uint32_t>(roommask >> 32)
        };
        LOGd("PresencePredicate(condition: %d roommask: 0x%04x 0x%04x)", static_cast<uint8_t>(cond) , rooms[1], rooms[0]);
    }

};
