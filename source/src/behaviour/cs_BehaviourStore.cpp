/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_BehaviourStore.h>
#include <common/cs_Types.h>
#include <events/cs_EventListener.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <util/cs_Hash.h>
#include <util/cs_WireFormat.h>

#include <algorithm>

#define LOGBehaviourStoreInfo LOGi
#define LOGBehaviourStoreDebug LOGd

void BehaviourStore::handleEvent(event_t& evt) {
	switch (evt.type) {
		case CS_TYPE::CMD_ADD_BEHAVIOUR: {
			handleSaveBehaviour(evt);
			dispatchBehaviourMutationEvent();
			break;
		}
		case CS_TYPE::CMD_REPLACE_BEHAVIOUR: {
			handleReplaceBehaviour(evt);
			dispatchBehaviourMutationEvent();
			break;
		}
		case CS_TYPE::CMD_REMOVE_BEHAVIOUR: {
			handleRemoveBehaviour(evt);
			dispatchBehaviourMutationEvent();
			break;
		}
		case CS_TYPE::CMD_GET_BEHAVIOUR: {
			handleGetBehaviour(evt);
			break;
		}
		case CS_TYPE::CMD_GET_BEHAVIOUR_INDICES: {
			handleGetBehaviourIndices(evt);
			break;
		}
		case CS_TYPE::CMD_CLEAR_ALL_BEHAVIOUR: {
			clearActiveBehavioursArray();
			break;
		}
		default: {
			break;
		}
	}
}

// ==================== handler functions ====================

void BehaviourStore::handleSaveBehaviour(event_t& evt) {
	evt.result.returnCode = ERR_UNSPECIFIED;
	uint8_t result_index  = 0xFF;

	// Check if the behaviour is already there.
	for (uint8_t index = 0; index < MaxBehaviours; ++index) {
		if (activeBehaviours[index] != nullptr && evt.size == activeBehaviours[index]->serializedSize()
			&& memcmp(evt.getData(), activeBehaviours[index]->serialized().data(), evt.size) == 0) {
			LOGBehaviourStoreInfo("Behaviour already exists at ind=%u", index);
			result_index          = index;
			evt.result.returnCode = ERR_SUCCESS;
			break;
		}
	}
	if (evt.result.returnCode != ERR_SUCCESS) {
		evt.result.returnCode = addBehaviour(evt.getData(), evt.size, result_index);
	}

	// Fill return buffer if it's large enough.
	if (evt.result.buf.data != nullptr && evt.result.buf.len >= sizeof(uint8_t) + sizeof(uint32_t)) {
		*reinterpret_cast<uint8_t*>(evt.result.buf.data + 0)  = result_index;
		*reinterpret_cast<uint32_t*>(evt.result.buf.data + 1) = calculateMasterHash();
		evt.result.dataSize                                   = sizeof(uint8_t) + sizeof(uint32_t);
	}
}



void BehaviourStore::StoreUpdate(uint8_t index, SwitchBehaviour::Type type, uint8_t* buf, cs_buffer_size_t bufSize) {
	CS_TYPE csType;
	switch(type) {
		case SwitchBehaviour::Type::Switch: csType = CS_TYPE::STATE_BEHAVIOUR_RULE; break;
		case SwitchBehaviour::Type::Twilight: csType = CS_TYPE::STATE_TWILIGHT_RULE; break;
		case SwitchBehaviour::Type::Extended: csType = CS_TYPE::STATE_EXTENDED_BEHAVIOUR_RULE; break;
		default: return;
	}

	cs_state_data_t data(csType, index, buf, bufSize);
	State::getInstance().set(data);
	storeMasterHash();
}

size_t BehaviourStore::getBehaviourSize(SwitchBehaviour::Type type) {
	switch (type) {
		case SwitchBehaviour::Type::Switch: return WireFormat::size<SwitchBehaviour>();
		case SwitchBehaviour::Type::Twilight: return WireFormat::size<TwilightBehaviour>();
		case SwitchBehaviour::Type::Extended: return WireFormat::size<ExtendedSwitchBehaviour>();
		default: {
			LOGe("Invalid behaviour type: %d", type);
			return 0;
		}
	}
}

ErrorCodesGeneral BehaviourStore::checkSizeAndType(SwitchBehaviour::Type type, cs_buffer_size_t bufSize) {
	size_t expectedSize = getBehaviourSize(type);
	if(expectedSize == 0) {
		return ERR_WRONG_PARAMETER;
	}

	if (bufSize != expectedSize) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH " while type of behaviour to save: type (%d)", bufSize, expectedSize, type);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	return ERR_SUCCESS;
}

