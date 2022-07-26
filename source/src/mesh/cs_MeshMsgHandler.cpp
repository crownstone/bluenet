/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <events/cs_Event.h>
#include <mesh/cs_MeshCommon.h>
#include <mesh/cs_MeshMsgHandler.h>
#include <mesh/cs_MeshMsgEvent.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>
#include <uart/cs_UartHandler.h>
#include <storage/cs_State.h>
#include <util/cs_Utils.h>

void MeshMsgHandler::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_ownId, sizeof(_ownId));
}

void MeshMsgHandler::handleMsg(MeshMsgEvent& msg) {
	_log(LogLevelMeshDebug, false, "handleMsg src=%u opCode=%u isReply=%u rssi=%i ttl=%u isRelayed=%u msg=",
			msg.srcStoneId, msg.opCode, msg.isReply, msg.rssi, msg.ttl, msg.isMaybeRelayed);
	_logArray(LogLevelMeshDebug, true, msg.msg.data, msg.msg.len);


	if (!MeshUtil::isValidMeshPayload(msg.type, msg.msg.data, msg.msg.len)) {
		LOGw("Invalid mesh message of type %u", msg.type);
		replyWithRetCode(msg.type, ERR_INVALID_MESSAGE, msg.reply);
		return;
	}

	event_t event(CS_TYPE::EVT_RECV_MESH_MSG, &msg, sizeof(msg));
	event.dispatch();

	if (event.result.returnCode != ERR_EVENT_UNHANDLED) {
		// some handler took care of business.
		return;
	}

	cs_ret_code_t retCode = ERR_UNSPECIFIED;
	switch (msg.type) {
		case CS_MESH_MODEL_TYPE_TEST: {
			retCode = handleTest(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_ACK: {
			retCode = handleAck(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_CMD_TIME: {
			retCode = handleCmdTime(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_TIME_SYNC: {
			retCode = handleTimeSync(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_CMD_NOOP: {
			retCode = handleCmdNoop(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_RSSI_PING: {
			retCode = handleRssiPing(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_RSSI_DATA: {
			retCode = handleRssiData(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH: {
			retCode = handleCmdMultiSwitch(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_STATE_0: {
			retCode = handleState0(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_STATE_1: {
			retCode = handleState1(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_PROFILE_LOCATION: {
			retCode = handleProfileLocation(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS: {
			// Handled elsewhere.
			break;
		}
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_REGISTER: {
			retCode = handleTrackedDeviceRegister(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_TOKEN: {
			retCode = handleTrackedDeviceToken(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_HEARTBEAT: {
			retCode = handleTrackedDeviceHeartbeat(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_LIST_SIZE: {
			retCode = handleTrackedDeviceListSize(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_SYNC_REQUEST: {
			retCode = handleSyncRequest(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_STATE_SET: {
			handleStateSet(msg);
			// Return instead of break, as this function already sets the reply.
			return;
		}
		case CS_MESH_MODEL_TYPE_RESULT: {
			retCode = handleResult(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_SET_IBEACON_CONFIG_ID: {
			retCode = handleSetIbeaconConfigId(msg);
			break;
		}
		case CS_MESH_MODEL_TYPE_STONE_MAC: {
			break;
		}
		case CS_MESH_MODEL_TYPE_ASSET_FILTER_VERSION: {
			break;
		}
		case CS_MESH_MODEL_TYPE_ASSET_INFO_MAC: {
			break;
		}
		case CS_MESH_MODEL_TYPE_ASSET_INFO_ID: {
			break;
		}
		case CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI: {
			break;
		}
		case CS_MESH_MODEL_TYPE_CTRL_CMD: {
			handleControlCommand(msg);
			// Return instead of break, as this function already sets the reply.
			return;
		}
		case CS_MESH_MODEL_TYPE_MICROAPP: {
			break;
		}
		case CS_MESH_MODEL_TYPE_UNKNOWN: {
			retCode = ERR_INVALID_MESSAGE;
			break;
		}
	}
	replyWithRetCode(msg.type, retCode, msg.reply);
}

cs_ret_code_t MeshMsgHandler::dispatchEventForMeshMsg(CS_TYPE evtType, MeshMsgEvent& msg) {
	event_t event(evtType, &msg, sizeof(msg));
	event.dispatch();

	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleTest(MeshMsgEvent& msg) {
	[[maybe_unused]] cs_mesh_model_msg_test_t* test = reinterpret_cast<cs_mesh_model_msg_test_t*>(msg.msg.data);
	LOGi("received test counter=%u", test->counter);
#if MESH_MODEL_TEST_MSG == 1
	if (_lastReceivedCounter == 0) {
		_lastReceivedCounter = test->counter;
		return ERR_SUCCESS;
	}
	if (_lastReceivedCounter == test->counter) {
		LOGi("received same counter");
		return ERR_SUCCESS;
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
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleAck(MeshMsgEvent& msg) {
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t MeshMsgHandler::handleCmdTime(MeshMsgEvent& msg) {
	cs_mesh_model_msg_time_t* packet = reinterpret_cast<cs_mesh_model_msg_time_t*>(msg.msg.data);
	TYPIFY(CMD_SET_TIME) timestamp = packet->timestamp;
	LOGi("received set time %u", timestamp);
	if (timestamp != _lastReveivedSetTime) {
		_lastReveivedSetTime = timestamp;
		// TODO: send source.
		event_t event(CS_TYPE::CMD_SET_TIME, &timestamp, sizeof(timestamp), cmd_source_t(CS_CMD_SOURCE_TYPE_ENUM, CS_CMD_SOURCE_NONE, true));
		event.dispatch();
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_CMD_TIME, msg.msg.data, msg.msg.len);
//		return event.result.returnCode;
		return ERR_SUCCESS;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleTimeSync(MeshMsgEvent& msg) {
	LOGMeshModelInfo("handleTimeSync");
	cs_mesh_model_msg_time_sync_t* packet = reinterpret_cast<cs_mesh_model_msg_time_sync_t*>(msg.msg.data);

	TYPIFY(EVT_MESH_TIME_SYNC) eventData;
	eventData.stamp.posix_s  = packet->posix_s;
	eventData.stamp.posix_ms = packet->posix_ms;
	eventData.stamp.version  = packet->version;
	if (packet->overrideRoot) {
		eventData.srcId = 0;
	}
	else {
		eventData.srcId = msg.srcStoneId;
	}

	event_t event(CS_TYPE::EVT_MESH_TIME_SYNC, &eventData, sizeof(eventData));
	event.dispatch();

	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleCmdNoop(MeshMsgEvent& msg) {
	LOGMeshModelInfo("received noop");
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleRssiPing(MeshMsgEvent& msg) {
	return dispatchEventForMeshMsg(CS_TYPE::EVT_MESH_RSSI_PING, msg);
}

cs_ret_code_t MeshMsgHandler::handleRssiData(MeshMsgEvent& msg) {
	return dispatchEventForMeshMsg(CS_TYPE::EVT_MESH_RSSI_DATA, msg);
}

cs_ret_code_t MeshMsgHandler::handleCmdMultiSwitch(MeshMsgEvent& msg) {
	auto item = reinterpret_cast<cs_mesh_model_msg_multi_switch_item_t*>(msg.msg.data);
	if (item->id == _ownId) {
//		LOGMeshModelInfo("received multi switch for me");
//		if (memcmp(&_lastReceivedMultiSwitch, item, sizeof(*item)) == 0) {
//			LOGMeshModelDebug("ignore similar multi switch");
//			return ERR_SUCCESS;
//		}
//		memcpy(&_lastReceivedMultiSwitch, item, sizeof(*item));

		TYPIFY(CMD_MULTI_SWITCH) internalItem;
		internalItem.id = item->id;
		internalItem.cmd.switchCmd = item->switchCmd;

		LOGMeshModelInfo("execute multi switch cmd=%u source: type=%u id=%u", item->switchCmd, item->source.source.type, item->source.source.id);
		event_t event(CS_TYPE::CMD_MULTI_SWITCH, &internalItem, sizeof(internalItem), item->source);
		event.source.source.flagExternal = true;
		event.dispatch();
//		return event.result.returnCode;
		return ERR_SUCCESS;
	}
	return ERR_EVENT_UNHANDLED;
}

cs_ret_code_t MeshMsgHandler::handleState0(MeshMsgEvent& msg) {
	auto packet = reinterpret_cast<cs_mesh_model_msg_state_0_t*>(msg.msg.data);
	LOGMeshModelInfo("received: id=%u switch=%u flags=%u powerFactor=%i powerUsage=%i ts=%u", msg.srcStoneId, packet->switchState, packet->flags, packet->powerFactor, packet->powerUsageReal, packet->partialTimestamp);

	// Send event
	TYPIFY(EVT_MESH_EXT_STATE_0) state;
	state.stoneId = msg.srcStoneId;
	state.meshState = *packet;
	event_t event(CS_TYPE::EVT_MESH_EXT_STATE_0, &state, sizeof(state));
	event.dispatch();

	if (!isFromSameState(msg.srcStoneId, msg.srcStoneId, packet->partialTimestamp)) {
		_lastReceivedState.partsReceivedBitmask = 0;
	}
	_lastReceivedState.srcId = msg.srcStoneId;
	_lastReceivedState.state.data.state.id = msg.srcStoneId;
	_lastReceivedState.state.data.extState.switchState = packet->switchState;
	_lastReceivedState.state.data.extState.flags.asInt = packet->flags;
	_lastReceivedState.state.data.extState.powerFactor = packet->powerFactor;
	_lastReceivedState.state.data.extState.powerUsageReal = packet->powerUsageReal;
	_lastReceivedState.state.data.extState.partialTimestamp = packet->partialTimestamp;
	CsUtils::setBit(_lastReceivedState.partsReceivedBitmask, 0);
	checkStateReceived();
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleState1(MeshMsgEvent& msg) {
	auto packet = reinterpret_cast<cs_mesh_model_msg_state_1_t*>(msg.msg.data);
	LOGMeshModelInfo("received: id=%u temp=%i energy=%i ts=%u", msg.srcStoneId, packet->temperature, packet->energyUsed, packet->partialTimestamp);

	// Send event
	TYPIFY(EVT_MESH_EXT_STATE_1) state;
	state.stoneId = msg.srcStoneId;
	state.meshState = *packet;
	event_t event(CS_TYPE::EVT_MESH_EXT_STATE_1, &state, sizeof(state));
	event.dispatch();

	if (!isFromSameState(msg.srcStoneId, msg.srcStoneId, packet->partialTimestamp)) {
		_lastReceivedState.partsReceivedBitmask = 0;
	}
	_lastReceivedState.srcId = msg.srcStoneId;
	_lastReceivedState.state.data.state.id = msg.srcStoneId;
	_lastReceivedState.state.data.extState.temperature = packet->temperature;
	_lastReceivedState.state.data.extState.energyUsed = packet->energyUsed;
	_lastReceivedState.state.data.extState.partialTimestamp = packet->partialTimestamp;
	CsUtils::setBit(_lastReceivedState.partsReceivedBitmask, 1);
	checkStateReceived();
	return ERR_SUCCESS;
}

bool MeshMsgHandler::isFromSameState(stone_id_t srcId, stone_id_t id, uint16_t partialTimestamp) {
	return (_lastReceivedState.srcId == srcId
			&& _lastReceivedState.state.data.extState.id == id
			&& _lastReceivedState.state.data.extState.partialTimestamp == partialTimestamp);
}

void MeshMsgHandler::checkStateReceived() {
	if (_lastReceivedState.partsReceivedBitmask != 0x03) {
		return;
	}
	_lastReceivedState.state.data.extState.validation = SERVICE_DATA_VALIDATION;
	_lastReceivedState.state.data.type = SERVICE_DATA_DATA_TYPE_EXT_STATE;
#if CS_SERIAL_NRF_LOG_ENABLED == 0
	LOGi("Received state: id=%u switch=%u flags=%u temp=%i pf=%i power=%i energy=%i ts=%u rssi=%i",
			_lastReceivedState.state.data.extState.id,
			_lastReceivedState.state.data.extState.switchState,
			_lastReceivedState.state.data.extState.flags.asInt,
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

cs_ret_code_t MeshMsgHandler::handleProfileLocation(MeshMsgEvent& msg) {
	auto profileLocation = reinterpret_cast<cs_mesh_model_msg_profile_location_t*>(msg.msg.data);
	LOGMeshModelDebug("received profile=%u location=%u", profileLocation->profile, profileLocation->location);
	TYPIFY(EVT_RECEIVED_PROFILE_LOCATION) eventData;
	eventData.profileId = profileLocation->profile;
	eventData.locationId = profileLocation->location;
	eventData.fromMesh = true;
	event_t event(CS_TYPE::EVT_RECEIVED_PROFILE_LOCATION, &eventData, sizeof(eventData));
	event.dispatch();
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_PROFILE_LOCATION, msg.msg.data, msg.msg.len);
//	return event.result.returnCode;
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleTrackedDeviceRegister(MeshMsgEvent& msg) {
	auto packet = reinterpret_cast<cs_mesh_model_msg_device_register_t*>(msg.msg.data);
	LOGMeshModelInfo("received tracked device register id=%u profile=%u location=%u", packet->deviceId, packet->profileId, packet->locationId);
	TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)* eventDataPtr = packet;
	event_t event(CS_TYPE::EVT_MESH_TRACKED_DEVICE_REGISTER, eventDataPtr, sizeof(TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER)));
	event.dispatch();
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_TRACKED_DEVICE_REGISTER, msg.msg.data, msg.msg.len);
//	return event.result.returnCode;
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleTrackedDeviceToken(MeshMsgEvent& msg) {
	auto packet = reinterpret_cast<cs_mesh_model_msg_device_token_t*>(msg.msg.data);
	LOGMeshModelInfo("received tracked device token id=%u TTL=%u token=%u %u %u", packet->deviceId, packet->ttlMinutes, packet->deviceToken[0], packet->deviceToken[1], packet->deviceToken[2]);
	TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)* eventDataPtr = packet;
	event_t event(CS_TYPE::EVT_MESH_TRACKED_DEVICE_TOKEN, eventDataPtr, sizeof(TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN)));
	event.dispatch();
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_TRACKED_DEVICE_TOKEN, msg.msg.data, msg.msg.len);
//	return event.result.returnCode;
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleTrackedDeviceHeartbeat(MeshMsgEvent& msg) {
	auto packet = reinterpret_cast<cs_mesh_model_msg_device_heartbeat_t*>(msg.msg.data);
	LOGMeshModelInfo("received tracked device heartbeat id=%u location=%u TTL=%u", packet->deviceId, packet->locationId, packet->ttlMinutes);
	TYPIFY(EVT_MESH_TRACKED_DEVICE_HEARTBEAT)* eventDataPtr = packet;
	event_t event(CS_TYPE::EVT_MESH_TRACKED_DEVICE_HEARTBEAT, eventDataPtr, sizeof(TYPIFY(EVT_MESH_TRACKED_DEVICE_HEARTBEAT)));
	event.dispatch();
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_TRACKED_DEVICE_HEARTBEAT, msg.msg.data, msg.msg.len);
//	return event.result.returnCode;
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleTrackedDeviceListSize(MeshMsgEvent& msg) {
	auto packet = reinterpret_cast<cs_mesh_model_msg_device_list_size_t*>(msg.msg.data);
	LOGMeshModelInfo("received tracked device list size=%u", packet->listSize);
	TYPIFY(EVT_MESH_TRACKED_DEVICE_LIST_SIZE)* eventDataPtr = packet;
	event_t event(CS_TYPE::EVT_MESH_TRACKED_DEVICE_LIST_SIZE, eventDataPtr, sizeof(TYPIFY(EVT_MESH_TRACKED_DEVICE_LIST_SIZE)));
	event.dispatch();
//	return event.result.returnCode;
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleSyncRequest(MeshMsgEvent& msg) {
	auto packet = reinterpret_cast<cs_mesh_model_msg_sync_request_t*>(msg.msg.data);
	LOGi("handleSyncRequest: id=%u bitmask=%x", packet->id, packet->bitmask);
	TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING)* eventDataPtr = packet;
	event_t event(CS_TYPE::EVT_MESH_SYNC_REQUEST_INCOMING, eventDataPtr, sizeof(TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING)));
	event.dispatch();
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_SYNC_REQUEST, msg.msg.data, msg.msg.len);
//	return event.result.returnCode;
	return ERR_SUCCESS;
}

cs_ret_code_t MeshMsgHandler::handleSetIbeaconConfigId(MeshMsgEvent& msg) {
	LOGi("handleSetIbeaconConfigId");
	// Event and mesh msg use the same type.
	event_t event(CS_TYPE::CMD_SET_IBEACON_CONFIG_ID, msg.msg.data, msg.msg.len);
	event.dispatch();
	return event.result.returnCode;
}

void MeshMsgHandler::handleStateSet(MeshMsgEvent& msg) {
	auto meshStateHeader = reinterpret_cast<cs_mesh_model_msg_state_header_ext_t*>(msg.msg.data);
	uint8_t stateDataSize = msg.msg.len - sizeof(*meshStateHeader);
	uint8_t* stateData = msg.msg.data + sizeof(*meshStateHeader);

	_log(SERIAL_INFO, false, "handleStateSet: type=%u id=%u persistenceMode=%u accessLevel=%u sourceId=%u data: ",
			meshStateHeader->header.type,
			meshStateHeader->header.id,
			meshStateHeader->header.persistenceMode,
			meshStateHeader->accessLevel,
			meshStateHeader->sourceId
			);
	_logArray(SERIAL_INFO, true, stateData, stateDataSize);

	// The reply is a result message, with a state header as payload.
	cs_mesh_model_msg_result_header_t* resultHeader = nullptr;
	if (msg.reply != nullptr && msg.reply->buf.len > sizeof(cs_mesh_model_msg_result_header_t) + sizeof(cs_mesh_model_msg_state_header_t)) {
		resultHeader = reinterpret_cast<cs_mesh_model_msg_result_header_t*>(msg.reply->buf.data);
		resultHeader->msgType = CS_MESH_MODEL_TYPE_STATE_SET;

		cs_mesh_model_msg_state_header_t* stateHeader = reinterpret_cast<cs_mesh_model_msg_state_header_t*>(
				msg.reply->buf.data + sizeof(cs_mesh_model_msg_result_header_t)
		);
		// Simply copy the data of the incoming message.
		*stateHeader = meshStateHeader->header;

		msg.reply->type = CS_MESH_MODEL_TYPE_RESULT;
		msg.reply->dataSize = sizeof(cs_mesh_model_msg_result_header_t) + sizeof(cs_mesh_model_msg_state_header_t);
	}

	TYPIFY(CMD_CONTROL_CMD) controlCmd;

	// The control command data starts with a state packet header,
	// followed by state data.
	uint8_t controlCmdDataSize = sizeof(state_packet_header_t) + stateDataSize;

	// We could also use the non ISO alloca().
	uint8_t* controlCmdData = new (std::nothrow) uint8_t[controlCmdDataSize];
	if (controlCmdData == nullptr) {
		LOGw("Cannot allocate control command data.");
		if (resultHeader != nullptr) {
			resultHeader->retCode = MeshUtil::getShortenedRetCode(ERR_NO_SPACE);
		}
		return;
	}

	state_packet_header_t* stateHeader = (state_packet_header_t*)controlCmdData;
	memcpy(controlCmdData + sizeof(state_packet_header_t), stateData, stateDataSize);

	// Inflate state header.
	stateHeader->stateType =           meshStateHeader->header.type;
	stateHeader->stateId =             meshStateHeader->header.id;
	stateHeader->persistenceMode =     meshStateHeader->header.persistenceMode;

	// Inflate source.
	cmd_source_with_counter_t source = MeshUtil::getInflatedSource(meshStateHeader->sourceId);

	// Inflate control command meta data.
	controlCmd.protocolVersion =  CS_CONNECTION_PROTOCOL_VERSION;
	controlCmd.type =             CTRL_CMD_STATE_SET;
	controlCmd.data =             controlCmdData;
	controlCmd.size =             controlCmdDataSize;
	controlCmd.accessLevel =      MeshUtil::getInflatedAccessLevel(meshStateHeader->accessLevel);

	event_t event(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd), source);
	event.dispatch();

	delete [] controlCmdData;

	if (resultHeader != nullptr) {
		resultHeader->retCode = MeshUtil::getShortenedRetCode(event.result.returnCode);
	}

	LOGi("retCode=%u", event.result.returnCode);
}

void MeshMsgHandler::handleControlCommand(MeshMsgEvent& msg) {
	_logArray(SERIAL_INFO, true, msg.msg.data, msg.msg.len);
	auto meshMsgHeader = reinterpret_cast<cs_mesh_model_msg_ctrl_cmd_header_ext_t*>(msg.msg.data);
	uint8_t cmdPayloadSize = msg.msg.len - sizeof(*meshMsgHeader);
	uint8_t* cmdPayloadData = msg.msg.data + sizeof(*meshMsgHeader);

	_log(SERIAL_INFO, false, "handleControlCommand: type=%u accessLevel=%u sourceId=%u data: ",
			meshMsgHeader->header.cmdType,
			meshMsgHeader->accessLevel,
			meshMsgHeader->sourceId
			);
	_logArray(SERIAL_INFO, true, cmdPayloadData, cmdPayloadSize);

	// The reply is a result message, with cs_mesh_model_msg_ctrl_cmd_header_t as payload.
	cs_mesh_model_msg_result_header_t* resultHeader = nullptr;
	if (msg.reply != nullptr && msg.reply->buf.len > sizeof(cs_mesh_model_msg_result_header_t) + sizeof(cs_mesh_model_msg_ctrl_cmd_header_t)) {
		resultHeader = reinterpret_cast<cs_mesh_model_msg_result_header_t*>(msg.reply->buf.data);
		resultHeader->msgType = CS_MESH_MODEL_TYPE_CTRL_CMD;

		cs_mesh_model_msg_ctrl_cmd_header_t* resultPayload = reinterpret_cast<cs_mesh_model_msg_ctrl_cmd_header_t*>(
				msg.reply->buf.data + sizeof(cs_mesh_model_msg_result_header_t)
		);

		// Simply copy the data of the incoming message.
		resultPayload->cmdType = meshMsgHeader->header.cmdType;

		msg.reply->type = CS_MESH_MODEL_TYPE_RESULT;
		msg.reply->dataSize = sizeof(cs_mesh_model_msg_result_header_t) + sizeof(*resultPayload);
	}

	TYPIFY(CMD_CONTROL_CMD) controlCmd;
	controlCmd.protocolVersion =  CS_CONNECTION_PROTOCOL_VERSION;
	controlCmd.type =             static_cast<CommandHandlerTypes>(meshMsgHeader->header.cmdType);
	controlCmd.data =             cmdPayloadData;
	controlCmd.size =             cmdPayloadSize;
	controlCmd.accessLevel =      MeshUtil::getInflatedAccessLevel(meshMsgHeader->accessLevel);

	// Inflate source.
	cmd_source_with_counter_t source = MeshUtil::getInflatedSource(meshMsgHeader->sourceId);

	event_t event(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd), source);
	event.dispatch();

	if (resultHeader != nullptr) {
		resultHeader->retCode = MeshUtil::getShortenedRetCode(event.result.returnCode);
	}

	if (msg.reply != nullptr) {
		_log(LogLevelMeshDebug, false, "reply: type=%u dataSize=%u buf=", msg.reply->type, msg.reply->dataSize);
		_logArray(LogLevelMeshDebug, true, msg.reply->buf.data, msg.reply->buf.len);
	}

	LOGi("retCode=%u", event.result.returnCode);
}

cs_ret_code_t MeshMsgHandler::handleResult(MeshMsgEvent& msg) {
	auto header = reinterpret_cast<cs_mesh_model_msg_result_header_t*>(msg.msg.data);
	cs_data_t resultData(msg.msg.data + sizeof(*header), msg.msg.len - sizeof(*header));

	// Convert to result packet header.
	uart_msg_mesh_result_packet_header_t resultHeader;
	resultHeader.stoneId = msg.srcStoneId;
	resultHeader.resultHeader.returnCode = MeshUtil::getInflatedRetCode(header->retCode);

	resultHeader.resultHeader.commandType = MeshUtil::getCtrlCmdType(
			msg.type,
			msg.msg.data,
			msg.msg.len
	);

	if (resultHeader.resultHeader.commandType == CTRL_CMD_UNKNOWN) {
		LOGw("Unknown command type for msg type %u, did you add it to getCtrlCmdType()?", header->msgType);
	}

	_log(SERIAL_INFO, false, "handleResult: id=%u meshType=%u commandType=%u retCode=%u data=",
			msg.srcStoneId,
			header->msgType,
			resultHeader.resultHeader.commandType,
			header->retCode);
	_logArray(SERIAL_INFO, true, resultData.data, resultData.len);

	// Convert result data if needed.
	switch (header->msgType) {
		case CS_MESH_MODEL_TYPE_STATE_SET: {
			if (resultData.len == sizeof(cs_mesh_model_msg_state_header_t)) {
				// Inflate state header.
				cs_mesh_model_msg_state_header_t* meshStateHeader = (cs_mesh_model_msg_state_header_t*)resultData.data;
				state_packet_header_t stateHeader;
				stateHeader.stateType =           meshStateHeader->type;
				stateHeader.stateId =             meshStateHeader->id;
				stateHeader.persistenceMode =     meshStateHeader->persistenceMode;
				sendResultToUart(resultHeader, cs_data_t((uint8_t*)&stateHeader, sizeof(stateHeader)));
				return ERR_SUCCESS;
			}
			LOGw("Wrong result data size: %u", resultData.len);
			resultData.len = 0;
			break;
		}
		case CS_MESH_MODEL_TYPE_CTRL_CMD: {
			// The control command type is in the resultData, but it was already used to set the command type in the resultHeader.
			// The remainder is payload data.
			if (resultData.len >= sizeof(cs_mesh_model_msg_ctrl_cmd_header_t)) {
				resultData.data += sizeof(cs_mesh_model_msg_ctrl_cmd_header_t);
				resultData.len -= sizeof(cs_mesh_model_msg_ctrl_cmd_header_t);
				sendResultToUart(resultHeader, resultData);
				return ERR_SUCCESS;
			}
			LOGw("Wrong result data size: %u", resultData.len);
			resultData.len = 0;
			break;
		}
		default: {
			sendResultToUart(resultHeader, resultData);
			break;
		}
	}
	return ERR_SUCCESS;
}

void MeshMsgHandler::replyWithRetCode(cs_mesh_model_msg_type_t type, cs_ret_code_t retCode, mesh_reply_t* reply) {
	if (reply == nullptr) {
		return;
	}
	if (reply->buf.len < sizeof(cs_mesh_model_msg_result_header_t)) {
		return;
	}
	cs_mesh_model_msg_result_header_t* packet = reinterpret_cast<cs_mesh_model_msg_result_header_t*>(reply->buf.data);
	packet->msgType = type;
	packet->retCode = MeshUtil::getShortenedRetCode(retCode);
	reply->type = CS_MESH_MODEL_TYPE_RESULT;
	reply->dataSize = sizeof(cs_mesh_model_msg_result_header_t);
}

void MeshMsgHandler::sendResultToUart(uart_msg_mesh_result_packet_header_t& resultHeader, const cs_data_t& resultData) {
	resultHeader.resultHeader.payloadSize = resultData.len;

	_log(SERIAL_INFO, false, "Ack result: id=%u cmdType=%u retCode=%u data=", resultHeader.stoneId, resultHeader.resultHeader.commandType, resultHeader.resultHeader.returnCode);
	_logArray(SERIAL_INFO, true, resultData.data, resultHeader.resultHeader.payloadSize);

	// Send out result.
	UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_MESH_RESULT, sizeof(resultHeader) + resultData.len);
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_MESH_RESULT, (uint8_t*)&resultHeader, sizeof(resultHeader));
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_MESH_RESULT, resultData.data, resultData.len);
	UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_MESH_RESULT);
}

