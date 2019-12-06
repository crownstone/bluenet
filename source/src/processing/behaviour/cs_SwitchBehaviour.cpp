/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/behaviour/cs_SwitchBehaviour.h>

#include <drivers/cs_Serial.h>
#include <time/cs_SystemTime.h>
#include <util/cs_WireFormat.h>

#include <algorithm>

#define LOGBehaviour_V LOGnone

SwitchBehaviour::SwitchBehaviour(
            uint8_t intensity,
            DayOfWeekBitMask activedaysofweek,
            TimeOfDay from, 
            TimeOfDay until, 
            PresenceCondition presencecondition) :
        activeIntensity (intensity),
        activeDays(activedaysofweek),
        behaviourAppliesFrom (from),
        behaviourAppliesUntil (until),
        presenceCondition (presencecondition)
    {
}

SwitchBehaviour::SwitchBehaviour(std::array<uint8_t, 26> arr) : 
    SwitchBehaviour(
        WireFormat::deserialize<uint8_t>(arr.data() + 0, 1),
        // WireFormat::deserialize<uint8_t>(arr.data() + 1, 1), // unused byte, 'options'
        WireFormat::deserialize<uint8_t>(arr.data() + 2, 1),
        WireFormat::deserialize<TimeOfDay>(arr.data() + 3, 5),
        WireFormat::deserialize<TimeOfDay>(arr.data() + 8, 5),
        WireFormat::deserialize<PresenceCondition>(arr.data() + 13, 13) ){
}

SwitchBehaviour::SerializedDataFormat SwitchBehaviour::serialize() const{
    SerializedDataFormat result;
    std::copy_n(std::begin(WireFormat::serialize(activeIntensity)),       1, std::begin(result) + 0);
    std::copy_n(std::begin(WireFormat::serialize(activeDays)),            1, std::begin(result) + 2);
    std::copy_n(std::begin(WireFormat::serialize(behaviourAppliesFrom)),  5, std::begin(result) + 3);
    std::copy_n(std::begin(WireFormat::serialize(behaviourAppliesUntil)), 5, std::begin(result) + 8);
    std::copy_n(std::begin(WireFormat::serialize(presenceCondition)),     13, std::begin(result) + 13);
    return result;
}

uint8_t SwitchBehaviour::value() const {
    return activeIntensity;
}

TimeOfDay SwitchBehaviour::from() const {
    return behaviourAppliesFrom; 
}

TimeOfDay SwitchBehaviour::until() const {
    return behaviourAppliesUntil; 
}

bool SwitchBehaviour::isValid(TimeOfDay currenttime, PresenceStateDescription currentpresence){
    return isValid(currenttime) && isValid(currentpresence);
}

bool SwitchBehaviour::isValid(TimeOfDay currenttime){
    return from() < until() // ensure proper midnight roll-over 
        ? (from() <= currenttime && currenttime < until()) 
        : (from() <= currenttime || currenttime < until());
}

bool SwitchBehaviour::isValid(PresenceStateDescription currentpresence){
    if(_isValid(currentpresence)){
        prevIsValidTimeStamp = SystemTime::up();
        return true;
    } 
    
    if(prevIsValidTimeStamp){
        if (CsMath::Interval(SystemTime::up(), PresenceIsValidTimeOut_s, true).contains(*prevIsValidTimeStamp)) {
            // presence invalid but we're in the grace period.
            LOGBehaviour_V("grace period for SwitchBehaviour::isActive, %d in [%d %d]", *prevIsValidTimeStamp, SystemTime::up() - *prevIsValidTimeStamp, SystemTime::up() );
            return true;
        } else {
            // fell out of grace, lets delete prev val.
            LOGBehaviour_V("grace period for SwitchBehaviour::isActive is over, %d in [%d %d]", *prevIsValidTimeStamp, SystemTime::up() - *prevIsValidTimeStamp, SystemTime::up() );
            prevIsValidTimeStamp.reset();
        }
    } 

    return false;
}

bool SwitchBehaviour::_isValid(PresenceStateDescription currentpresence){
    return presenceCondition(currentpresence);
}

void SwitchBehaviour::print() const {
    uint32_t rooms[2] = {
        static_cast<uint32_t>(presenceCondition.pred.RoomsBitMask >> 0 ),
        static_cast<uint32_t>(presenceCondition.pred.RoomsBitMask >> 32)
    };

    LOGd("SwitchBehaviour: %02d:%02d:%02d - %02d:%02d:%02d %3d%%, days(%x), presencetype(%d) roommask(%x %x)",
        from().h(),from().m(),from().s(),
        until().h(),until().m(),until().s(),
        activeIntensity,
        activeDays,
        presenceCondition.pred.cond,
        rooms[1],rooms[0]
    );
}
