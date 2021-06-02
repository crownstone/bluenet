/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sept 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_BehaviourStore.h>

#include <algorithm>
#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <events/cs_EventListener.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <util/cs_Hash.h>
#include <util/cs_WireFormat.h>

#define LOGBehaviourStoreInfo LOGi
#define LOGBehaviourStoreDebug LOGd

// allocate space for the behaviours.
std::array<Behaviour*, BehaviourStore::MaxBehaviours> BehaviourStore::activeBehaviours = {};

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
	uint8_t result_index = 0xFF;

	// Check if the behaviour is already there.
	for (uint8_t index = 0; index < MaxBehaviours; ++index) {
		if (activeBehaviours[index] != nullptr &&
				evt.size == activeBehaviours[index]->serializedSize() &&
				memcmp(evt.getData(), activeBehaviours[index]->serialized().data(), evt.size) == 0) {
			LOGBehaviourStoreInfo("Behaviour already exists at ind=%u", index);
			result_index = index;
			evt.result.returnCode = ERR_SUCCESS;
			break;
		}
	}
	if (evt.result.returnCode != ERR_SUCCESS) {
		evt.result.returnCode = addBehaviour(evt.getData(), evt.size, result_index);
	}

	// Fill return buffer if it's large enough.
	if (evt.result.buf.data != nullptr && evt.result.buf.len >= sizeof(uint8_t) + sizeof(uint32_t)) {
		*reinterpret_cast<uint8_t*>( evt.result.buf.data + 0) = result_index;
		*reinterpret_cast<uint32_t*>(evt.result.buf.data + 1) = calculateMasterHash();
		evt.result.dataSize = sizeof(uint8_t) + sizeof(uint32_t);
	}
}

