/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <presence/cs_PresencePredicate.h>
#include <util/cs_WireFormat.h>
#include <drivers/cs_Serial.h>
#include <time/cs_SystemTime.h>

PresencePredicate::PresencePredicate(Condition c, PresenceStateDescription roomsMask): 
    cond(c), RoomsBitMask(roomsMask) {
}

PresencePredicate::PresencePredicate(std::array<uint8_t, 9> arr) : 
    PresencePredicate(
        static_cast<Condition>(arr[0]),
        WireFormat::deserialize<uint64_t>(arr.data() +1, 8)) {
}

bool PresencePredicate::operator()(
        PresenceStateDescription currentroomspresencebitmask) const{
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
    
    return false;
}

PresencePredicate::SerializedDataType PresencePredicate::serialize() const {
    SerializedDataType result;
    std::copy_n(std::begin(WireFormat::serialize(static_cast<uint8_t>(cond))),  1, std::begin(result) + 0);
    std::copy_n(std::begin(WireFormat::serialize(RoomsBitMask)),                8, std::begin(result) + 1);
    return result;
}