/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_SwitchBehaviour.h>

#include <drivers/cs_Serial.h>
#include <time/cs_SystemTime.h>
#include <util/cs_WireFormat.h>

#include <algorithm>

#define LOGBehaviour_V LOGnone

SwitchBehaviour::SwitchBehaviour(
            uint8_t intensity,
            uint8_t profileid,
            DayOfWeekBitMask activedaysofweek,
            TimeOfDay from, 
            TimeOfDay until, 
            PresenceCondition presencecondition) :
        Behaviour(Behaviour::Type::Switch,intensity,profileid,activedaysofweek,from,until),
        presenceCondition (presencecondition)
    {
}

SwitchBehaviour::SwitchBehaviour(std::array<uint8_t, 1+26> arr) : 
    Behaviour(          WireFormat::deserialize<Behaviour>(         arr.data() +  0, 14)),
    presenceCondition(  WireFormat::deserialize<PresenceCondition>( arr.data() + 14, 13)){
}

SwitchBehaviour::SerializedDataType SwitchBehaviour::serialize() const{
    SerializedDataType result;
    std::copy_n(std::begin(Behaviour::serialize()),  WireFormat::size<Behaviour>(),            std::begin(result) + 0);
    std::copy_n(std::begin(WireFormat::serialize(presenceCondition)), WireFormat::size<PresenceCondition>(),    std::begin(result) + 14);

    return result;
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

    LOGd("SwitchBehaviour: %02d:%02d:%02d - %02d:%02d:%02d %3d%%, days(%x), presencetype(%d) roommask(%x %x), timeout(%d)",
        from().h(),from().m(),from().s(),
        until().h(),until().m(),until().s(),
        activeIntensity,
        activeDays,
        presenceCondition.pred.cond,
        rooms[1],rooms[0], 
        presenceCondition.timeOut
    );
}
