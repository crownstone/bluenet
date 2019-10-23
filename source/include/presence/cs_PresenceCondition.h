/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <util/cs_WireFormat.h>

class PresenceCondition{
    public:
    PresencePredicate pred;
    uint32_t timeOut;

    PresenceCondition(PresencePredicate p, uint32_t t) : pred(p), timeOut(t){}

    PresenceCondition(std::array<uint8_t,9+4> arr): PresenceCondition(
        WireFormat::deserialize<PresencePredicate>(arr + 0, 9),
        WireFormat::deserialize<uint32_t>(         arr + 9, 4)){
    }

};