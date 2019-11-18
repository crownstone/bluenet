/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/behaviour/cs_BehaviourStore.h>

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <events/cs_EventListener.h>
#include <protocol/cs_ErrorCodes.h>

#include <algorithm>

// allocate space for the behaviours.
std::array<std::optional<Behaviour>,BehaviourStore::MaxBehaviours> BehaviourStore::activeBehaviours;

void BehaviourStore::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_SAVE_BEHAVIOUR:{
            LOGd("save behaviour event");

            Behaviour* newBehaviour = reinterpret_cast<TYPIFY(EVT_SAVE_BEHAVIOUR)*>(evt.data);
            newBehaviour->print();
            
//            if(evt.result.data == nullptr || evt.result.len < sizeof(uint8_t)){
//                // cannot communicate the result, so won't do anything.
//                evt.returnCode = ERR_BUFFER_TOO_SMALL;
//                return;
//            }

            // find the first empty index.
            size_t empty_index = 0;
            while(activeBehaviours[empty_index].has_value() && empty_index < MaxBehaviours){
                empty_index++;
            }

            if(saveBehaviour(*newBehaviour,empty_index)){
                // found an empty spot in the list.
                evt.returnCode = ERR_SUCCESS;
                if(evt.result.data != nullptr && evt.result.len >= 5) {
                	uint8_t index = empty_index;
					*reinterpret_cast<uint32_t*>(evt.result.data + 0) = masterHash();
					*reinterpret_cast<uint8_t*>( evt.result.data + 4) = index;
					evt.result.len = sizeof(uint32_t) + sizeof(index);
                }
                else {
                	evt.result.len = 0;
                }
            } else {
                // behaviour store is full!
                evt.returnCode = ERR_NO_SPACE;
                if(evt.result.data != nullptr && evt.result.len >= 4) {
                    *reinterpret_cast<uint32_t*>(evt.result.data + 0) = masterHash();
                    evt.result.len = sizeof(uint32_t);
                }
                else {
                	evt.result.len = 0;
                }
            }

            break;
        }
        case CS_TYPE::EVT_REPLACE_BEHAVIOUR:{
            auto replace_request = reinterpret_cast<TYPIFY(EVT_REPLACE_BEHAVIOUR)*>(evt.data);
            
            
            if(saveBehaviour(std::get<1>(*replace_request), std::get<0>(*replace_request))){

                LOGd("replace successful");
                evt.returnCode = ERR_SUCCESS;
            } else {
                LOGd("replace failed");
                evt.returnCode = ERR_UNSPECIFIED;
            }
            if(evt.result.data != nullptr && evt.result.len >= 4) {
				uint32_t hash = masterHash();
				*reinterpret_cast<uint32_t*>(evt.result.data + 0) = hash;
				evt.result.len = sizeof(hash);
            }
            else {
            	evt.result.len = 0;
            }
            break;
        }
        case CS_TYPE::EVT_REMOVE_BEHAVIOUR:{
            LOGd("remove behaviour event");

            uint8_t index = *reinterpret_cast<TYPIFY(EVT_REMOVE_BEHAVIOUR)*>(evt.data);
            if(removeBehaviour(index)){
                evt.returnCode = ERR_SUCCESS;
            } else {
                evt.returnCode = ERR_NOT_FOUND;
            }
            if(evt.result.data != nullptr && evt.result.len >= 4) {
				uint32_t hash = masterHash();
				*reinterpret_cast<uint32_t*>(evt.result.data + 0) = hash;
				evt.result.len = sizeof(hash);
            }
            else {
            	evt.result.len = 0;
            }
            break;
        }
        case CS_TYPE::EVT_GET_BEHAVIOUR:{
            LOGd("Get behaviour event");
            uint8_t index = *reinterpret_cast<TYPIFY(EVT_GET_BEHAVIOUR)*>(evt.data);

            if(index >= MaxBehaviours || !activeBehaviours[index].has_value()){
                evt.result.len = 0;
                evt.returnCode = ERR_NOT_FOUND;
                return;
            }

            if(evt.result.data == nullptr) {
            	evt.result.len = 0;
            	evt.returnCode = ERR_BUFFER_UNASSIGNED;
            	return;
            }

            Behaviour::SerializedDataFormat bs = activeBehaviours[index]->serialize();

            if(evt.result.len < bs.size()){
                // cannot communicate the result, so won't do anything.
            	evt.result.len = 0;
                evt.returnCode = ERR_BUFFER_TOO_SMALL;
                return;
            }

            std::copy_n(bs.data(), bs.size(), evt.result.data);
            evt.result.len = bs.size();
            evt.returnCode = ERR_SUCCESS;
            break;
        }
        case CS_TYPE::EVT_GET_BEHAVIOUR_INDICES: {
        	if (evt.result.data == nullptr) {
        		evt.result.len = 0;
        		evt.returnCode = ERR_BUFFER_UNASSIGNED;
        		return;
        	}

        	uint8_t maxProfiles = 5;
        	if (evt.result.len < MaxBehaviours * maxProfiles) {
        		evt.result.len = 0;
        		evt.returnCode = ERR_BUFFER_TOO_SMALL;
        		return;
        	}
        	size_t listSize = 0;
        	for (uint8_t i = 0; i < MaxBehaviours; ++i) {
        		if (activeBehaviours[i].has_value()) {
        			evt.result.data[listSize++] = i;
        		}
        	}
        	evt.result.len = listSize;
        	evt.returnCode = ERR_SUCCESS;
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

    b.print();
    LOGd("save behaviour to index %d",index);

    activeBehaviours[index] = b;
    return true;
}

uint32_t BehaviourStore::masterHash(){
    return 0x12345678;
}
