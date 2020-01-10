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

SwitchBehaviour::SerializedDataType SwitchBehaviour::serialize(){
    SerializedDataType result;
    std::copy_n(std::begin(Behaviour::serialize()),  WireFormat::size<Behaviour>(),            std::begin(result) + 0);
    std::copy_n(std::begin(WireFormat::serialize(presenceCondition)), WireFormat::size<PresenceCondition>(),    std::begin(result) + 14);

    return result;
}

uint8_t* SwitchBehaviour::serialize(uint8_t* outbuff, size_t max_size){
    const auto size = serializedSize();

    if(max_size < size){
        return 0;
    }

    return std::copy_n(std::begin(serialize()),size,outbuff);
}


size_t SwitchBehaviour::serializedSize() const {
    return WireFormat::size<SwitchBehaviour>();
}

bool SwitchBehaviour::requiresPresence() { 
    return presenceCondition.pred.requiresPresence();
}
bool SwitchBehaviour::requiresAbsence() {
    return presenceCondition.pred.requiresAbsence();
}

bool SwitchBehaviour::isValid(Time currenttime, PresenceStateDescription currentpresence){
    return isValid(currenttime) && isValid(currentpresence);
}

bool SwitchBehaviour::isValid(PresenceStateDescription currentpresence) {
    LOGBehaviour_V("isValid(presence) called");
    if (!requiresPresence() && !requiresAbsence()) {
    	return true;
    }

    // Presence stays valid when profile left a room, not when entering.
    if (_isValid(currentpresence) && requiresPresence()) {
    	// 9-1-2020 TODO Bart @ Arend: this relies on isValid(presence) to be called often.
        prevInRoomTimeStamp = SystemTime::up();
        return true;
    }
    if (!_isValid(currentpresence) && requiresAbsence()) {
    	// 9-1-2020 TODO Bart @ Arend: this relies on isValid(presence) to be called often.
    	prevInRoomTimeStamp = SystemTime::up();
    	return false;
    }
    
    if (prevInRoomTimeStamp) {
    	bool gracePeriod;
        if (CsMath::Interval(SystemTime::up(), presenceCondition.timeOut, true).contains(*prevInRoomTimeStamp)) {
            // presence invalid but we're in the grace period.
            LOGBehaviour_V("grace period for SwitchBehaviour::isActive, %d in [%d %d]", *prevInRoomTimeStamp, SystemTime::up() - presenceCondition.timeOut, SystemTime::up() );
            gracePeriod = true;
        } else {
            // fell out of grace, lets delete prev val.
            LOGBehaviour_V("grace period for SwitchBehaviour::isActive is over, %d in [%d %d]", *prevInRoomTimeStamp, SystemTime::up() - presenceCondition.timeOut, SystemTime::up() );
            prevInRoomTimeStamp.reset();
            gracePeriod = false;
        }
        if (requiresPresence()) {
        	return gracePeriod;
        }
        if (requiresAbsence()) {
        	return !gracePeriod;
        }
    }
    // Not valid, and not in grace period.
    return false;
}

bool SwitchBehaviour::_isValid(PresenceStateDescription currentpresence){
    return presenceCondition(currentpresence);
}

void SwitchBehaviour::print(){
    LOGd("SwitchBehaviour: %02d:%02d:%02d - %02d:%02d:%02d %3d%%, days(0x%x), presencetype(%d), timeout(%d) (%s)",
        from().h(),from().m(),from().s(),
        until().h(),until().m(),until().s(),
        activeIntensity,
        activeDays,
        presenceCondition.pred.cond,
        presenceCondition.timeOut,
        (isValid(SystemTime::now()) ? "valid" : "invalid")
    );
    presenceCondition.pred.RoomsBitMask.print();
}
