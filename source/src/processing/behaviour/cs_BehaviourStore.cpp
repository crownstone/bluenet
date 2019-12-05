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
#include <util/cs_Hash.h>
#include <util/cs_WireFormat.h>

#include <algorithm>

// allocate space for the behaviours.
std::array<std::optional<Behaviour>,BehaviourStore::MaxBehaviours> BehaviourStore::activeBehaviours = {};

void BehaviourStore::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_SAVE_BEHAVIOUR:{
            handleSaveBehaviour(evt);
            dispatchBehaviourMutationEvent();
            break;
        }
        case CS_TYPE::EVT_REPLACE_BEHAVIOUR:{
            handleReplaceBehaviour(evt);
            dispatchBehaviourMutationEvent();
            break;
        }
        case CS_TYPE::EVT_REMOVE_BEHAVIOUR:{
            handleRemoveBehaviour(evt);
            dispatchBehaviourMutationEvent();
            break;
        }
        case CS_TYPE::EVT_GET_BEHAVIOUR:{
            handleGetBehaviour(evt);
            break;
        }
        case CS_TYPE::EVT_GET_BEHAVIOUR_INDICES: {
            handleGetBehaviourIndices(evt);
        }
        default:{
            break;
        }
    }
}

// ==================== handler functions ====================

void BehaviourStore::handleSaveBehaviour(event_t& evt){

	if(evt.size < 1){
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, evt.size);
		evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
        return;
	}

	Behaviour::Type typ = static_cast<Behaviour::Type>(evt.getData()[0]);

	switch(typ){
		case Behaviour::Type::Switch:{
			// Its a switch behaviour packet, let's check the size
			if(evt.size - 1 != sizeof(Behaviour::SerializedDataFormat)){
				LOGe(FMT_WRONG_PAYLOAD_LENGTH " while type of behaviour to save: size(%d) type (%d)", evt.size,typ);
                evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
				return;
			}

			Behaviour b = WireFormat::deserialize<Behaviour>(evt.getData() + 1, evt.size - 1);

            LOGd("save behaviour event");

            // find the first empty index.
            size_t empty_index = 0;
            while(activeBehaviours[empty_index].has_value() && empty_index < MaxBehaviours){
                empty_index++;
            }
            
            evt.result.returnCode = saveBehaviour(b, empty_index);

            if(evt.result.buf.data == nullptr || evt.result.buf.len < sizeof(uint8_t) + sizeof(uint32_t)) {
                evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
            } else {
                uint8_t index = evt.result.returnCode == ERR_SUCCESS ? empty_index : 0xff;

                *reinterpret_cast<uint8_t*>( evt.result.buf.data + 0) = index;
                *reinterpret_cast<uint32_t*>(evt.result.buf.data + 1) = masterHash();
                evt.result.dataSize = sizeof(uint32_t) + sizeof(index);
            }

			return;
		}
		case Behaviour::Type::Twilight:{
			LOGe("Not implemented: save twilight");
			break;
		}
		case Behaviour::Type::Extended:{
			LOGe("Note implemented: save extended behaviour");
		}
		default:{
			LOGe("Invalid behaviour type: %d", typ);
		}
	}

}

void BehaviourStore::handleReplaceBehaviour(event_t& evt){
	if(evt.size < 2){
		LOGe(FMT_WRONG_PAYLOAD_LENGTH " %d", evt.size);
		evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
        return;
	}

    uint8_t* dat = static_cast<uint8_t*>(evt.data);
	
    uint8_t index = dat[0];
	Behaviour::Type type = static_cast<Behaviour::Type>(dat[1]);

	switch(type){
		case Behaviour::Type::Switch:{
			// Its a switch behaviour packet, let's check the size
			if(evt.size -2 != sizeof(Behaviour::SerializedDataFormat)){
				LOGe(FMT_WRONG_PAYLOAD_LENGTH "(% d)", evt.size);
				evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
                return;
			}

			Behaviour b = WireFormat::deserialize<Behaviour>(evt.getData() + 2, evt.size - 2);
            
            evt.result.returnCode = saveBehaviour(b, index);

            if(evt.result.buf.data == nullptr || evt.result.buf.len < sizeof(uint8_t) + sizeof(uint32_t)) {
                LOGd("ERR_BUFFER_TOO_SMALL");
                evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
                break;
            }

            evt.result.buf.data[0] = index;
            *reinterpret_cast<uint32_t*>(evt.result.buf.data + sizeof(uint8_t)) = masterHash();
            evt.result.dataSize = sizeof(uint8_t) + sizeof(uint32_t);
            break;
		}
		case Behaviour::Type::Twilight:{
			LOGe("Not implemented: save twilight");
			break;
		}
		case Behaviour::Type::Extended:{
			LOGe("Note implemented: save extended behaviour");
			break;
		}
		default:{
			LOGe("Invalid behaviour type");
			break;
		}
	}

}

