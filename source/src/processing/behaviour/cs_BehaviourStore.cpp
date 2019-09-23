/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/behaviour/cs_BehaviourStore.h>

#include <events/cs_EventListener.h>
#include <common/cs_Types.h>

// allocate space for the behaviours.
// std::array<Behaviour,BehaviourStore::MaxBehaviours> BehaviourStore::activeBehaviours;

void BehaviourStore::handleEvent(event_t& evt){
    switch(evt.type){
        default:{
            break;
        }
    }
}