/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <events/cs_Event.h>
#include <mesh/cs_MeshMsgSender.h>
#include <mesh/cs_MeshCommon.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>
#include <util/cs_BleError.h>



void MeshMsgSender::init(MeshModelSelector* selector) {
	_selector = selector;
}

cs_ret_code_t MeshMsgSender::sendMsg(cs_mesh_msg_t *meshMsg) {
	if (!MeshUtil::isValidMeshMessage(meshMsg)) {
		LOGMeshWarning("MeshMsgSender::sendMsg invalid message");
		return ERR_INVALID_MESSAGE;
	}

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = 0;
	item.metaData.type = meshMsg->type;
	item.metaData.priority = meshMsg->urgency == CS_MESH_URGENCY_HIGH;
	item.metaData.transmissionsOrTimeout = meshMsg->reliability;
	item.reliable = meshMsg->flags.flags.reliable;
	item.broadcast = meshMsg->flags.flags.broadcast;
	item.noHop = meshMsg->flags.flags.noHops;
	item.numIds = meshMsg->idCount;
	item.stoneIdsPtr = meshMsg->targetIds;
	item.msgPayload.len = meshMsg->size;
	item.msgPayload.data = meshMsg->payload;

	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendTestMsg() {
	cs_mesh_model_msg_test_t test;
#if MESH_MODEL_TEST_MSG != 0
	test.counter = _nextSendCounter++;
#else
	test.counter = 0;
#endif

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = 0;
	item.metaData.type = CS_MESH_MODEL_TYPE_TEST;
	item.metaData.priority = false;

#if MESH_MODEL_TEST_MSG == 2
	item.metaData.transmissionsOrTimeout = 0;
	item.reliable = true;
	item.broadcast = false;
	stone_id_t targetId = 1;
	item.numIds = 1;
	item.stoneIdsPtr = &targetId;
#else
	item.metaData.transmissionsOrTimeout = 1;
	item.reliable = false;
	item.broadcast = true;
#endif

	for (uint8_t i=0; i<sizeof(test.dummy); ++i) {
		test.dummy[i] = i;
	}
	item.msgPayload.len = sizeof(test);
	item.msgPayload.data = (uint8_t*)&test;

	LOGd("sendTestMsg counter=%u", test.counter);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendSetTime(const cs_mesh_model_msg_time_t* packet, uint8_t transmissions) {
	LOGd("sendSetTime");
	if (packet->timestamp == 0) {
		return ERR_WRONG_PARAMETER;
	}

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = 0;
	item.metaData.type = CS_MESH_MODEL_TYPE_CMD_TIME;
	item.metaData.transmissionsOrTimeout = (transmissions == 0) ? CS_MESH_RELIABILITY_LOW : transmissions;
	item.metaData.priority = true;
	item.reliable = false;
	item.broadcast = true;
	item.msgPayload.len = sizeof(*packet);
	item.msgPayload.data = (uint8_t*)packet;

	// Remove old messages of same type, as only the latest is of interest.
	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendNoop(uint8_t transmissions) {
	LOGd("sendNoop");

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = 0;
	item.metaData.type = CS_MESH_MODEL_TYPE_CMD_NOOP;
	item.metaData.transmissionsOrTimeout = (transmissions == 0) ? CS_MESH_RELIABILITY_LOW : transmissions;
	item.metaData.priority = false;
	item.reliable = false;
	item.broadcast = true;

	// Remove old messages of same type, as only the latest is of interest.
	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendMultiSwitchItem(const internal_multi_switch_item_t* switchItem, const cmd_source_with_counter_t& source, uint8_t transmissions) {
	LOGMeshModelDebug("sendMultiSwitchItem");
	cs_mesh_model_msg_multi_switch_item_t meshItem;
	meshItem.id = switchItem->id;
	meshItem.switchCmd = switchItem->cmd.switchCmd;
	meshItem.source = source;

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = switchItem->id;
	item.metaData.type = CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH;
	item.metaData.transmissionsOrTimeout = (transmissions == 0) ? CS_MESH_RELIABILITY_LOW : transmissions;
	item.metaData.priority = true;
	item.reliable = false;
	item.broadcast = true;
	item.msgPayload.len = sizeof(meshItem);
	item.msgPayload.data = (uint8_t*)(&meshItem);

	switch (source.source.type) {
		case CS_CMD_SOURCE_TYPE_ENUM: {
			switch (source.source.id) {
				case CS_CMD_SOURCE_CONNECTION: {
//				case CS_CMD_SOURCE_UART: {
					LOGMeshModelInfo("Source connection: set high transmission count");
					transmissions = CS_MESH_RELIABILITY_HIGH;
					// Keep using unreliable for now, as reliable will be handled 1 by 1 instead of interleaved.
//					LOGMeshModelInfo("Source connection or uart: use acked msg");
//					item.reliable = true;
//					item.numIds = 1;
//					item.metaData.transmissionsOrTimeout = 0;
//					item.stoneIdsPtr = (stone_id_t*) &(switchItem->id);
					break;
				}
				default:
					break;
			}
			break;
		}
		case CS_CMD_SOURCE_TYPE_UART: {
			LOGMeshModelInfo("Source uart: set high transmission count");
			transmissions = CS_MESH_RELIABILITY_HIGH;
			break;
		}
		default:
			break;
	}

	// Remove old messages of same type and with same target id.
	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendBehaviourSettings(const behaviour_settings_t* packet, uint8_t transmissions) {
	LOGMeshModelDebug("sendBehaviourSettings");

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = 0;
	item.metaData.type = CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS;
	item.metaData.transmissionsOrTimeout = (transmissions == 0) ? CS_MESH_RELIABILITY_LOWEST : transmissions;
	item.metaData.priority = false;
	item.reliable = false;
	item.broadcast = true;
	item.msgPayload.len = sizeof(*packet);
	item.msgPayload.data = (uint8_t*)packet;

	// Remove old messages of same type, as only the latest is of interest.
	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendProfileLocation(const cs_mesh_model_msg_profile_location_t* packet, uint8_t transmissions) {
	LOGd("sendProfileLocation profile=%u location=%u", packet->profile, packet->location);

	uint16_t id = (packet->location << 8) + packet->profile;

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = id;
	item.metaData.type = CS_MESH_MODEL_TYPE_PROFILE_LOCATION;
	item.metaData.transmissionsOrTimeout = (transmissions == 0) ? CS_MESH_RELIABILITY_LOWEST : transmissions;
	item.metaData.priority = false;
	item.reliable = false;
	item.broadcast = true;
	item.msgPayload.len = sizeof(*packet);
	item.msgPayload.data = (uint8_t*)packet;

	// Remove old messages of same type, location, and profile.
	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendTrackedDeviceRegister(const cs_mesh_model_msg_device_register_t* packet, uint8_t transmissions) {
	LOGd("sendTrackedDeviceRegister id=%u profile=%u location=%u", packet->deviceId, packet->profileId, packet->locationId);

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = packet->deviceId;
	item.metaData.type = CS_MESH_MODEL_TYPE_TRACKED_DEVICE_REGISTER;
	item.metaData.transmissionsOrTimeout = (transmissions == 0) ? CS_MESH_RELIABILITY_LOW : transmissions;
	item.metaData.priority = false;
	item.reliable = false;
	item.broadcast = true;
	item.msgPayload.len = sizeof(*packet);
	item.msgPayload.data = (uint8_t*)packet;

	// Remove old messages of same type, and device id, as only the latest register is of interest.
	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendTrackedDeviceToken(const cs_mesh_model_msg_device_token_t* packet, uint8_t transmissions) {
	LOGd("sendTrackedDeviceToken id=%u TTL=%u token=%u %u %u", packet->deviceId, packet->ttlMinutes, packet->deviceToken[0], packet->deviceToken[1], packet->deviceToken[2]);

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = packet->deviceId;
	item.metaData.type = CS_MESH_MODEL_TYPE_TRACKED_DEVICE_TOKEN;
	item.metaData.transmissionsOrTimeout = (transmissions == 0) ? CS_MESH_RELIABILITY_LOW : transmissions;
	item.metaData.priority = false;
	item.reliable = false;
	item.broadcast = true;
	item.msgPayload.len = sizeof(*packet);
	item.msgPayload.data = (uint8_t*)packet;

	// Remove old messages of same type, and device id, as only the latest token is of interest.
	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendTrackedDeviceHeartbeat(const cs_mesh_model_msg_device_heartbeat_t* packet, uint8_t transmissions) {
	LOGd("sendTrackedDeviceHeartbeat id=%u location=%u TTL=%u", packet->deviceId, packet->locationId, packet->ttlMinutes);

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = packet->deviceId;
	item.metaData.type = CS_MESH_MODEL_TYPE_TRACKED_DEVICE_HEARTBEAT;
	item.metaData.transmissionsOrTimeout = (transmissions == 0) ? CS_MESH_RELIABILITY_LOW : transmissions;
	item.metaData.priority = false;
	item.reliable = false;
	item.broadcast = true;
	item.msgPayload.len = sizeof(*packet);
	item.msgPayload.data = (uint8_t*)packet;

	// Remove old messages of same type, and device id, as only the latest token is of interest.
	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::sendTrackedDeviceListSize(const cs_mesh_model_msg_device_list_size_t* packet, uint8_t transmissions) {
	LOGd("sendTrackedDeviceListSize");

	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = 0;
	item.metaData.type = CS_MESH_MODEL_TYPE_TRACKED_DEVICE_LIST_SIZE;
	item.metaData.transmissionsOrTimeout = (transmissions == 0) ? CS_MESH_RELIABILITY_LOW : transmissions;
	item.metaData.priority = false;
	item.reliable = false;
	item.broadcast = true;
	item.msgPayload.len = sizeof(*packet);
	item.msgPayload.data = (uint8_t*)packet;

	// Remove old messages of same type, as only the latest is of interest.
	remFromQueue(item);
	return addToQueue(item);
}

cs_ret_code_t MeshMsgSender::addToQueue(MeshUtil::cs_mesh_queue_item_t & item) {
	assert(_selector != nullptr, "No model selector set.");

	if (item.reliable) {
		if (item.metaData.transmissionsOrTimeout == 0) {
			item.metaData.transmissionsOrTimeout = MESH_MODEL_RELIABLE_TIMEOUT_DEFAULT;
		}
		else if (item.metaData.transmissionsOrTimeout < (ACCESS_RELIABLE_TIMEOUT_MIN / 1000000)) {
			item.metaData.transmissionsOrTimeout = (ACCESS_RELIABLE_TIMEOUT_MIN / 1000000);
		}
		else if (item.metaData.transmissionsOrTimeout > (ACCESS_RELIABLE_TIMEOUT_MAX / 1000000)) {
			item.metaData.transmissionsOrTimeout = (ACCESS_RELIABLE_TIMEOUT_MAX / 1000000);
		}
	}
	else {
		if (item.metaData.transmissionsOrTimeout == 0) {
			item.metaData.transmissionsOrTimeout = MESH_MODEL_TRANSMISSIONS_DEFAULT;
		}
		else if (item.metaData.transmissionsOrTimeout > MESH_MODEL_TRANSMISSIONS_MAX) {
			item.metaData.transmissionsOrTimeout = MESH_MODEL_TRANSMISSIONS_MAX;
		}
	}
	return _selector->addToQueue(item);
}

cs_ret_code_t MeshMsgSender::remFromQueue(MeshUtil::cs_mesh_queue_item_t & item) {
	assert(_selector != nullptr, "No model selector set.");
	return _selector->remFromQueue(item);
}



cs_ret_code_t MeshMsgSender::handleSendMeshCommand(mesh_control_command_packet_t* command, const cmd_source_with_counter_t& source) {
	LOGi("handleSendMeshCommand type=%u idCount=%u flags=%u timeoutOrTransmissions=%u",
			command->header.type,
			command->header.idCount,
			command->header.flags.asInt,
			command->header.timeoutOrTransmissions
			);
	LOGi("  ctrlType=%u ctrlSize=%u accessLevel=%u sourceType=%u sourceId=%u",
			command->controlCommand.type,
			command->controlCommand.size,
			command->controlCommand.accessLevel,
			source.source.type,
			source.source.id
			);
	for (uint8_t i=0; i<command->header.idCount; ++i) {
		LOGd("  id: %u", command->targetIds[i]);
	}

	MeshUtil::cs_mesh_queue_item_t item;

	// These fields can should be decided per type.
	item.metaData.id = 0;
	item.metaData.type = CS_MESH_MODEL_TYPE_UNKNOWN;
	item.metaData.priority = false;

	// All these fields can be set from the mesh command.
	item.metaData.transmissionsOrTimeout = command->header.timeoutOrTransmissions;
	item.reliable = command->header.flags.flags.reliable;
	item.broadcast = command->header.flags.flags.broadcast;
	item.numIds = command->header.idCount;
	item.stoneIdsPtr = command->targetIds;

	switch (command->controlCommand.type) {
		case CTRL_CMD_SET_TIME: {
			if (command->controlCommand.size != sizeof(uint32_t)) {
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			if (command->header.idCount != 0
					|| command->header.flags.flags.broadcast != true
					|| command->header.flags.flags.reliable != false
					|| command->header.flags.flags.useKnownIds != false) {
				return ERR_NOT_IMPLEMENTED;
			}
			uint32_t* time = (uint32_t*) command->controlCommand.data;
			cs_mesh_model_msg_time_t packet;
			packet.timestamp = *time;

			uint8_t transmissions = command->header.timeoutOrTransmissions;
			if (transmissions == 0) {
				transmissions = CS_MESH_RELIABILITY_MEDIUM;
			}
			return sendSetTime(&packet, transmissions);
		}
		case CTRL_CMD_SET_IBEACON_CONFIG_ID: {
			if (command->controlCommand.size != sizeof(set_ibeacon_config_id_packet_t)) {
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			item.metaData.id = 0;
			item.metaData.type = CS_MESH_MODEL_TYPE_SET_IBEACON_CONFIG_ID;
			item.metaData.priority = false;
			item.msgPayload.len = command->controlCommand.size;
			item.msgPayload.data = command->controlCommand.data;
			return addToQueue(item);
			break;
		}
		case CTRL_CMD_STATE_SET: {
			// Size has already been checked in command handler.
			state_packet_header_t* stateHeader = (state_packet_header_t*) command->controlCommand.data;
			LOGd("State type=%u id=%u persistenceMode=%u", stateHeader->stateType, stateHeader->stateId, stateHeader->persistenceMode);
			if (command->header.flags.flags.reliable != true || command->header.flags.flags.useKnownIds != false) {
				return ERR_NOT_IMPLEMENTED;
			}
			if (!MeshUtil::canShortenStateType(stateHeader->stateType)) {
				LOGw("Can't shorten state type %u", stateHeader->stateType);
				return ERR_WRONG_PARAMETER;
			}
			if (!MeshUtil::canShortenStateId(stateHeader->stateId)) {
				LOGw("Can't shorten state id %u", stateHeader->stateId);
				return ERR_WRONG_PARAMETER;
			}
			if (!MeshUtil::canShortenPersistenceMode(stateHeader->persistenceMode)) {
				LOGw("Can't shorten persistenceMode %u", stateHeader->persistenceMode);
				return ERR_WRONG_PARAMETER;
			}
			if (!MeshUtil::canShortenAccessLevel(command->controlCommand.accessLevel)) {
				LOGw("Can't shorten accessLevel %u", command->controlCommand.accessLevel);
				return ERR_WRONG_PARAMETER;
			}
			if (!MeshUtil::canShortenSource(source)) {
				LOGw("Can't shorten source type=%u id=%u", source.source.type, source.source.id);
				return ERR_WRONG_PARAMETER;
			}

			uint8_t stateHeaderSize = sizeof(state_packet_header_t);
			uint8_t statePayloadSize = command->controlCommand.size - stateHeaderSize;

			uint16_t meshMsgSize = sizeof(cs_mesh_model_msg_state_header_ext_t) + statePayloadSize;
			uint8_t* meshMsg = new (std::nothrow) uint8_t[meshMsgSize];
			if (meshMsg == nullptr) {
				return ERR_NO_SPACE;
			}
			cs_mesh_model_msg_state_header_ext_t* meshStateHeader = (cs_mesh_model_msg_state_header_ext_t*) meshMsg;

			meshStateHeader->header.type = stateHeader->stateType;
			meshStateHeader->header.id = stateHeader->stateId;
			meshStateHeader->header.persistenceMode = stateHeader->persistenceMode;
			meshStateHeader->accessLevel = MeshUtil::getShortenedAccessLevel(command->controlCommand.accessLevel);
			meshStateHeader->sourceId = MeshUtil::getShortenedSource(source);

			memcpy(meshMsg + sizeof(*meshStateHeader), command->controlCommand.data + stateHeaderSize, statePayloadSize);

			item.metaData.id = 0;
			item.metaData.type = CS_MESH_MODEL_TYPE_STATE_SET;
			item.metaData.priority = false;
			item.msgPayload.len = meshMsgSize;
			item.msgPayload.data = meshMsg;
			cs_ret_code_t retCode = addToQueue(item);
			delete [] meshMsg;
			return retCode;
		}
		default: {
			// Size and cmd type have already been checked in command handler.
			if (!MeshUtil::canShortenAccessLevel(command->controlCommand.accessLevel)) {
				LOGw("Can't shorten accessLevel %u", command->controlCommand.accessLevel);
				return ERR_WRONG_PARAMETER;
			}
			if (!MeshUtil::canShortenSource(source)) {
				LOGw("Can't shorten source type=%u id=%u", source.source.type, source.source.id);
				return ERR_WRONG_PARAMETER;
			}

			uint8_t cmdPayloadSize = command->controlCommand.size;
			uint16_t meshMsgSize = sizeof(cs_mesh_model_msg_ctrl_cmd_header_ext_t) + cmdPayloadSize;
			uint8_t* meshMsg = new (std::nothrow) uint8_t[meshMsgSize];
			if (meshMsg == nullptr) {
				return ERR_NO_SPACE;
			}

			cs_mesh_model_msg_ctrl_cmd_header_ext_t* meshMsgHeader = reinterpret_cast<cs_mesh_model_msg_ctrl_cmd_header_ext_t*>(meshMsg);
			meshMsgHeader->header.cmdType = command->controlCommand.type;
			meshMsgHeader->accessLevel = MeshUtil::getShortenedAccessLevel(command->controlCommand.accessLevel);
			meshMsgHeader->sourceId = MeshUtil::getShortenedSource(source);

			memcpy(meshMsg + sizeof(*meshMsgHeader), command->controlCommand.data, cmdPayloadSize);

			item.metaData.id = 0;
			item.metaData.type = CS_MESH_MODEL_TYPE_CTRL_CMD;
			item.metaData.priority = false;
			item.msgPayload.len = meshMsgSize;
			item.msgPayload.data = meshMsg;
			cs_ret_code_t retCode = addToQueue(item);
			delete [] meshMsg;
			return retCode;
		}
	}

	return ERR_NOT_IMPLEMENTED;
}

void MeshMsgSender::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::CMD_SEND_MESH_MSG: {
			TYPIFY(CMD_SEND_MESH_MSG)* msg = (TYPIFY(CMD_SEND_MESH_MSG)*)event.data;
			event.result.returnCode = sendMsg(msg);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_SET_TIME: {
			TYPIFY(CMD_SEND_MESH_MSG_SET_TIME)* packet = (TYPIFY(CMD_SEND_MESH_MSG_SET_TIME)*)event.data;
			cs_mesh_model_msg_time_t item;
			item.timestamp = *packet;
			event.result.returnCode = sendSetTime(&item);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_NOOP: {
			event.result.returnCode = sendNoop();
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_MULTI_SWITCH: {
			TYPIFY(CMD_SEND_MESH_MSG_MULTI_SWITCH)* packet = (TYPIFY(CMD_SEND_MESH_MSG_MULTI_SWITCH)*)event.data;
			event.result.returnCode = sendMultiSwitchItem(packet, event.source);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS: {
			TYPIFY(CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS)* packet = (TYPIFY(CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS)*)event.data;
			event.result.returnCode = sendBehaviourSettings(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_PROFILE_LOCATION: {
			TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION)* packet = (TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION)*)event.data;
			event.result.returnCode = sendProfileLocation(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER: {
			TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER)* packet = (TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER)*)event.data;
			event.result.returnCode = sendTrackedDeviceRegister(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN: {
			TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN)* packet = (TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN)*)event.data;
			event.result.returnCode = sendTrackedDeviceToken(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_HEARTBEAT: {
			TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_HEARTBEAT)* packet = (TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_HEARTBEAT)*)event.data;
			event.result.returnCode = sendTrackedDeviceHeartbeat(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE: {
			TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE)* packet = (TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE)*)event.data;
			event.result.returnCode = sendTrackedDeviceListSize(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_CONTROL_COMMAND: {
			TYPIFY(CMD_SEND_MESH_CONTROL_COMMAND)* packet = (TYPIFY(CMD_SEND_MESH_CONTROL_COMMAND)*)event.data;
			event.result.returnCode = handleSendMeshCommand(packet, event.source);
			break;
		}
		default:
			break;
	}
}
