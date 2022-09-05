/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <test/cs_TestAccess.h>
#include <presence/cs_PresencePredicate.h>
#include <testaccess/cs_PresenceDescription.h>

template<>
class TestAccess<PresencePredicate> {
public:
    PresencePredicate::Condition _condition;
    TestAccess<PresenceStateDescription> _presence;

    TestAccess() {
        reset();
    }

    void reset() {
        _condition = PresencePredicate::Condition::AnyoneInSelectedRooms;
        _presence.reset();
    }

    PresencePredicate get() {
        return {
                _condition, _presence.get()
        };
    }
};