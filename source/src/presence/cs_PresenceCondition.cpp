/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 25, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <presence/cs_PresenceCondition.h>
#include <util/cs_WireFormat.h>
#include <drivers/cs_Serial.h>

PresenceCondition::PresenceCondition(PresencePredicate p, uint32_t t) : 
    pred(p), timeOut(t){}

PresenceCondition::PresenceCondition(SerializedDataType arr): 
    PresenceCondition(
        WireFormat::deserialize<PresencePredicate>(arr.data() + 0, 9),
        WireFormat::deserialize<uint32_t>(         arr.data() + 9, 4)){
}

PresenceCondition::SerializedDataType PresenceCondition::serialize() const{
    SerializedDataType result;
    std::copy_n(std::begin(WireFormat::serialize(pred)),    9, std::begin(result) + 0);
    std::copy_n(std::begin(WireFormat::serialize(timeOut)), 4, std::begin(result) + 9);
    return result;
}

bool PresenceCondition::operator()(PresenceStateDescription currentPresence) const{
    return pred(currentPresence);
}