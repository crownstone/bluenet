/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 25, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <presence/cs_PresenceCondition.h>
#include <util/cs_WireFormat.h>

PresenceCondition::PresenceCondition(PresencePredicate p, uint32_t t) : 
    pred(p), timeOut(t){}

PresenceCondition::PresenceCondition(std::array<uint8_t,9+4> arr): 
    PresenceCondition(
        WireFormat::deserialize<PresencePredicate>(arr.data() + 0, 9),
        WireFormat::deserialize<uint32_t>(         arr.data() + 9, 4)){
}

bool PresenceCondition::operator()(PresenceStateDescription currentPresence) const{
    return pred(currentPresence);
}