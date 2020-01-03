/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 6, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_Behaviour.h>
#include <util/cs_WireFormat.h>
#include <drivers/cs_Serial.h>
#include <time/cs_SystemTime.h>

Behaviour::Behaviour(
            Type typ,
            uint8_t intensity,
            uint8_t profileid,
            DayOfWeekBitMask activedaysofweek,
            TimeOfDay from, 
            TimeOfDay until) : 
        typ(typ),
        activeIntensity (intensity),
        profileId (profileid),
        activeDays(activedaysofweek),
        behaviourAppliesFrom (from),
        behaviourAppliesUntil (until)
    {
}

Behaviour::Behaviour(SerializedDataType arr) : 
    Behaviour(
        Type(WireFormat::deserialize<uint8_t>(arr.data() +        0,  WireFormat::size<uint8_t>())),
        WireFormat::deserialize<uint8_t>(arr.data() +             1,  WireFormat::size<uint8_t>()),
        WireFormat::deserialize<uint8_t>(arr.data() +             2,  WireFormat::size<uint8_t>()),
        WireFormat::deserialize<DayOfWeekBitMask>(arr.data() +    3,  WireFormat::size<DayOfWeekBitMask>()),
        WireFormat::deserialize<TimeOfDay>(arr.data() +           4,  WireFormat::size<TimeOfDay>()),
        WireFormat::deserialize<TimeOfDay>(arr.data() +           9,  WireFormat::size<TimeOfDay>())
    ){
}

Behaviour::SerializedDataType Behaviour::serialize() {
    SerializedDataType result;
    auto result_iter = std::begin(result);

    result_iter = std::copy_n( std::begin(WireFormat::serialize(static_cast<uint8_t>(typ))),  WireFormat::size<uint8_t>(),     result_iter);
    result_iter = std::copy_n( std::begin(WireFormat::serialize(activeIntensity)),            WireFormat::size<uint8_t>(),     result_iter);
    result_iter = std::copy_n( std::begin(WireFormat::serialize(profileId)),                  WireFormat::size<uint8_t>(),     result_iter);
    result_iter = std::copy_n( std::begin(WireFormat::serialize(activeDays)),                 WireFormat::size<uint8_t>(),     result_iter);
    result_iter = std::copy_n( std::begin(WireFormat::serialize(behaviourAppliesFrom)),       WireFormat::size<TimeOfDay>(),   result_iter);
    result_iter = std::copy_n( std::begin(WireFormat::serialize(behaviourAppliesUntil)),      WireFormat::size<TimeOfDay>(),   result_iter);

    return result;
}

uint8_t* Behaviour::serialize(uint8_t* outbuff, size_t max_size) {
    const auto size = serializedSize();

    if(max_size < size){
        return 0;
    }

    return std::copy_n(std::begin(serialize()),size,outbuff);
}

size_t Behaviour::serializedSize() const {
    return WireFormat::size<Behaviour>();
}

std::vector<uint8_t> Behaviour::serialized(){
    // TODO(Arend, 12-12-2019): 
    // The intermediate std::array object in the underlying Behaviour::serialize() 
    // can be avoided in this call if we
    // set up serialization the other way around: make the serialize(outbuff*,size)
    // variation the primitive one and then serialize to a stack variable or heap 
    // in the othter variatons.
    // note however that this method works for all subclasses too because 
    // serialize(uint8_t*,size_t) and serializedSize are virtual :)

    std::vector<uint8_t> vec (serializedSize()) ; // preallocate correct size vector.
    serialize(vec.data(),vec.size());

    return vec;
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

bool Behaviour::isValid(Time currenttime){
//	LOGd("activeDays=%u dayOfWeek=%u timeofday=%02d:%02d:%02d", activeDays, static_cast<uint8_t>(currenttime.dayOfWeek()), currenttime.timeOfDay().h(), currenttime.timeOfDay().m(), currenttime.timeOfDay().s());
    if (activeDays & static_cast<uint8_t>(currenttime.dayOfWeek())) {
        return from() < until() // ensure proper midnight roll-over 
            ? (from() <= currenttime && currenttime < until()) 
            : (from() <= currenttime || currenttime < until());
    }
    return false;
}

void Behaviour::print() {
    LOGd("Behaviour: type(%d) %02d:%02d:%02d - %02d:%02d:%02d %3d%%, days(%x) for #% (%s)",
        static_cast<uint8_t>(typ),
        from().h(),from().m(),from().s(),
        until().h(),until().m(),until().s(),
        activeIntensity,
        activeDays,
        profileId,
        (isValid(SystemTime::now()) ? "valid" : "invalid")
    );
}
