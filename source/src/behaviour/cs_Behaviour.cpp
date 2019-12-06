/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 6, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_Behaviour.h>
#include <util/cs_WireFormat.h>

Behaviour::Behaviour(
            uint8_t intensity,
            DayOfWeekBitMask activedaysofweek,
            TimeOfDay from, 
            TimeOfDay until) : 
        activeIntensity (intensity),
        activeDays(activedaysofweek),
        behaviourAppliesFrom (from),
        behaviourAppliesUntil (until)
    {
}

Behaviour::Behaviour(SerializedDataType arr) : 
    Behaviour(
        WireFormat::deserialize<uint8_t>(arr.data() +           0,  WireFormat::size<uint8_t>()),
        // unused field 'profileID'
        WireFormat::deserialize<DayOfWeekBitMask>(arr.data() +  2,  WireFormat::size<DayOfWeekBitMask>()),
        WireFormat::deserialize<TimeOfDay>(arr.data() +         3,  WireFormat::size<TimeOfDay>()),
        WireFormat::deserialize<TimeOfDay>(arr.data() +         8,  WireFormat::size<TimeOfDay>())
    ){

}

Behaviour::SerializedDataType Behaviour::serialize() const{
    SerializedDataType result;
    std::copy_n( std::begin(WireFormat::serialize(activeIntensity)),        WireFormat::size(&activeIntensity), std::begin(result) + 0);
    std::copy_n( std::begin(WireFormat::serialize(activeDays)),             WireFormat::size(&activeDays), std::begin(result) + 2);
    std::copy_n( std::begin(WireFormat::serialize(behaviourAppliesFrom)),   WireFormat::size(&behaviourAppliesFrom), std::begin(result) + 3);
    std::copy_n( std::begin(WireFormat::serialize(behaviourAppliesUntil)),  WireFormat::size(&behaviourAppliesUntil), std::begin(result) + 8);

    return result;
}


uint8_t Behaviour::value() const {
    return activeIntensity;
}

TimeOfDay Behaviour::from() const {
    return behaviourAppliesFrom; 
}

TimeOfDay Behaviour::until() const {
    return behaviourAppliesUntil; 
}