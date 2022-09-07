/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 07, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <test/cs_TestAccess.h>
#include <behaviour/cs_BehaviourStore.h>

template<>
class TestAccess<BehaviourStore> {
public:

    static std::ostream& toStream(std::ostream& out, BehaviourStore& store) {
        // TODO: only prints switch behaviours for now.  implement twilight.
        out << "{" << std::endl;
        for (auto i{0}; i < BehaviourStore::MaxBehaviours; i++){
            if(auto switchBehaviour = dynamic_cast<SwitchBehaviour*>(store.activeBehaviours[i])) {
                out << i << ": " << *switchBehaviour << std::endl;
            }
        }
        out << "}";
        return out;
    }
};


/**
 * Allows streaming BehaviourStore objects to std::cout and other streams.
 * global definition forwards to TestAccess to elevate access rights for inspection.
 */
std::ostream & operator<< (std::ostream &out, BehaviourStore& s){
    return TestAccess<BehaviourStore>::toStream(out,s);
}