/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_BehaviourStore.h>

#include <algorithm>
#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <events/cs_EventListener.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <util/cs_Hash.h>
#include <util/cs_WireFormat.h>

// allocate space for the behaviours.
std::array<Behaviour*,BehaviourStore::MaxBehaviours> BehaviourStore::activeBehaviours = {};

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
            break;
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
    uint8_t result_index = 0xff;

    // find the first empty index.
    uint8_t empty_index = 0;
    while(activeBehaviours[empty_index] != nullptr && empty_index < MaxBehaviours){
        empty_index++;
    }

    if(empty_index >= MaxBehaviours){
        evt.result.returnCode = ERR_NO_SPACE;
    } else {
        result_index = empty_index;

        LOGd("resolving save Behaviour event to index %d",result_index);
        switch(typ){
            case SwitchBehaviour::Type::Switch:{
                // check size
                if(evt.size != WireFormat::size<SwitchBehaviour>()){
                    LOGe(FMT_WRONG_PAYLOAD_LENGTH " while type of behaviour to save: type (%d), expected size(%d)", 
                        evt.size, typ, WireFormat::size<SwitchBehaviour>());
                    evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
                    return;
                }

                LOGd("save behaviour event");
                LOGd("Allocating new SwitchBehaviour");
                // save the new behaviour
                // no need to delete previous entry, already checked for nullptr
                activeBehaviours[result_index] = new SwitchBehaviour(WireFormat::deserialize<SwitchBehaviour>(evt.getData(), evt.size));
                activeBehaviours[result_index]->print();

				cs_state_data_t data(CS_TYPE::STATE_BEHAVIOUR_RULE, result_index, evt.getData(), evt.size);
				State::getInstance().set(data);

                evt.result.returnCode = ERR_SUCCESS;

                break;
            }
            case SwitchBehaviour::Type::Twilight:{// check size
                if(evt.size != WireFormat::size<TwilightBehaviour>()){
                    LOGe(FMT_WRONG_PAYLOAD_LENGTH " while type of behaviour to save: type (%d), expected size(%d)", 
                        evt.size, typ, WireFormat::size<TwilightBehaviour>());
                    evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
                    return;
                }

                LOGd("save behaviour event");
                LOGd("Allocating new TwilightBehaviour");
                // save the new behaviour
                // no need to delete previous entry, already checked for nullptr
                activeBehaviours[result_index] = new TwilightBehaviour(WireFormat::deserialize<TwilightBehaviour>(evt.getData(), evt.size));
                activeBehaviours[result_index]->print();
                
				cs_state_data_t data (CS_TYPE::STATE_TWILIGHT_RULE, result_index, evt.getData(), evt.size);
				State::getInstance().set(data);

                evt.result.returnCode = ERR_SUCCESS;

                break;
            }
            case SwitchBehaviour::Type::Extended:{
                LOGe("Not implemented! Save extended behaviour");
                evt.result.returnCode = ERR_WRONG_PARAMETER;
                break;
            }
            default:{
                LOGe("Invalid behaviour type: %d", typ);
                evt.result.returnCode = ERR_WRONG_PARAMETER;
                break;
            }
        }
    }

    // check return buffer
    if(evt.result.buf.data == nullptr || evt.result.buf.len < sizeof(uint8_t) + sizeof(uint32_t)) {
        LOGd("ERR_BUFFER_TOO_SMALL");
        evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
        return;
    }

    // return value can always be used.
    *reinterpret_cast<uint8_t*>( evt.result.buf.data + 0) = result_index;
    *reinterpret_cast<uint32_t*>(evt.result.buf.data + 1) = masterHash();
    evt.result.dataSize = sizeof(uint8_t) + sizeof(uint32_t);
}

