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
            LOGd("replace behaviour event to index 0");
            newBehaviour->print();
            saveBehaviour(*newBehaviour, 0);
        }
        case CS_TYPE::EVT_SAVE_BEHAVIOUR:{
            Behaviour* newBehaviour = reinterpret_cast<TYPIFY(EVT_SAVE_BEHAVIOUR)*>(evt.data);
            LOGd("saving behaviour event to index 0");
            newBehaviour->print();
            
            if(evt.result.data == nullptr || evt.result.len < sizeof(uint8_t)){
                // cannot communicate the result, so won't do anything.
                evt.returnCode = ERR_BUFFER_TOO_SMALL;
                return;
            }

            size_t empty_index = 0;
            while(activeBehaviours[empty_index].has_value() && empty_index < MaxBehaviours){
                empty_index++;
            }

            if(saveBehaviour(*newBehaviour,empty_index)){
                // found an empty spot in the list.
                evt.returnCode = EVENT_RESULT_SET;
                *reinterpret_cast<uint8_t*>(evt.result.data) = empty_index;
            } else {
                // behaviour store is full!
                evt.returnCode = ERR_NO_SPACE;
            }

            break;
        }
        case CS_TYPE::EVT_REMOVE_BEHAVIOUR:{
            LOGd("remove behaviour event");
            activeBehaviours[0].reset();
            break;
        }
        case CS_TYPE::EVT_GET_BEHAVIOUR:{
            LOGd("remove behaviour event");
            activeBehaviours[0].reset();
            break;
        }
        default:{
            break;
        }
    }
}

bool BehaviourStore::saveBehaviour(Behaviour b, uint8_t index){
    if(index >= MaxBehaviours){
        return false;
    }

    activeBehaviours[index] = b;
    return true;
}