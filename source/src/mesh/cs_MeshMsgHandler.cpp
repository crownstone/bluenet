/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <events/cs_Event.h>
#include <mesh/cs_MeshCommon.h>
#include <mesh/cs_MeshMsgHandler.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>
#include <storage/cs_State.h>
#include <util/cs_Utils.h>

void MeshMsgHandler::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_ownId, sizeof(_ownId));
}

void MeshMsgHandler::handleMsg(const MeshUtil::cs_mesh_received_msg_t& msg) {
//	BLEutil::printArray(msg.msg, msg.msgSize);
//	if (msg.opCode == CS_MESH_MODEL_OPCODE_RELIABLE_MSG) {
//		LOGe("sendReply");
////		sendReply(accessMsg);
//	}
	if (!MeshUtil::isValidMeshMessage(msg.msg, msg.msgSize)) {
		LOGw("Invalid mesh message");
		BLEutil::printArray(msg.msg, msg.msgSize);
		return;
	}
	cs_mesh_model_msg_type_t msgType = MeshUtil::getType(msg.msg);
	uint8_t* payload;
	size16_t payloadSize;
	MeshUtil::getPayload(msg.msg, msg.msgSize, payload, payloadSize);

	switch (msgType){
		case CS_MESH_MODEL_TYPE_TEST: {
			handleTest(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_ACK: {
			handleAck(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_STATE_TIME: {
			handleStateTime(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_CMD_TIME: {
			handleCmdTime(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_CMD_NOOP: {
			handleCmdNoop(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH: {
			handleCmdMultiSwitch(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_STATE_0: {
			handleState0(payload, payloadSize, msg.srcAddress, msg.rssi, msg.hops);
			break;
		}
		case CS_MESH_MODEL_TYPE_STATE_1: {
			handleState1(payload, payloadSize, msg.srcAddress, msg.rssi, msg.hops);
			break;
		}
		case CS_MESH_MODEL_TYPE_PROFILE_LOCATION: {
			handleProfileLocation(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS: {
			handleSetBehaviourSettings(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_REGISTER: {
			handleTrackedDeviceRegister(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_TOKEN: {
			handleTrackedDeviceToken(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_LIST_SIZE: {
			handleTrackedDeviceListSize(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_SYNC_REQUEST: {
			handleSyncRequest(payload, payloadSize);
			break;
		}
		case CS_MESH_MODEL_TYPE_STATE_SET: {
			handleStateSet(payload, payloadSize);
			break;
		}
	}
}

void MeshMsgHandler::handleTest(uint8_t* payload, size16_t payloadSize) {
	[[maybe_unused]] cs_mesh_model_msg_test_t* test = (cs_mesh_model_msg_test_t*)payload;
	LOGi("received test counter=%u", test->counter);
#if MESH_MODEL_TEST_MSG == 1
	if (_lastReceivedCounter == 0) {
		_lastReceivedCounter = test->counter;
		return;
	}
	if (_lastReceivedCounter == test->counter) {
		LOGi("received same counter");
		return;
	}
	uint32_t expectedCounter = _lastReceivedCounter + 1;
	_lastReceivedCounter = test->counter;
	LOGMeshModelVerbose("receivedCounter=%u expectedCounter=%u", _lastReceivedCounter, expectedCounter);
	if (expectedCounter == _lastReceivedCounter) {
		_received++;
	}
	else if (_lastReceivedCounter < expectedCounter) {
		LOGw("receivedCounter=%u < expectedCounter=%u", _lastReceivedCounter, expectedCounter);
	}
	else {
		_dropped += test->counter - expectedCounter;
		_received++;
	}
	LOGi("received=%u dropped=%u received=%u%%", _received, _dropped, (_received * 100) / (_received + _dropped));
#endif
}

void MeshMsgHandler::handleAck(uint8_t* payload, size16_t payloadSize) {
}

void MeshMsgHandler::handleStateTime(uint8_t* payload, size16_t payloadSize) {
	cs_mesh_model_msg_time_t* packet = (cs_mesh_model_msg_time_t*)payload;
	TYPIFY(EVT_MESH_TIME) timestamp = packet->timestamp;
	LOGMeshModelDebug("received state time %u", timestamp);
	event_t event(CS_TYPE::EVT_MESH_TIME, &timestamp, sizeof(timestamp));
	event.dispatch();
}

void MeshMsgHandler::handleCmdTime(uint8_t* payload, size16_t payloadSize) {
	cs_mesh_model_msg_time_t* packet = (cs_mesh_model_msg_time_t*)payload;
	TYPIFY(CMD_SET_TIME) timestamp = packet->timestamp;
	LOGMeshModelInfo("received set time %u", timestamp);
	if (timestamp != _lastReveivedSetTime) {
		_lastReveivedSetTime = timestamp;
		event_t event(CS_TYPE::CMD_SET_TIME, &timestamp, sizeof(timestamp));
		event.dispatch();
	}
}

void MeshMsgHandler::handleCmdNoop(uint8_t* payload, size16_t payloadSize) {
	LOGMeshModelDebug("received noop");
}

void MeshMsgHandler::handleCmdMultiSwitch(uint8_t* payload, size16_t payloadSize) {
	cs_mesh_model_msg_multi_switch_item_t* item = (cs_mesh_model_msg_multi_switch_item_t*) payload;
	if (item->id == _ownId) {
		LOGMeshModelInfo("received multi switch for me");
		if (memcmp(&_lastReceivedMultiSwitch, item, sizeof(*item)) == 0) {
			LOGMeshModelDebug("ignore similar multi switch");
			return;
		}
		memcpy(&_lastReceivedMultiSwitch, item, sizeof(*item));

		TYPIFY(CMD_MULTI_SWITCH) internalItem;
		internalItem.id = item->id;
		internalItem.cmd.switchCmd = item->switchCmd;
		internalItem.cmd.delay = item->delay;
		internalItem.cmd.source = item->source;
		internalItem.cmd.source.flagExternal = true;

		LOGi("dispatch multi switch");
		event_t event(CS_TYPE::CMD_MULTI_SWITCH, &internalItem, sizeof(internalItem));
		event.dispatch();
	}
}

void MeshMsgHandler::handleState0(uint8_t* payload, size16_t payloadSize, uint16_t srcAddress, int8_t rssi, uint8_t hops) {
	cs_mesh_model_msg_state_0_t* packet = (cs_mesh_model_msg_state_0_t*) payload;
	stone_id_t srcId = srcAddress;
	LOGMeshModelDebug("received: id=%u switch=%u flags=%u powerFactor=%i powerUsage=%i ts=%u", srcId, packet->switchState, packet->flags, packet->powerFactor, packet->powerUsageReal, packet->partialTimestamp);

	// Send event
	TYPIFY(EVT_MESH_EXT_STATE_0)* state = packet;
	event_t event(CS_TYPE::EVT_MESH_EXT_STATE_0, state, sizeof(*state));
	event.dispatch();

	if (!isFromSameState(srcAddress, srcAddress, packet->partialTimestamp)) {
		_lastReceivedState.partsReceivedBitmask = 0;
	}
	_lastReceivedState.address = srcAddress;
	_lastReceivedState.state.data.state.id = srcId;
	_lastReceivedState.state.data.extState.switchState = packet->switchState;
	_lastReceivedState.state.data.extState.flags = packet->flags;
	_lastReceivedState.state.data.extState.powerFactor = packet->powerFactor;
	_lastReceivedState.state.data.extState.powerUsageReal = packet->powerUsageReal;
	_lastReceivedState.state.data.extState.partialTimestamp = packet->partialTimestamp;
	BLEutil::setBit(_lastReceivedState.partsReceivedBitmask, 0);
	checkStateReceived(rssi, hops);
}

void MeshMsgHandler::handleState1(uint8_t* payload, size16_t payloadSize, uint16_t srcAddress, int8_t rssi, uint8_t hops) {
	cs_mesh_model_msg_state_1_t* packet = (cs_mesh_model_msg_state_1_t*) payload;
	stone_id_t srcId = srcAddress;
	LOGMeshModelDebug("received: id=%u temp=%i energy=%i ts=%u", srcId, packet->temperature, packet->energyUsed, packet->partialTimestamp);

	// Send event
	TYPIFY(EVT_MESH_EXT_STATE_1)* state = packet;
	event_t event(CS_TYPE::EVT_MESH_EXT_STATE_1, state, sizeof(*state));
	event.dispatch();

	if (!isFromSameState(srcAddress, srcAddress, packet->partialTimestamp)) {
		_lastReceivedState.partsReceivedBitmask = 0;
	}
	_lastReceivedState.address = srcAddress;
	_lastReceivedState.state.data.state.id = srcId;
	_lastReceivedState.state.data.extState.temperature = packet->temperature;
	_lastReceivedState.state.data.extState.energyUsed = packet->energyUsed;
	_lastReceivedState.state.data.extState.partialTimestamp = packet->partialTimestamp;
	BLEutil::setBit(_lastReceivedState.partsReceivedBitmask, 1);
	checkStateReceived(rssi, hops);
}

bool MeshMsgHandler::isFromSameState(uint16_t srcAddress, stone_id_t id, uint16_t partialTimestamp) {
	return (_lastReceivedState.address == srcAddress
			&& _lastReceivedState.state.data.extState.id == id
			&& _lastReceivedState.state.data.extState.partialTimestamp == partialTimestamp);
}

void MeshMsgHandler::checkStateReceived(int8_t rssi, uint8_t hops) {
	if (_lastReceivedState.partsReceivedBitmask != 0x03) {
		return;
	}
	if (hops == 0) {
		// Maximum ttl, so we received it without hops: RSSI is valid.
		_lastReceivedState.state.rssi = rssi;
		_lastReceivedState.state.data.extState.rssi = 0;
	}
	else {
		// We don't actually know the RSSI to the source of this message.
		// We only get RSSI to a mac address, but we don't know what id a mac address has.
		// So for now, just assume we don't have direct contact with the source stone.
		_lastReceivedState.state.rssi = 0;
		_lastReceivedState.state.data.extState.rssi = 0;
	}
	_lastReceivedState.state.data.extState.validation = SERVICE_DATA_VALIDATION;
	_lastReceivedState.state.data.type = SERVICE_DATA_TYPE_EXT_STATE;
#if CS_SERIAL_NRF_LOG_ENABLED != 2
	LOGMeshModelInfo("combined: id=%u switch=%u flags=%u temp=%i pf=%i power=%i energy=%i ts=%u rssi=%i",
			_lastReceivedState.state.data.extState.id,
			_lastReceivedState.state.data.extState.switchState,
			_lastReceivedState.state.data.extState.flags,
			_lastReceivedState.state.data.extState.temperature,
			_lastReceivedState.state.data.extState.powerFactor,
			_lastReceivedState.state.data.extState.powerUsageReal,
			_lastReceivedState.state.data.extState.energyUsed,
			_lastReceivedState.state.data.extState.partialTimestamp,
			_lastReceivedState.state.data.extState.rssi
	);
#endif
	// Reset parts received
	_lastReceivedState.partsReceivedBitmask = 0;

	// Send event
	TYPIFY(EVT_STATE_EXTERNAL_STONE)* stateExtStone = &(_lastReceivedState.state);
	event_t event(CS_TYPE::EVT_STATE_EXTERNAL_STONE, stateExtStone, sizeof(*stateExtStone));
	event.dispatch();
}

void MeshMsgHandler::handleProfileLocation(uint8_t* payload, size16_t payloadSize) {
	cs_mesh_model_msg_profile_location_t* profileLocation = (cs_mesh_model_msg_profile_location_t*) payload;
	LOGMeshModelDebug("received profile=%u location=%u", profileLocation->profile, profileLocation->location);
	TYPIFY(EVT_PROFILE_LOCATION) eventData;
	eventData.profileId = profileLocation->profile;
	eventData.locationId = profileLocation->location;
	eventData.fromMesh = true;
	event_t event(CS_TYPE::EVT_PROFILE_LOCATION, &eventData, sizeof(eventData));
	event.dispatch();
}

void MeshMsgHandler::handleSetBehaviourSettings(uint8_t* payload, size16_t payloadSize) {
	behaviour_settings_t* packet = (behaviour_settings_t*) payload;
	LOGMeshModelInfo("received behaviour settings %u", packet->asInt);
	TYPIFY(STATE_BEHAVIOUR_SETTINGS)* eventDataPtr = packet;
//	cs_state_data_t stateData(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, (uint8_t*)eventDataPtr, sizeof(TYPIFY(STATE_BEHAVIOUR_SETTINGS)));
//	State::getInstance().set(stateData);
	State::getInstance().set(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, eventDataPtr, sizeof(TYPIFY(STATE_BEHAVIOUR_SETTINGS)));
}

void MeshMsgHandler::handleTrackedDeviceRegister(uint8_t* payload, size16_t payloadSize) {
	cs_mesh_model_msg_device_register_t* packet = (cs_mesh_model_msg_device_register_t*) payload;
	LOGMeshModelDebug("received tracked device register id=%u", packet->deviceId);
	TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)* eventDataPtr = packet;
	event_t event(CS_TYPE::EVT_MESH_TRACKED_DEVICE_REGISTER, eventDataPtr, sizeof(TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)));
	event.dispatch();
}

void MeshMsgHandler::handleTrackedDeviceToken(uint8_t* payload, size16_t payloadSize) {
	cs_mesh_model_msg_device_token_t* packet = (cs_mesh_model_msg_device_token_t*) payload;
	LOGMeshModelDebug("received tracked device token id=%u", packet->deviceId);
	TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)* eventDataPtr = packet;
	event_t event(CS_TYPE::EVT_MESH_TRACKED_DEVICE_TOKEN, eventDataPtr, sizeof(TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)));
	event.dispatch();
}

void MeshMsgHandler::handleTrackedDeviceListSize(uint8_t* payload, size16_t payloadSize) {
	cs_mesh_model_msg_device_list_size_t* packet = (cs_mesh_model_msg_device_list_size_t*) payload;
	LOGMeshModelDebug("received tracked device list size=%u", packet->listSize);
	TYPIFY(EVT_MESH_TRACKED_DEVICE_LIST_SIZE)* eventDataPtr = packet;
	event_t event(CS_TYPE::EVT_MESH_TRACKED_DEVICE_LIST_SIZE, eventDataPtr, sizeof(TYPIFY(EVT_MESH_TRACKED_DEVICE_LIST_SIZE)));
	event.dispatch();
}

void MeshMsgHandler::handleSyncRequest(uint8_t* payload, size16_t payloadSize) {
	auto packet = reinterpret_cast<cs_mesh_model_msg_sync_request_t*>(payload);
	LOGi("handleSyncRequest: id=%u bitmask=%x", packet->id, packet->bitmask);
	TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING)* eventDataPtr = packet;
	event_t event(CS_TYPE::EVT_MESH_SYNC_REQUEST_INCOMING, eventDataPtr, sizeof(TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING)));
	event.dispatch();
}

void MeshMsgHandler::handleStateSet(uint8_t* payload, size16_t payloadSize) {
	auto meshStateHeader = reinterpret_cast<cs_mesh_model_msg_state_header_t*>(payload);
	uint8_t stateDataSize = payloadSize - sizeof(*meshStateHeader);
	uint8_t* stateData = payload + sizeof(*meshStateHeader);

	LOGi("handleStateSet: type=%u id=%u persistenceMode=%u accessLevel=%u sourceId=%u data:",
			meshStateHeader->type,
			meshStateHeader->id,
			meshStateHeader->persistenceMode,
			meshStateHeader->accessLevel,
			meshStateHeader->sourceId
			);
	BLEutil::printArray(stateData, stateDataSize);

	TYPIFY(CMD_CONTROL_CMD) controlCmd;

	// The control command data starts with a state packet header,
	// followed by state data.
	uint8_t controlCmdDataSize = sizeof(state_packet_header_t) + stateDataSize;
	uint8_t controlCmdData[controlCmdDataSize];

	state_packet_header_t* stateHeader = (state_packet_header_t*)controlCmdData;
	memcpy(controlCmdData + sizeof(state_packet_header_t), stateData, stateDataSize);

	// Inflate state header.
	stateHeader->stateType =           meshStateHeader->type;
	stateHeader->stateId =             meshStateHeader->id;
	stateHeader->persistenceMode =     meshStateHeader->persistenceMode;

	// Inflate control command meta data.
	controlCmd.type =        CTRL_CMD_STATE_SET;
	controlCmd.data =        controlCmdData;
	controlCmd.size =        controlCmdDataSize;
	controlCmd.accessLevel = MeshUtil::getInflatedAccessLevel(meshStateHeader->accessLevel);
	controlCmd.source =      MeshUtil::getInflatedSource(meshStateHeader->sourceId);

	event_t event(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd));
	event.dispatch();
}

