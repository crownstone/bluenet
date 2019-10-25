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
std::array<std::optional<Behaviour>,BehaviourStore::MaxBehaviours> BehaviourStore::activeBehaviours;

void BehaviourStore::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_REPLACE_BEHAVIOUR:{
            Behaviour* newBehaviour = reinterpret_cast<TYPIFY(EVT_REPLACE_BEHAVIOUR)*>(evt.data);
            LOGd("newBehaviour: from %d, until %d", newBehaviour->from(), newBehaviour->until());

        }
        case CS_TYPE::EVT_SAVE_BEHAVIOUR:{
            Behaviour* newBehaviour = reinterpret_cast<TYPIFY(EVT_SAVE_BEHAVIOUR)*>(evt.data);
            LOGd("save behaviour event: from %d, until %d", newBehaviour->from(), newBehaviour->until());

            break;
        }
        case CS_TYPE::EVT_REMOVE_BEHAVIOUR:{
            LOGd("remove behaviour event");

            break;
        }
        default:{
            break;
        }
    }
}

bool BehaviourStore::saveBehaviour(Behaviour b, uint8_t index){
    if(index > MaxBehaviours){
        return false;
    }

    activeBehaviours[index] = b;
    return true;
}