void BehaviourStore::allocateBehaviour(uint8_t index, SwitchBehaviour::Type type, uint8_t* buf, cs_buffer_size_t bufSize) {
	Behaviour* behaviour = nullptr;
	switch (type) {
		case SwitchBehaviour::Type::Switch: {
			LOGBehaviourStoreDebug("Allocating new SwitchBehaviour");
			behaviour = new SwitchBehaviour(WireFormat::deserialize<SwitchBehaviour>(buf, bufSize));
			break;
		}
		case SwitchBehaviour::Type::Twilight: {
			LOGBehaviourStoreDebug("Allocating new TwilightBehaviour");
			behaviour = new TwilightBehaviour(WireFormat::deserialize<TwilightBehaviour>(buf, bufSize));
			break;
		}
		case SwitchBehaviour::Type::Extended: {
			LOGBehaviourStoreDebug("Allocating new ExtendedSwitchBehaviour");
			behaviour = new ExtendedSwitchBehaviour(WireFormat::deserialize<ExtendedSwitchBehaviour>(buf, bufSize));
			break;
		}
		default: return;
	}

	// no need to delete previous entry, already checked for nullptr
	activeBehaviours[index] = behaviour;
	activeBehaviours[index]->print();
}

ErrorCodesGeneral BehaviourStore::addBehaviour(uint8_t* buf, cs_buffer_size_t bufSize, uint8_t& index) {
	if (bufSize < 1) {
		LOGe(FMT_ZERO_PAYLOAD_LENGTH, bufSize);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	Behaviour::Type typ = static_cast<Behaviour::Type>(buf[0]);

	auto retval = checkSizeAndType(typ, bufSize);
	if (retval != ERR_SUCCESS) {
		return retval;
	}

	// find the first empty index.
	uint8_t empty_index = 0;
	while (activeBehaviours[empty_index] != nullptr && empty_index < MaxBehaviours) {
		empty_index++;
	}
	if (empty_index >= MaxBehaviours) {
		return ERR_NO_SPACE;
	}
	LOGBehaviourStoreInfo("Add behaviour of type %u to index %u", typ, empty_index);

	allocateBehaviour(empty_index, typ, buf, bufSize);
	StoreUpdate(empty_index, typ, buf, bufSize);
	index = empty_index;
	return ERR_SUCCESS;
}

bool BehaviourStore::ReplaceParameterValidation(event_t& evt, uint8_t index, SwitchBehaviour::Type type) {
	size_t behaviourSize = getBehaviourSize(type);

	if(behaviourSize == 0) {
		LOGe("Invalid behaviour type");
		evt.result.returnCode = ERR_WRONG_PARAMETER;
		return false;
	}

	const uint8_t indexSize                  = sizeof(uint8_t);
	TYPIFY(STATE_BEHAVIOUR_MASTER_HASH) hash = calculateMasterHash();
	State::getInstance().set(CS_TYPE::STATE_BEHAVIOUR_MASTER_HASH, &hash, sizeof(hash));

	// check size
	if (evt.size != indexSize + behaviourSize) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH " in replace switchbehaviour", evt.size, (indexSize + behaviourSize));
		evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return false;
	}

	// check parameter
	if (index >= MaxBehaviours) {
		evt.result.returnCode = ERR_WRONG_PARAMETER;
		return false;
	}

	return true;
}


void BehaviourStore::handleReplaceBehaviour(event_t& evt) {
	const uint8_t indexSize = sizeof(uint8_t);
	const uint8_t typeSize  = sizeof(uint8_t);

	if (evt.size < indexSize + typeSize) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, evt.size, (indexSize + typeSize));
		evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	uint8_t* dat         = static_cast<uint8_t*>(evt.data);
	uint8_t index        = dat[0];
	Behaviour::Type type = static_cast<Behaviour::Type>(dat[indexSize]);

	LOGBehaviourStoreInfo("Replace behaviour at ind=%u, type=%u", index, static_cast<uint8_t>(type));

	switch (type) {
		case Behaviour::Type::Switch: {
			if (!ReplaceParameterValidation(evt, index, type)) {
				break;
			}

			removeBehaviour(index);
			allocateBehaviour(index, type, dat + indexSize, evt.size - indexSize);
			StoreUpdate(index, type, dat + indexSize, evt.size - indexSize);

			evt.result.returnCode = ERR_SUCCESS;
			break;
		}
		case Behaviour::Type::Twilight: {
			if (!ReplaceParameterValidation(evt, index, type)) {
				break;
			}

			removeBehaviour(index);
			allocateBehaviour(index, type, dat + indexSize, evt.size - indexSize);
			StoreUpdate(index, type, dat + indexSize, evt.size - indexSize);

			evt.result.returnCode = ERR_SUCCESS;

			break;
		}
		case Behaviour::Type::Extended: {
			if (!ReplaceParameterValidation(evt, index, type)) {
				break;
			}

			removeBehaviour(index);
			allocateBehaviour(index, type, dat + indexSize, evt.size - indexSize);
			StoreUpdate(index, type, dat + indexSize, evt.size - indexSize);

			evt.result.returnCode = ERR_SUCCESS;
			break;
		}
		default: {
			LOGe("Invalid behaviour type");
			evt.result.returnCode = ERR_WRONG_PARAMETER;
			break;
		}
	}

	// Fill return buffer if it's large enough.
	if (evt.result.buf.data != nullptr && evt.result.buf.len >= sizeof(uint8_t) + sizeof(uint32_t)) {
		evt.result.buf.data[0]                                              = index;
		*reinterpret_cast<uint32_t*>(evt.result.buf.data + sizeof(uint8_t)) = calculateMasterHash();
		evt.result.dataSize                                                 = sizeof(uint8_t) + sizeof(uint32_t);
	}
}

