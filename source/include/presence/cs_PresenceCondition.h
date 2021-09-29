/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <presence/cs_PresencePredicate.h>

#include <array>

class PresenceCondition {
    public:
    typedef std::array<uint8_t,9+4> SerializedDataType;

    PresencePredicate predicate;
    uint32_t timeOut;

    PresenceCondition(PresencePredicate p, uint32_t t);
    PresenceCondition(SerializedDataType arr);

    SerializedDataType serialize();

    // return value: pointer to next empty val in outbuff.
    // if max_size is 0, outbuff is not checked for nullptr,
    // and no size check is performed. otherwise, both are validated.
    virtual uint8_t* serialize(uint8_t* outbuff, size_t maxSize = 0);

    virtual size_t serializedSize() const;

    /**
     * Returns true if this condition is satisfied given the presence state.
     */
    bool isTrue(PresenceStateDescription currentPresence);
};
