/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <presence/cs_PresencePredicate.h>
#include <util/cs_WireFormat.h>
#include <logging/cs_Logger.h>
#include <time/cs_SystemTime.h>

PresencePredicate::PresencePredicate(Condition c, PresenceStateDescription roomsMask): 
    cond(c), RoomsBitMask(roomsMask) {
}

PresencePredicate::PresencePredicate(std::array<uint8_t, 9> arr) : 
    PresencePredicate(
        static_cast<Condition>(arr[0]),
        PresenceStateDescription(WireFormat::deserialize<uint64_t>(arr.data() +1, 8))) {
}

bool PresencePredicate::operator()(
        PresenceStateDescription currentroomspresencebitmask){
    switch(cond){
        case Condition::VacuouslyTrue: 
            return true;
        case Condition::AnyoneInSelectedRooms:
            return (static_cast<uint64_t>(currentroomspresencebitmask) & static_cast<uint64_t>(RoomsBitMask)) != 0;
        case Condition::NooneInSelectedRooms:
            return (static_cast<uint64_t>(currentroomspresencebitmask) & static_cast<uint64_t>(RoomsBitMask)) == 0;
        case Condition::AnyoneInSphere:
            return static_cast<uint64_t>(currentroomspresencebitmask) != 0;
        case Condition::NooneInSphere:
            return static_cast<uint64_t>(currentroomspresencebitmask) == 0;
    }
    
    return false;
}

bool PresencePredicate::requiresPresence() const { 
    return cond == Condition::AnyoneInSelectedRooms || cond == Condition::AnyoneInSphere; 
}

bool PresencePredicate::requiresAbsence() const { 
    return cond == Condition::NooneInSelectedRooms || cond ==Condition::NooneInSphere; 
}

PresencePredicate::SerializedDataType PresencePredicate::serialize() {
    SerializedDataType result;
    std::copy_n(std::begin(WireFormat::serialize(static_cast<uint8_t>(cond))),              1, std::begin(result) + 0);
    std::copy_n(std::begin(WireFormat::serialize(static_cast<uint64_t>(RoomsBitMask))),     8, std::begin(result) + 1);
    return result;
}