void BehaviourStore::handleRemoveBehaviour(event_t& evt){
    uint8_t index = *reinterpret_cast<TYPIFY(EVT_REMOVE_BEHAVIOUR)*>(evt.data);
    LOGd("remove behaviour event %d", index);
    
    evt.result.returnCode = removeBehaviour(index);
    
    if(evt.result.buf.data == nullptr || evt.result.buf.len < sizeof(uint8_t) + sizeof(uint32_t)) {
        LOGd("ERR_BUFFER_TOO_SMALL");
        evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
        return;
    }
    
    evt.result.buf.data[0] = index;
    *reinterpret_cast<uint32_t*>(evt.result.buf.data + sizeof(uint8_t)) = masterHash();
    evt.result.dataSize = sizeof(uint8_t) + sizeof(uint32_t);
}

void BehaviourStore::handleGetBehaviour(event_t& evt){
    uint8_t index = *reinterpret_cast<TYPIFY(EVT_GET_BEHAVIOUR)*>(evt.data);
    LOGd("Get behaviour event %d ", index);

    if(index >= MaxBehaviours || !activeBehaviours[index].has_value()){
        LOGd("ERR_NOT_FOUND");
        evt.result.returnCode = ERR_NOT_FOUND;
        return;
    }

    activeBehaviours[index]->print();

    if(evt.result.buf.data == nullptr) {
        LOGd("ERR_BUFFER_UNASSIGNED");
        evt.result.returnCode = ERR_BUFFER_UNASSIGNED;
        return;
    }

    Behaviour::SerializedDataFormat bs = activeBehaviours[index]->serialize();

    if(evt.result.buf.len < bs.size() + sizeof(uint8_t)){
        // cannot communicate the result, so won't do anything.
        LOGd("ERR_BUFFER_TOO_SMALL");
        evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
        return;
    }

    std::copy_n(bs.data(), bs.size(), evt.result.buf.data + sizeof(uint8_t));
    evt.result.dataSize = bs.size() + sizeof(uint8_t);
    evt.result.returnCode = ERR_SUCCESS;
}

void BehaviourStore::handleGetBehaviourIndices(event_t& evt){
    LOGd("handle EVT_GET_BEHAVIOUR_INDICES");
    if (evt.result.buf.data == nullptr) {
        LOGd("ERR_BUFFER_UNASSIGNED");
        evt.result.returnCode = ERR_BUFFER_UNASSIGNED;
        return;
    }

    if ( evt.result.buf.len < MaxBehaviours * (sizeof(uint8_t) + sizeof(uint32_t)) ) {
        LOGd("ERR_BUFFER_TOO_SMALL");
        evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
        return;
    }

    size_t listSize = 0;
    for (uint8_t i = 0; i < MaxBehaviours; ++i) {
        if (activeBehaviours[i].has_value()) {
            evt.result.buf.data[listSize] = i;
            listSize += sizeof(uint8_t);

            *reinterpret_cast<uint32_t*>(evt.result.buf.data) = 
                Fletcher(activeBehaviours[i]->serialize().data(),sizeof(Behaviour::SerializedDataFormat) );
            listSize += sizeof(uint32_t);

            LOGd("behaviour found at index %d",i);
        }
    }
    evt.result.dataSize = listSize;
    evt.result.returnCode = ERR_SUCCESS;
}

void BehaviourStore::dispatchBehaviourMutationEvent(){
    event_t evt(CS_TYPE::EVT_BEHAVIOURSTORE_MUTATION,nullptr,0);
    evt.dispatch();
}

// ==================== public functions ====================

ErrorCodesGeneral BehaviourStore::removeBehaviour(uint8_t index){
    if(index >= MaxBehaviours){
        return ERR_WRONG_PARAMETER;
    } else if (!activeBehaviours[index].has_value()){
        LOGw("ERR_NOT_FOUND tried removing empty slot in behaviourstore");
        return ERR_SUCCESS;
    }
    
    activeBehaviours[index].reset();
    return ERR_SUCCESS;
}

ErrorCodesGeneral BehaviourStore::saveBehaviour(Behaviour b, uint8_t index){
    if(index >= MaxBehaviours){
        return ERR_WRONG_PARAMETER;
    }
    
    LOGd("save behaviour to index %d",index);

    activeBehaviours[index] = b;
    return ERR_SUCCESS;
}

uint32_t BehaviourStore::masterHash(){
    uint32_t fletch = 0;

    for(uint8_t i = 0; i < MaxBehaviours; i++){
        // append index as uint16_t to hash data
        fletch = Fletcher(&i,sizeof(i), fletch); // Fletcher() will padd i to the correct width for us.

        // append behaviour or empty array to hash data
        if(activeBehaviours[i]){
            fletch = Fletcher(activeBehaviours[i]->serialize().data(),sizeof(Behaviour::SerializedDataFormat), fletch);
        } else {
            Behaviour::SerializedDataFormat zeroes = {0};
            fletch = Fletcher(zeroes.data(), sizeof(Behaviour::SerializedDataFormat), fletch);
        }
    }

    return fletch;
}
