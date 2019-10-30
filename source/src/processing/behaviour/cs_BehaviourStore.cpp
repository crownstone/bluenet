/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/behaviour/cs_BehaviourStore.h>

#include <events/cs_EventListener.h>
#include <common/cs_Types.h>

#include <algorithm>

// allocate space for the behaviours.
std::array<std::optional<Behaviour>,BehaviourStore::MaxBehaviours> BehaviourStore::activeBehaviours;

void BehaviourStore::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_REPLACE_BEHAVIOUR:{
            Behaviour* newBehaviour = reinterpret_cast<TYPIFY(EVT_REPLACE_BEHAVIOUR)*>(evt.data);
            LOGd("replace behaviour event to index 0");
            newBehaviour->print();
            saveBehaviour(*newBehaviour, 0);

            // Return master hash
        }
        case CS_TYPE::EVT_SAVE_BEHAVIOUR:{
            LOGd("save behaviour event");

            Behaviour* newBehaviour = reinterpret_cast<TYPIFY(EVT_SAVE_BEHAVIOUR)*>(evt.data);
            newBehaviour->print();
            
            if(evt.result.data == nullptr || evt.result.len < sizeof(uint8_t)){
                // cannot communicate the result, so won't do anything.
                evt.returnCode = ERR_BUFFER_TOO_SMALL;
                return;
            }

            // find the first empty index.
            size_t empty_index = 0;
            while(activeBehaviours[empty_index].has_value() && empty_index < MaxBehaviours){
                empty_index++;
            }

            if(saveBehaviour(*newBehaviour,empty_index)){
                // found an empty spot in the list.
                evt.returnCode = EVENT_RESULT_SET;
                reinterpret_cast<uint8_t*>(evt.result.data)[0] = empty_index;
                evt.result.len = sizeof(empty_index);
            } else {
                // behaviour store is full!
                evt.returnCode = ERR_NO_SPACE;
            }

            // TODO add master hash

            break;
        }
        case CS_TYPE::EVT_REMOVE_BEHAVIOUR:{
            LOGd("remove behaviour event");

            uint8_t index = *reinterpret_cast<TYPIFY(EVT_REMOVE_BEHAVIOUR)*>(evt.data);
            if(removeBehaviour(index)){
                evt.returnCode = EVENT_RESULT_SET;
                evt.result.len = 0;
            } else {
                evt.returnCode = ERR_NOT_FOUND;
            }
            
            // add master hash

            break;
        }
        case CS_TYPE::EVT_GET_BEHAVIOUR:{
            LOGd("Get behaviour event");
            uint8_t index = *reinterpret_cast<TYPIFY(EVT_REMOVE_BEHAVIOUR)*>(evt.data);

            if(index >= MaxBehaviours || !activeBehaviours[index].has_value()){
                evt.returnCode = ERR_NOT_FOUND;
                return;
            }

            Behaviour::SerializedDataFormat bs = activeBehaviours[index]->serialize();

            if(evt.result.data == nullptr || evt.result.len < bs.size()){
                // cannot communicate the result, so won't do anything.
                evt.returnCode = ERR_BUFFER_TOO_SMALL;
                return;
            }

            evt.returnCode = EVENT_RESULT_SET;
            std::copy_n(bs.data(), bs.size(), evt.result.data);
            evt.result.len = bs.size();

            break;
        }
        default:{
            break;
        }
    }
}

bool BehaviourStore::removeBehaviour(uint8_t index){
    if(index >= MaxBehaviours || !activeBehaviours[index].has_value()){
        return false;
    }
    
    activeBehaviours[index].reset();
    return true;
}

bool BehaviourStore::saveBehaviour(Behaviour b, uint8_t index){
    if(index >= MaxBehaviours){
        return false;
    }

    activeBehaviours[index] = b;
    return true;
}