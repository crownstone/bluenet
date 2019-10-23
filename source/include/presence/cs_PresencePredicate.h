/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <util/cs_WireFormat.h>
#include <array>

class PresencePredicate{

    public:
    enum class Condition { 
        VacuouslyTrue  = 0, 
        AnyoneAnyRoom  = 1, 
        AnyoneInSphere = 2, 
        NooneAnyRoom   = 3, 
        NooneInSphere  = 4
    };
    
    private: 
    Condition cond;
    uint64_t RoomsBitMask;

    public:
    PresencePredicate(Condition c, uint64_t roomsMask): 
        cond(c), RoomsBitMask(roomsMask) {
    }

    PresencePredicate(std::array<uint8_t, 9> arr);
    
    // parameter bit i is 1 whenever there is presence detected in the
    // room with index i.
    bool evaluate(uint64_t currentroomspresencebitmask);

};