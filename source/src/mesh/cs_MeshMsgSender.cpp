/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
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
		return ERR_INVALID_MESSAGE;
	}
	uint8_t repeats = meshMsg->reliability;
	bool priority = meshMsg->urgency == CS_MESH_URGENCY_HIGH;
	remFromQueue(meshMsg->type, 0, 0);
	return addToQueue(meshMsg->type, 0, 0, meshMsg->payload, meshMsg->size, repeats, priority);
}

cs_ret_code_t MeshMsgSender::sendTestMsg() {
	cs_mesh_model_msg_test_t test;
#if MESH_MODEL_TEST_MSG != 0
	test.counter = _nextSendCounter++;
#else
	test.counter = 0;
#endif
#if MESH_MODEL_TEST_MSG == 2
	stone_id_t targetId = 1;
#else
	stone_id_t targetId = 0;
#endif
	for (uint8_t i=0; i<sizeof(test.dummy); ++i) {
		test.dummy[i] = i;
	}
	uint8_t repeats = 1;
	LOGd("sendTestMsg counter=%u", test.counter);
	return addToQueue(CS_MESH_MODEL_TYPE_TEST, targetId, 0, (uint8_t*)&test, sizeof(test), repeats, false);
}

cs_ret_code_t MeshMsgSender::sendSetTime(const cs_mesh_model_msg_time_t* item, uint8_t repeats) {
	if (item->timestamp == 0) {
		return ERR_WRONG_PARAMETER;
	}
	// Remove old messages of same type, as only the latest is of interest.
	remFromQueue(CS_MESH_MODEL_TYPE_CMD_TIME, 0, 0);
	return addToQueue(CS_MESH_MODEL_TYPE_CMD_TIME, 0, 0, (uint8_t*)item, sizeof(*item), repeats, false);
}

cs_ret_code_t MeshMsgSender::sendNoop(uint8_t repeats) {
	// Remove old messages of same type, as only the latest is of interest.
	remFromQueue(CS_MESH_MODEL_TYPE_CMD_NOOP, 0, 0);
	return addToQueue(CS_MESH_MODEL_TYPE_CMD_NOOP, 0, 0, nullptr, 0, repeats, false);
}

cs_ret_code_t MeshMsgSender::sendMultiSwitchItem(const internal_multi_switch_item_t* item, uint8_t repeats) {
	cs_mesh_model_msg_multi_switch_item_t meshItem;
	meshItem.id = item->id;
	meshItem.switchCmd = item->cmd.switchCmd;
	meshItem.delay = item->cmd.delay;
	meshItem.source = item->cmd.source;

	switch (item->cmd.source.sourceId) {
		case CS_CMD_SOURCE_CONNECTION:
		case CS_CMD_SOURCE_UART: {
			LOGMeshModelInfo("Source connection: set high repeat count");
			repeats = CS_MESH_RELIABILITY_HIGH;
			break;
		}
		default:
			break;
	}
	// Remove old messages of same type and with same target id.
	remFromQueue(CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH, 0, item->id);
	return addToQueue(CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH, 0, item->id, (uint8_t*)(&meshItem), sizeof(meshItem), repeats, true);
}

cs_ret_code_t MeshMsgSender::sendTime(const cs_mesh_model_msg_time_t* item, uint8_t repeats) {
	if (item->timestamp == 0) {
		return ERR_WRONG_PARAMETER;
	}
	// Remove old messages of same type, as only the latest is of interest.
	remFromQueue(CS_MESH_MODEL_TYPE_STATE_TIME, 0, 0);
	return addToQueue(CS_MESH_MODEL_TYPE_STATE_TIME, 0, 0, (uint8_t*)item, sizeof(*item), repeats, false);
}

cs_ret_code_t MeshMsgSender::sendBehaviourSettings(const behaviour_settings_t* item, uint8_t repeats) {
	// Remove old messages of same type, as only the latest is of interest.
	remFromQueue(CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS, 0, 0);
	return addToQueue(CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS, 0, 0, (uint8_t*)item, sizeof(*item), repeats, false);
}

cs_ret_code_t MeshMsgSender::sendProfileLocation(const cs_mesh_model_msg_profile_location_t* item, uint8_t repeats) {
	// Don't remove old messages of same type, as they may have other profile location combinations.
	// Remove old messages of same type, location, and profile.
	uint16_t id = (item->location << 8) + item->profile;
//	remFromQueue(CS_MESH_MODEL_TYPE_PROFILE_LOCATION, id);
	return addToQueue(CS_MESH_MODEL_TYPE_PROFILE_LOCATION, 0, id, (uint8_t*)item, sizeof(*item), repeats, false);
}

