/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <test/cs_TestAccess.h>
#include <presence/cs_PresenceDescription.h>

template<>
class TestAccess<PresenceStateDescription> {
public:
    uint64_t _bitmask;

    TestAccess() {
        reset();
    }

    void reset() {
        _bitmask = 0;
    }

    PresenceStateDescription get() {
        return {
                _bitmask
        };
    }

};