void BehaviourStore::handleRemoveBehaviour(event_t& evt) {
	uint8_t index = *reinterpret_cast<TYPIFY(CMD_REMOVE_BEHAVIOUR)*>(evt.data);
	LOGBehaviourStoreInfo("Remove behaviour %u", index);

	evt.result.returnCode = removeBehaviour(index);

	// Fill return buffer if it's large enough.
	if (evt.result.buf.data != nullptr && evt.result.buf.len >= sizeof(uint8_t) + sizeof(uint32_t)) {
		evt.result.buf.data[0]                                              = index;
		*reinterpret_cast<uint32_t*>(evt.result.buf.data + sizeof(uint8_t)) = calculateMasterHash();
		evt.result.dataSize                                                 = sizeof(uint8_t) + sizeof(uint32_t);
	}
}

void BehaviourStore::handleGetBehaviour(event_t& evt) {
	uint8_t index = *reinterpret_cast<TYPIFY(CMD_GET_BEHAVIOUR)*>(evt.data);
	LOGBehaviourStoreInfo("Get behaviour %u ", index);

	// validate behaviour index
	if (index >= MaxBehaviours || activeBehaviours[index] == nullptr) {
		LOGBehaviourStoreDebug("ERR_NOT_FOUND");
		evt.result.returnCode = ERR_NOT_FOUND;
		return;
	}

	activeBehaviours[index]->print();

	// validate buffer
	if (evt.result.buf.data == nullptr) {
		LOGw("ERR_BUFFER_UNASSIGNED");
		evt.result.returnCode = ERR_BUFFER_UNASSIGNED;
		return;
	}

	// validate size
	if (evt.result.buf.len < sizeof(uint8_t) + activeBehaviours[index]->serializedSize()) {
		// cannot communicate the result, so won't do anything.
		LOGw("ERR_BUFFER_TOO_SMALL");
		evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}

	// populate response
	evt.result.dataSize    = sizeof(uint8_t) + activeBehaviours[index]->serializedSize();

	evt.result.buf.data[0] = index;
	activeBehaviours[index]->serialize(evt.result.buf.data + 1, evt.result.buf.len - 1);

	evt.result.returnCode = ERR_SUCCESS;
}

void BehaviourStore::handleGetBehaviourIndices(event_t& evt) {
	LOGBehaviourStoreDebug("handle EVT_GET_BEHAVIOUR_INDICES");
	if (evt.result.buf.data == nullptr) {
		LOGw("ERR_BUFFER_UNASSIGNED");
		evt.result.returnCode = ERR_BUFFER_UNASSIGNED;
		return;
	}

	if (evt.result.buf.len < MaxBehaviours * (sizeof(uint8_t) + sizeof(uint32_t))) {
		LOGw("ERR_BUFFER_TOO_SMALL");
		evt.result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}

	size_t listSize = 0;
	for (uint8_t i = 0; i < MaxBehaviours; ++i) {
		if (activeBehaviours[i] != nullptr) {
			evt.result.buf.data[listSize] = i;
			listSize += sizeof(uint8_t);

			*reinterpret_cast<uint32_t*>(evt.result.buf.data + listSize) =
					Fletcher(activeBehaviours[i]->serialized().data(), activeBehaviours[i]->serializedSize());
			listSize += sizeof(uint32_t);

			LOGBehaviourStoreDebug("behaviour found at index %d", i);
		}
	}
	if (listSize == 0) {
		LOGBehaviourStoreInfo("No behaviour found");
	}
	evt.result.dataSize   = listSize;
	evt.result.returnCode = ERR_SUCCESS;
}