ErrorCodesGeneral BehaviourStore::addBehaviour(uint8_t* buf, cs_buffer_size_t bufSize, uint8_t & index) {
	if (bufSize < 1) {
		LOGe(FMT_ZERO_PAYLOAD_LENGTH, bufSize);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	Behaviour::Type typ = static_cast<Behaviour::Type>(buf[0]);

	// find the first empty index.
	uint8_t empty_index = 0;
	while (activeBehaviours[empty_index] != nullptr && empty_index < MaxBehaviours) {
		empty_index++;
	}
	if (empty_index >= MaxBehaviours) {
		return ERR_NO_SPACE;
	}
	LOGBehaviourStoreInfo("Add behaviour of type %u to index %u", typ, empty_index);
	switch (typ) {
		case SwitchBehaviour::Type::Switch: {
			if (bufSize != WireFormat::size<SwitchBehaviour>()) {
				LOGe(FMT_WRONG_PAYLOAD_LENGTH " while type of behaviour to save: type (%d)",
						bufSize,
						WireFormat::size<SwitchBehaviour>(),
						typ
						);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			LOGBehaviourStoreDebug("Allocating new SwitchBehaviour");
			// no need to delete previous entry, already checked for nullptr
			activeBehaviours[empty_index] = new SwitchBehaviour(WireFormat::deserialize<SwitchBehaviour>(buf, bufSize));
			activeBehaviours[empty_index]->print();

			cs_state_data_t data(CS_TYPE::STATE_BEHAVIOUR_RULE, empty_index, buf, bufSize);
			State::getInstance().set(data);
			index = empty_index;
			storeMasterHash();
			return ERR_SUCCESS;
		}
		case SwitchBehaviour::Type::Twilight: {// check size
			if (bufSize != WireFormat::size<TwilightBehaviour>()) {
				LOGe(FMT_WRONG_PAYLOAD_LENGTH " while type of behaviour to save: type (%d)",
						bufSize,
						WireFormat::size<TwilightBehaviour>(),
						typ
						);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			LOGBehaviourStoreDebug("Allocating new TwilightBehaviour");
			// no need to delete previous entry, already checked for nullptr
			activeBehaviours[empty_index] = new TwilightBehaviour(WireFormat::deserialize<TwilightBehaviour>(buf, bufSize));
			activeBehaviours[empty_index]->print();

			cs_state_data_t data (CS_TYPE::STATE_TWILIGHT_RULE, empty_index, buf, bufSize);
			State::getInstance().set(data);
			index = empty_index;
			storeMasterHash();
			return ERR_SUCCESS;
		}
		case SwitchBehaviour::Type::Extended: {
			if (bufSize != WireFormat::size<ExtendedSwitchBehaviour>()) {
				LOGe(FMT_WRONG_PAYLOAD_LENGTH " while type of behaviour to save: type (%d)",
						bufSize,
						WireFormat::size<ExtendedSwitchBehaviour>(),
						typ
						);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			LOGBehaviourStoreDebug("Allocating new ExtendedSwitchBehaviour");
			// no need to delete previous entry, already checked for nullptr
			activeBehaviours[empty_index] = new ExtendedSwitchBehaviour(WireFormat::deserialize<ExtendedSwitchBehaviour>(buf, bufSize));
			activeBehaviours[empty_index]->print();

			cs_state_data_t data(CS_TYPE::STATE_EXTENDED_BEHAVIOUR_RULE, empty_index, buf, bufSize);
			State::getInstance().set(data);
			index = empty_index;
			storeMasterHash();
			return ERR_SUCCESS;
		}
		default: {
			LOGe("Invalid behaviour type: %d", typ);
			return ERR_WRONG_PARAMETER;
		}
	}
}

bool BehaviourStore::ReplaceParameterValidation(event_t& evt, uint8_t index, const size_t& behaviourSize) {
	const uint8_t indexSize = sizeof(uint8_t);
	TYPIFY(STATE_BEHAVIOUR_MASTER_HASH) hash = calculateMasterHash();
	State::getInstance().set(CS_TYPE::STATE_BEHAVIOUR_MASTER_HASH, &hash, sizeof(hash));

	// check size
	if (evt.size != indexSize + behaviourSize) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH " in replace switchbehaviour",
				evt.size,
				(indexSize + behaviourSize)
			);
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
	const uint8_t typeSize = sizeof(uint8_t);

	if (evt.size < indexSize + typeSize) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH,
				evt.size,
				(indexSize + typeSize)
			);
		evt.result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	uint8_t* dat = static_cast<uint8_t*>(evt.data);
	uint8_t index = dat[0];
	Behaviour::Type type = static_cast<Behaviour::Type>(dat[indexSize]);

	LOGBehaviourStoreInfo("Replace behaviour at ind=%u, type=%u", index, static_cast<uint8_t>(type));

	switch (type) {
		case Behaviour::Type::Switch: {
			if (!ReplaceParameterValidation(evt, index, WireFormat::size<SwitchBehaviour>())) {
				break;
			}

			removeBehaviour(index);

			LOGBehaviourStoreDebug("Allocating new SwitchBehaviour");
			activeBehaviours[index] = new SwitchBehaviour(WireFormat::deserialize<SwitchBehaviour>(evt.getData() + indexSize, evt.size - indexSize));
			activeBehaviours[index]->print();

			cs_state_data_t data (CS_TYPE::STATE_BEHAVIOUR_RULE, index, evt.getData() + indexSize, evt.size - indexSize);
			State::getInstance().set(data);
			storeMasterHash();
			evt.result.returnCode = ERR_SUCCESS;
			break;
		}
		case Behaviour::Type::Twilight: {
			if (!ReplaceParameterValidation(evt, index, WireFormat::size<TwilightBehaviour>())) {
				break;
			}

			removeBehaviour(index);

			LOGBehaviourStoreDebug("Allocating new TwilightBehaviour");
			activeBehaviours[index] = new TwilightBehaviour( WireFormat::deserialize<TwilightBehaviour>(evt.getData() + indexSize, evt.size - indexSize));
			activeBehaviours[index]->print();

			cs_state_data_t data (CS_TYPE::STATE_TWILIGHT_RULE, index, evt.getData() + indexSize, evt.size - indexSize);
			State::getInstance().set(data);
			storeMasterHash();
			evt.result.returnCode = ERR_SUCCESS;

			break;
		}
		case Behaviour::Type::Extended: {
			if (!ReplaceParameterValidation(evt, index, WireFormat::size<ExtendedSwitchBehaviour>())) {
				break;
			}

			removeBehaviour(index);

			LOGBehaviourStoreDebug("Allocating new SwitchBehaviour");
			activeBehaviours[index] = new ExtendedSwitchBehaviour(WireFormat::deserialize<ExtendedSwitchBehaviour>(evt.getData() + indexSize, evt.size - indexSize));
			activeBehaviours[index]->print();

			cs_state_data_t data (CS_TYPE::STATE_EXTENDED_BEHAVIOUR_RULE, index, evt.getData() + indexSize, evt.size - indexSize);
			State::getInstance().set(data);
			storeMasterHash();
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
		evt.result.buf.data[0] = index;
		*reinterpret_cast<uint32_t*>(evt.result.buf.data + sizeof(uint8_t)) = calculateMasterHash();
		evt.result.dataSize = sizeof(uint8_t) + sizeof(uint32_t);
	}
}

void BehaviourStore::handleRemoveBehaviour(event_t& evt) {
	uint8_t index = *reinterpret_cast<TYPIFY(CMD_REMOVE_BEHAVIOUR)*>(evt.data);
	LOGBehaviourStoreInfo("Remove behaviour %u", index);

	evt.result.returnCode = removeBehaviour(index);

	// Fill return buffer if it's large enough.
	if (evt.result.buf.data != nullptr && evt.result.buf.len >= sizeof(uint8_t) + sizeof(uint32_t)) {
		evt.result.buf.data[0] = index;
		*reinterpret_cast<uint32_t*>(evt.result.buf.data + sizeof(uint8_t)) = calculateMasterHash();
		evt.result.dataSize = sizeof(uint8_t) + sizeof(uint32_t);
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
	evt.result.dataSize = sizeof(uint8_t) + activeBehaviours[index]->serializedSize();

	evt.result.buf.data[0] = index;
	activeBehaviours[index]->serialize(evt.result.buf.data + 1, evt.result.buf.len -1);

	evt.result.returnCode = ERR_SUCCESS;
}

void BehaviourStore::handleGetBehaviourIndices(event_t& evt) {
	LOGBehaviourStoreDebug("handle EVT_GET_BEHAVIOUR_INDICES");
	if (evt.result.buf.data == nullptr) {
		LOGw("ERR_BUFFER_UNASSIGNED");
		evt.result.returnCode = ERR_BUFFER_UNASSIGNED;
		return;
	}

	if ( evt.result.buf.len < MaxBehaviours * (sizeof(uint8_t) + sizeof(uint32_t)) ) {
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
					Fletcher(
							activeBehaviours[i]->serialized().data(),
							activeBehaviours[i]->serializedSize());
			listSize += sizeof(uint32_t);

			LOGBehaviourStoreDebug("behaviour found at index %d", i);
		}
	}
	if (listSize == 0) {
		LOGBehaviourStoreInfo("No behaviour found");
	}
	evt.result.dataSize = listSize;
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
		case Behaviour::Type::Switch:
			State::getInstance().remove(CS_TYPE::STATE_BEHAVIOUR_RULE, index);
			break;
		case Behaviour::Type::Twilight:
			State::getInstance().remove(CS_TYPE::STATE_TWILIGHT_RULE, index);
			break;
		case Behaviour::Type::Extended:
			State::getInstance().remove(CS_TYPE::STATE_EXTENDED_BEHAVIOUR_RULE, index);
			break;
		default:
			LOGw("Unknown type: %u", type);
	}
	storeMasterHash();
	return ERR_SUCCESS;
}

uint32_t BehaviourStore::calculateMasterHash() {
	uint32_t fletch = 0;
	for (uint8_t i = 0; i < MaxBehaviours; i++) {
		if (activeBehaviours[i]) {
			// append index as uint16_t to hash data
			fletch = Fletcher(&i, sizeof(i), fletch); // Fletcher() will padd i to the correct width for us.
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

template<class BehaviourType>
void BehaviourStore::LoadBehavioursFromMemory(CS_TYPE BehaviourCsType) {
	cs_ret_code_t retCode;
	std::vector<cs_state_id_t> *ids = nullptr;

	retCode = State::getInstance().getIds(BehaviourCsType, ids);

	if (retCode == ERR_SUCCESS) {
		size16_t data_size = WireFormat::size<BehaviourType>();
		uint8_t data_array[data_size];
		for (auto iter: *ids) {
			if (iter >= MaxBehaviours) {
				LOGw("Invalid ind: %u", iter);
				continue;
			}

			cs_state_data_t data (BehaviourCsType, iter, data_array, data_size);
			retCode = State::getInstance().get(data);

			if (retCode == ERR_SUCCESS) {
				if (activeBehaviours[iter] != nullptr) {
					LOGw("Overwrite ind=%u", iter);
					delete activeBehaviours[iter];
				}
				activeBehaviours[iter] = new BehaviourType(WireFormat::deserialize<BehaviourType>(data_array, data_size));
				LOGBehaviourStoreInfo("Loaded behaviour at ind=%u:", iter);
//				activeBehaviours[iter]->print();
			}
		}
		storeMasterHash();
	}
}

void BehaviourStore::init() {
	// load rules from flash
	LoadBehavioursFromMemory<SwitchBehaviour>(CS_TYPE::STATE_BEHAVIOUR_RULE);
	LoadBehavioursFromMemory<TwilightBehaviour>(CS_TYPE::STATE_TWILIGHT_RULE);
	LoadBehavioursFromMemory<ExtendedSwitchBehaviour>(CS_TYPE::STATE_EXTENDED_BEHAVIOUR_RULE);
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
