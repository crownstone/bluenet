/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <presence/cs_PresenceDescription.h>
#include <array>



class PresencePredicate{
    public:
    typedef std::array<uint8_t, 9> SerializedDataType;

    // user id restrictions?
    enum class Condition : uint8_t { 
        VacuouslyTrue  = 0, 
        AnyoneAnyRoom  = 1, 
        NooneAnyRoom   = 2, 
        AnyoneInSphere = 3, 
        NooneInSphere  = 4
    };

    
    // private: DEBUG
    Condition cond;
    uint64_t RoomsBitMask;

    public:
    PresencePredicate(Condition c, PresenceStateDescription roomsMask);

    PresencePredicate(SerializedDataType arr);

    SerializedDataType serialize() const;
    
    // parameter bit i is 1 whenever there is presence detected in the
    // room with index i.
    bool operator()(PresenceStateDescription currentroomspresencebitmask) const;

};