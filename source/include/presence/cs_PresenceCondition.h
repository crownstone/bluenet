/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <presence/cs_PresencePredicate.h>
#include <array>

class PresenceCondition{
    public:
    typedef std::array<uint8_t,9+4> SerializedDataType;

    PresencePredicate pred;
    uint32_t timeOut;

    PresenceCondition(PresencePredicate p, uint32_t t);
    PresenceCondition(SerializedDataType arr);

    SerializedDataType serialize() const;

    /**
     * Does this condition hold given the [currentPresence]?
     * 
     * TODO: cache result of previous call with timestamp and
     * use those to implement the time out feature
     */
    bool operator()(PresenceStateDescription currentPresence) const;
};
