/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 20, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_ExtendedSwitchBehaviour.h>

#include <util/cs_WireFormat.h>

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

bool ExtendedSwitchBehaviour::requiresPresence();

bool ExtendedSwitchBehaviour::requiresAbsence();

bool ExtendedSwitchBehaviour::isValid(Time currenttime, PresenceStateDescription currentpresence);