void BehaviourStore::handleReplaceBehaviour(event_t& evt){
	if(evt.size < 2){
		LOGe(FMT_WRONG_PAYLOAD_LENGTH " %d", evt.size);
		evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
        return;
	}

    uint8_t* dat = static_cast<uint8_t*>(evt.data);
	
    uint8_t index = dat[0];
	SwitchBehaviour::Type type = static_cast<SwitchBehaviour::Type>(dat[1]);

	switch(type){
		case SwitchBehaviour::Type::Switch:{
			// check size
			if(evt.size -1 != WireFormat::size<SwitchBehaviour>()){
				LOGe("replace switchbehaviour received wrong size event (%d != %d)",
                    evt.size, 1 + WireFormat::size<SwitchBehaviour>());
				evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
                break;
			}

            // check parameter
            if(index >= MaxBehaviours){
                evt.result.returnCode = ERR_WRONG_PARAMETER;
                break;
            }

            // check return buffer
            if(evt.result.buf.data == nullptr || evt.result.buf.len < sizeof(uint8_t) + sizeof(uint32_t)) {
                LOGd("ERR_BUFFER_TOO_SMALL");
                evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
                break;
            }

            // check previous data
            if(activeBehaviours[index] != nullptr){
                LOGd("deleting previous occupant of index %d", index);
                delete activeBehaviours[index];
                activeBehaviours[index] = nullptr;
            }

			// TODO @Arend, here is +1 / - 1 used... Are you sure
            LOGd("Allocating new SwitchBehaviour");
			activeBehaviours[index] = new SwitchBehaviour(WireFormat::deserialize<SwitchBehaviour>(evt.getData() + 1, evt.size - 1));
            activeBehaviours[index]->print();
				
			// TODO @Arend, also adjust here...
			cs_state_data_t data (CS_TYPE::STATE_BEHAVIOUR_RULE, index, evt.getData() + 1, evt.size - 1);
			State::getInstance().set(data);
            
            evt.result.returnCode = ERR_SUCCESS;
            break;
		}
		case SwitchBehaviour::Type::Twilight:{
            // check size
			if(evt.size -1 != WireFormat::size<TwilightBehaviour>()){
				LOGe("replace twilightbehaviour received wrong size event (%d != %d)", 
                    evt.size, 1 + WireFormat::size<TwilightBehaviour>());
				evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
                break;
			}

            // check parameter
            if(index >= MaxBehaviours){
                evt.result.returnCode = ERR_WRONG_PARAMETER;
                break;
            } 
            
            // check previous data
            if(activeBehaviours[index] != nullptr){
                LOGd("deleting previous occupant of index %d", index);
                delete activeBehaviours[index];
                activeBehaviours[index] = nullptr;

            }

            LOGd("Allocating new TwilightBehaviour");
			// TODO @Arend, also adjust here...
            activeBehaviours[index] = new TwilightBehaviour( WireFormat::deserialize<TwilightBehaviour>(evt.getData() + 1, evt.size - 1));
            activeBehaviours[index]->print();
			
			// TODO @Arend, also adjust here...
			cs_state_data_t data (CS_TYPE::STATE_BEHAVIOUR_RULE, index, evt.getData() + 1, evt.size - 1);
			State::getInstance().set(data);
            
			evt.result.returnCode = ERR_SUCCESS;

			break;
		}
		case SwitchBehaviour::Type::Extended:{
			LOGe("Note implemented: save extended behaviour");
			evt.result.returnCode = ERR_WRONG_PARAMETER;
            break;
		}
		default:{
			LOGe("Invalid behaviour type");
            evt.result.returnCode = ERR_WRONG_PARAMETER;
			break;
		}
	}

    // can always add a return value
    evt.result.buf.data[0] = index;
    *reinterpret_cast<uint32_t*>(evt.result.buf.data + sizeof(uint8_t)) = masterHash();
    evt.result.dataSize = sizeof(uint8_t) + sizeof(uint32_t);
}

