/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <presence/cs_PresencePredicate.h>



PresencePredicate(std::array<uint8_t, 9> arr) : 
    PresencePredicate(
        static_cast<Condition>(arr[0]),
        WireFormat::deserialize<uint64_t>(arr.data(),8)) {
}

bool PresencePredicate::evaluate(uint64_t currentroomspresencebitmask){
    switch(cond){
        case Condition::VacuouslyTrue: 
            return true;
        case Condition::AnyoneAnyRoom:
            return (currentroomspresencebitmask & RoomsBitMask) != 0;
        case Condition::NooneAnyRoom:
            return (currentroomspresencebitmask & RoomsBitMask) == 0;
        case Condition::AnyoneInSphere:
            return currentroomspresencebitmask != 0;
        case Condition::NooneInSphere:
            return currentroomspresencebitmask == 0;
    }
}