cs_ret_code_t MeshMsgSender::sendTrackedDeviceRegister(const cs_mesh_model_msg_device_register_t* item, uint8_t repeats) {
	// Remove old messages of same type, and device id, as only the latest register is of interest.
	remFromQueue(CS_MESH_MODEL_TYPE_TRACKED_DEVICE_REGISTER, 0, item->deviceId);
	return addToQueue(CS_MESH_MODEL_TYPE_TRACKED_DEVICE_REGISTER, 0, item->deviceId, (uint8_t*)item, sizeof(*item), repeats, false);
}

cs_ret_code_t MeshMsgSender::sendTrackedDeviceToken(const cs_mesh_model_msg_device_token_t* item, uint8_t repeats) {
	// Remove old messages of same type, and device id, as only the latest token is of interest.
	remFromQueue(CS_MESH_MODEL_TYPE_TRACKED_DEVICE_TOKEN, 0, item->deviceId);
	return addToQueue(CS_MESH_MODEL_TYPE_TRACKED_DEVICE_TOKEN, item->deviceId, 0, (uint8_t*)item, sizeof(*item), repeats, false);
}

cs_ret_code_t MeshMsgSender::sendTrackedDeviceListSize(const cs_mesh_model_msg_device_list_size_t* item, uint8_t repeats) {
	// Remove old messages of same type, as only the latest is of interest.
	remFromQueue(CS_MESH_MODEL_TYPE_TRACKED_DEVICE_LIST_SIZE, 0, 0);
	return addToQueue(CS_MESH_MODEL_TYPE_TRACKED_DEVICE_LIST_SIZE, 0, 0, (uint8_t*)item, sizeof(*item), repeats, false);
}

cs_ret_code_t MeshMsgSender::addToQueue(cs_mesh_model_msg_type_t type, stone_id_t targetId, uint16_t id, uint8_t* payload, uint8_t payloadSize, uint8_t repeats, bool priority) {
	assert(_selector != nullptr, "No model selector set.");
	MeshUtil::cs_mesh_queue_item_t item;
	item.metaData.id = id;
	item.metaData.type = type;
	item.metaData.targetId = targetId;
	item.metaData.priority = priority;
	item.metaData.reliable = false;
	item.metaData.repeats = repeats;
	item.payloadSize = payloadSize;
	item.payloadPtr = payload;
	return _selector->addToQueue(item);
}

cs_ret_code_t MeshMsgSender::remFromQueue(cs_mesh_model_msg_type_t type, stone_id_t targetId, uint16_t id) {
	assert(_selector != nullptr, "No model selector set.");
	MeshUtil::cs_mesh_queue_item_meta_data_t item;
	item.id = id;
	item.type = type;
	item.targetId = targetId;
//	item.priority =
//	item.reliable =
//	item.repeats =
	return _selector->remFromQueue(item);
}

void MeshMsgSender::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::CMD_SEND_MESH_MSG: {
			TYPIFY(CMD_SEND_MESH_MSG)* msg = (TYPIFY(CMD_SEND_MESH_MSG)*)event.data;
			sendMsg(msg);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_SET_TIME: {
			TYPIFY(CMD_SEND_MESH_MSG_SET_TIME)* packet = (TYPIFY(CMD_SEND_MESH_MSG_SET_TIME)*)event.data;
			cs_mesh_model_msg_time_t item;
			item.timestamp = *packet;
			sendSetTime(&item);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_NOOP: {
			sendNoop();
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_MULTI_SWITCH: {
			TYPIFY(CMD_SEND_MESH_MSG_MULTI_SWITCH)* packet = (TYPIFY(CMD_SEND_MESH_MSG_MULTI_SWITCH)*)event.data;
			sendMultiSwitchItem(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS: {
			TYPIFY(CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS)* packet = (TYPIFY(CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS)*)event.data;
			sendBehaviourSettings(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_PROFILE_LOCATION: {
			TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION)* packet = (TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION)*)event.data;
			LOGd("send profile=%u location=%u", packet->profile, packet->location);
			sendProfileLocation(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER: {
			LOGd("send tracked device register");
			TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER)* packet = (TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER)*)event.data;
			sendTrackedDeviceRegister(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN: {
			LOGd("send tracked device token");
			TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN)* packet = (TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN)*)event.data;
			sendTrackedDeviceToken(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE: {
			LOGd("send tracked device list size");
			TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE)* packet = (TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE)*)event.data;
			sendTrackedDeviceListSize(packet);
			break;
		}
		case CS_TYPE::CMD_SEND_MESH_CONTROL_COMMAND: {

			break;
		}
		default:
			break;
	}
}