void BehaviourStore::dispatchBehaviourMutationEvent() {
	event_t evt(CS_TYPE::EVT_BEHAVIOURSTORE_MUTATION, nullptr, 0);
	evt.dispatch();
}

// ==================== public functions ====================

ErrorCodesGeneral BehaviourStore::removeBehaviour(uint8_t index) {
	if (index >= MaxBehaviours) {
		return ERR_WRONG_PARAMETER;
	}
	if (activeBehaviours[index] == nullptr) {
		LOGBehaviourStoreDebug("Already removed");
		return ERR_SUCCESS;
	}
	auto type = activeBehaviours[index]->getType();

	LOGBehaviourStoreDebug("deleting behaviour");
	delete activeBehaviours[index];
	activeBehaviours[index] = nullptr;

	switch (type) {
		case Behaviour::Type::Switch: State::getInstance().remove(CS_TYPE::STATE_BEHAVIOUR_RULE, index); break;
		case Behaviour::Type::Twilight: State::getInstance().remove(CS_TYPE::STATE_TWILIGHT_RULE, index); break;
		case Behaviour::Type::Extended:
			State::getInstance().remove(CS_TYPE::STATE_EXTENDED_BEHAVIOUR_RULE, index);
			break;
		default: LOGw("Unknown type: %u", type);
	}
	storeMasterHash();
	return ERR_SUCCESS;
}

uint32_t BehaviourStore::calculateMasterHash() {
	uint32_t fletch = 0;
	for (uint8_t i = 0; i < MaxBehaviours; i++) {
		if (activeBehaviours[i]) {
			// append index as uint16_t to hash data
			fletch = Fletcher(&i, sizeof(i), fletch);  // Fletcher() will padd i to the correct width for us.
			// append behaviour to hash data
			fletch = Fletcher(activeBehaviours[i]->serialized().data(), activeBehaviours[i]->serializedSize(), fletch);
		}
	}
	return fletch;
}

void BehaviourStore::storeMasterHash() {
	TYPIFY(STATE_BEHAVIOUR_MASTER_HASH) hash = calculateMasterHash();
	LOGBehaviourStoreDebug("storeMasterHash %u", hash);
	State::getInstance().set(CS_TYPE::STATE_BEHAVIOUR_MASTER_HASH, &hash, sizeof(hash));
}

template <class BehaviourType>
void BehaviourStore::LoadBehavioursFromMemory(CS_TYPE BehaviourCsType) {
	cs_ret_code_t retCode;
	std::vector<cs_state_id_t>* ids = nullptr;

	retCode                         = State::getInstance().getIds(BehaviourCsType, ids);

	if (retCode == ERR_SUCCESS) {
		size16_t data_size = WireFormat::size<BehaviourType>();
		uint8_t data_array[data_size];
		for (auto iter : *ids) {
			if (iter >= MaxBehaviours) {
				LOGw("Invalid ind: %u", iter);
				continue;
			}

			cs_state_data_t data(BehaviourCsType, iter, data_array, data_size);
			retCode = State::getInstance().get(data);

			if (retCode == ERR_SUCCESS) {
				if (activeBehaviours[iter] != nullptr) {
					LOGw("Overwrite ind=%u", iter);
					delete activeBehaviours[iter];
				}
				activeBehaviours[iter] =
						new BehaviourType(WireFormat::deserialize<BehaviourType>(data_array, data_size));
				LOGBehaviourStoreInfo("Loaded behaviour at ind=%u:", iter);
				//				activeBehaviours[iter]->print();
			}
		}
		storeMasterHash();
	}
}

cs_ret_code_t BehaviourStore::init() {
	// load rules from flash
	LoadBehavioursFromMemory<SwitchBehaviour>(CS_TYPE::STATE_BEHAVIOUR_RULE);
	LoadBehavioursFromMemory<TwilightBehaviour>(CS_TYPE::STATE_TWILIGHT_RULE);
	LoadBehavioursFromMemory<ExtendedSwitchBehaviour>(CS_TYPE::STATE_EXTENDED_BEHAVIOUR_RULE);

	return ERR_SUCCESS;
}

void BehaviourStore::clearActiveBehavioursArray() {
	LOGBehaviourStoreInfo("clearing all behaviours");
	for (size_t i = 0; i < MaxBehaviours; i++) {
		removeBehaviour(i);
	}
}

BehaviourStore::~BehaviourStore() {
	clearActiveBehavioursArray();
}
