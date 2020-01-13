/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_ExtendedSwitchBehaviour.h>

#include <time/cs_SystemTime.h>
#include <util/cs_WireFormat.h>

#include <drivers/cs_Serial.h>

ExtendedSwitchBehaviour::ExtendedSwitchBehaviour(SwitchBehaviour corebehaviour, PresenceCondition extensioncondition) :
    SwitchBehaviour(corebehaviour), extensionCondition(extensioncondition) {
}

ExtendedSwitchBehaviour::ExtendedSwitchBehaviour(SerializedDataType arr) : 
    ExtendedSwitchBehaviour(
        WireFormat::deserialize<SwitchBehaviour>(   
            arr.data() +  0, 
            WireFormat::size<SwitchBehaviour>()
        ),  
        WireFormat::deserialize<PresenceCondition>( 
            arr.data() + WireFormat::size<SwitchBehaviour>(), 
            WireFormat::size<PresenceCondition>()
        )
    ) {
}

ExtendedSwitchBehaviour::SerializedDataType ExtendedSwitchBehaviour::serialize(){
    SerializedDataType result;

    serialize(result.data(),WireFormat::size<ExtendedSwitchBehaviour>());

    return result;
}

uint8_t* ExtendedSwitchBehaviour::serialize(uint8_t* outbuff, size_t max_size){
    if(max_size != 0){
        if (outbuff == nullptr || max_size < serializedSize()) {
            // all or nothing..
            return outbuff;
        }
    }

    // outbuff is big enough :)

    outbuff = SwitchBehaviour::serialize(outbuff, max_size);
    outbuff = extensionCondition.serialize(outbuff);

    return outbuff;
}

size_t ExtendedSwitchBehaviour::serializedSize() const {
    return WireFormat::size<ExtendedSwitchBehaviour>();
}

bool ExtendedSwitchBehaviour::requiresPresence(){
    return extensionIsActive 
        ? presenceCondition.pred.requiresPresence()
        : extensionCondition.pred.requiresPresence();
}

bool ExtendedSwitchBehaviour::requiresAbsence(){
    return extensionIsActive 
        ? presenceCondition.pred.requiresAbsence()
        : extensionCondition.pred.requiresAbsence();
}

bool ExtendedSwitchBehaviour::isValid(Time currenttime, PresenceStateDescription currentpresence){
    // implementation detail: 
    // SwitchBehaviour::isValid(PresenceStateDescription) caches the last valid presence timestamp.
    // However, this must be recomputed in the extension anyway because the conditions may differ.

    if (SwitchBehaviour::isValid(currenttime)) {
        // currenttime between from() and until()
        extensionIsActive = SwitchBehaviour::isValid(currentpresence);
        return extensionIsActive;
    }

    if (!extensionIsActive) {
        // not extended, nor in active time slot
        return false;
    }

    if (extensionCondition(currentpresence)) {
        // in extension and presence match
        prevExtensionIsValidTimeStamp = SystemTime::posix();
        return true;
    }

    if(prevExtensionIsValidTimeStamp){
        if ( CsMath::Interval<uint32_t>(
                SystemTime::posix().timestamp(), extensionCondition.timeOut, true )
            .contains(prevExtensionIsValidTimeStamp->timestamp()) ) {
            // in extension and presence is invalid,
            // but we're in the extension's grace period.
            return true;
        } 
    }
    
    // deactivate
    extensionIsActive = false;
    prevExtensionIsValidTimeStamp.reset();

    return false;
}

void ExtendedSwitchBehaviour::print(){
    LOGd("## ExtendedSwitchBehaviour:")
    SwitchBehaviour::print();
    extensionCondition.pred.print();
    
    LOGd("extension is active: %d", extensionIsActive);

    if(prevExtensionIsValidTimeStamp){
        TimeOfDay t = prevExtensionIsValidTimeStamp->timeOfDay();
        LOGd("extension timestamp : %02d:%02d:%02d", t.h(),t.m(),t.s());
    }
    LOGd("##");

}