void BehaviourStore::handleRemoveBehaviour(event_t& evt){
    uint8_t index = *reinterpret_cast<TYPIFY(EVT_REMOVE_BEHAVIOUR)*>(evt.data);
    LOGd("remove behaviour event %d", index);
    
    evt.result.returnCode = removeBehaviour(index);
			
	State::getInstance().remove(CS_TYPE::STATE_BEHAVIOUR_RULE, index);
    
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

    // validate behaviour index
    if(index >= MaxBehaviours || activeBehaviours[index] == nullptr){
        LOGd("ERR_NOT_FOUND");
        evt.result.returnCode = ERR_NOT_FOUND;
        return;
    }

    activeBehaviours[index]->print();

    // validate buffer
    if(evt.result.buf.data == nullptr) {
        LOGd("ERR_BUFFER_UNASSIGNED");
        evt.result.returnCode = ERR_BUFFER_UNASSIGNED;
        return;
    }

    // validate size
    if(evt.result.buf.len < sizeof(uint8_t) + activeBehaviours[index]->serializedSize()){
        // cannot communicate the result, so won't do anything.
        LOGd("ERR_BUFFER_TOO_SMALL");
        evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
        return;
    }

    // populate response
    evt.result.dataSize = sizeof(uint8_t) + activeBehaviours[index]->serializedSize();

    evt.result.buf.data[0] = index;
    activeBehaviours[index]->serialize(evt.result.buf.data + 1, evt.result.buf.len -1);
   
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
        if (activeBehaviours[i] != nullptr) {
            evt.result.buf.data[listSize] = i;
            listSize += sizeof(uint8_t);

            *reinterpret_cast<uint32_t*>(evt.result.buf.data + listSize) = 
                Fletcher(
                    activeBehaviours[i]->serialized().data(),
                    activeBehaviours[i]->serializedSize());
            listSize += sizeof(uint32_t);

            LOGd("behaviour found at index %d",i);
        }
    }
	if (listSize == 0) {
		LOGi("No behaviour found");
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
    } 
    
    if (activeBehaviours[index] == nullptr){
        LOGw("ERR_NOT_FOUND tried removing empty slot in behaviourstore");
        return ERR_SUCCESS;
    }
    
    if(activeBehaviours[index] != nullptr){
        LOGd("deleting behaviour");
        delete activeBehaviours[index];
        activeBehaviours[index] = nullptr;
    }

    return ERR_SUCCESS;
}

// ErrorCodesGeneral BehaviourStore::saveBehaviour(SwitchBehaviour b, uint8_t index){
//     if(index >= MaxBehaviours){
//         return ERR_WRONG_PARAMETER;
//     }
    
//     LOGd("save behaviour to index %d",index);

//     activeBehaviours[index] = b;
//     return ERR_SUCCESS;
// }

uint32_t BehaviourStore::masterHash() {
    uint32_t fletch = 0;
    for (uint8_t i = 0; i < MaxBehaviours; i++) {
        if (activeBehaviours[i]) {
        	// append index as uint16_t to hash data
        	fletch = Fletcher(&i,sizeof(i), fletch); // Fletcher() will padd i to the correct width for us.
        	// append behaviour to hash data
            fletch = Fletcher(activeBehaviours[i]->serialized().data(), activeBehaviours[i]->serializedSize(), fletch);
        }
    }
    return fletch;
}

void BehaviourStore::init() {
	// load rules from flash
	std::vector<cs_state_id_t> *ids;
	State::getInstance().getIds(CS_TYPE::STATE_BEHAVIOUR_RULE, ids);
	cs_ret_code_t retCode;
	if (ids != NULL) {
		size16_t data_size = WireFormat::size<SwitchBehaviour>();
		uint8_t data_array[data_size];
		for (auto iter: *ids) {
			cs_state_data_t data (CS_TYPE::STATE_BEHAVIOUR_RULE, iter, data_array, data_size);
			retCode = State::getInstance().get(data);
			if (retCode == ERR_SUCCESS) {
				activeBehaviours[iter] = new SwitchBehaviour(WireFormat::deserialize<SwitchBehaviour>(data_array, data_size));
				LOGi("Loaded behaviour at ind=%u:", iter);
				activeBehaviours[iter]->print();
			}
		}
	}
	ids = NULL;
	State::getInstance().getIds(CS_TYPE::STATE_TWILIGHT_RULE, ids);
	if (ids != NULL) {
		size16_t data_size = WireFormat::size<TwilightBehaviour>();
		uint8_t data_array[data_size];
		for (auto iter: *ids) {
			cs_state_data_t data (CS_TYPE::STATE_TWILIGHT_RULE, iter, data_array, data_size);
			retCode = State::getInstance().get(data);
			if (retCode == ERR_SUCCESS) {
				activeBehaviours[iter] = new TwilightBehaviour(WireFormat::deserialize<TwilightBehaviour>(data_array, data_size));
				LOGi("Loaded behaviour at ind=%u:", iter);
				activeBehaviours[iter]->print();
			}
		}
	}
}

BehaviourStore::~BehaviourStore(){
    for(Behaviour* bptr: activeBehaviours){
        if(bptr != nullptr){
            delete bptr;
        };
    }
}
