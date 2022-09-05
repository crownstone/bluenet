/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 05, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <test/cs_TestAccess.h>
#include <testaccess/cs_PresenceDescription.h>
#include <presence/cs_PresencePredicate.h>

std::ostream & operator<< (std::ostream &out, PresencePredicate::Condition& c){
    return out << +static_cast<uint8_t>(c);
}

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

    static std::ostream & toStream(std::ostream &out, PresencePredicate& obj) {
        return out << "{"
                   << "_condition: " << obj._condition << ", "
                   << "_presence: " << obj._presence
                   << "}";
    }
};

/**
 * Allows streaming PresencePredicate objects to std::cout and other streams.
 * global definition forwards to TestAccess to elevate access rights for inspection.
 */
std::ostream & operator<< (std::ostream &out, PresencePredicate& s) {
    return TestAccess<PresencePredicate>::toStream(out,s);
}