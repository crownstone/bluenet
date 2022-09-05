/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <test/cs_TestAccess.h>
#include <presence/cs_PresenceCondition.h>
#include <testaccess/cs_PresencePredicate.h>

template<>
class TestAccess<PresenceCondition> {
public:
    TestAccess<PresencePredicate> predicate;
    uint32_t timeOut;

    TestAccess() {
        reset();
    }

    void reset() {
       predicate.reset();
       timeOut = 60*5;
    }

    PresenceCondition get() {
        return {
            predicate.get(),
            timeOut
        };
    }

};