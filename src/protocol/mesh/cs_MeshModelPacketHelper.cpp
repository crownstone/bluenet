/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "protocol/mesh/cs_MeshModelPacketHelper.h"
#include "drivers/cs_Serial.h"
#include <cstring> // For memcpy

#define LOGMeshModelPacketHelperDebug LOGd

namespace MeshModelPacketHelper {

bool isValidMeshMessage(uint8_t* meshMsg, size16_t msgSize) {
	if (msgSize < MESH_HEADER_SIZE) {
		return false;
	}
	uint8_t* payload = NULL;
	size16_t payloadSize;
	getPayload(meshMsg, msgSize, payload, payloadSize);
	LOGMeshModelPacketHelperDebug("meshMsg=%p size=%u type=%u payload=%p size=%u", meshMsg, msgSize, getType(meshMsg), payload, payloadSize);
	switch (getType(meshMsg)) {
	case CS_MESH_MODEL_TYPE_STATE_TIME:
		return timeIsValid((cs_mesh_model_msg_time_t*)payload, payloadSize);
	case CS_MESH_MODEL_TYPE_CMD_TIME:
		return timeIsValid((cs_mesh_model_msg_time_t*)payload, payloadSize);
	case CS_MESH_MODEL_TYPE_CMD_NOOP:
		return noopIsValid(payload, payloadSize);
	case CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH:
		return multiSwitchIsValid((cs_mesh_model_msg_multi_switch_t*)payload, payloadSize);
	case CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE:
		return keepAliveIsValid((cs_mesh_model_msg_keep_alive_t*)payload, payloadSize);
	}
	return false;
}

bool timeIsValid(const cs_mesh_model_msg_time_t* msg, size16_t msgSize) {
	return msgSize == sizeof(cs_mesh_model_msg_time_t);
}

bool noopIsValid(const uint8_t* msg, size16_t msgSize) {
	return msgSize == 0;
}

bool multiSwitchIsValid(const cs_mesh_model_msg_multi_switch_t* msg, size16_t msgSize) {
	if (msgSize < MESH_MESH_MULTI_SWITCH_HEADER_SIZE) {
		LOGMeshModelPacketHelperDebug("size=%u < header=%u", msgSize, MESH_MESH_MULTI_SWITCH_HEADER_SIZE);
		return false;
	}
	if (msg->type != 0) {
		LOGMeshModelPacketHelperDebug("type=%u != 0", msg->type);
		return false;
	}
	if (msg->count > MESH_MESH_MULTI_SWITCH_MAX_ITEM_COUNT) {
		LOGMeshModelPacketHelperDebug("count=%u > max=%u", msg->count, MESH_MESH_MULTI_SWITCH_MAX_ITEM_COUNT);
		return false;
	}
	if (msgSize < MESH_MESH_MULTI_SWITCH_HEADER_SIZE + msg->count * sizeof(cs_mesh_model_msg_multi_switch_item_t)) {
		LOGMeshModelPacketHelperDebug("size=%u < header=%u + count=%u * itemSize=%u", msgSize, MESH_MESH_MULTI_SWITCH_HEADER_SIZE, msg->count, sizeof(cs_mesh_model_msg_multi_switch_item_t));
		return false;
	}
	return true;
}

bool keepAliveIsValid(const cs_mesh_model_msg_keep_alive_t* msg, size16_t msgSize) {
	if (msgSize < MESH_MESH_KEEP_ALIVE_HEADER_SIZE) {
		LOGMeshModelPacketHelperDebug("size=%u < header=%u", msgSize, MESH_MESH_KEEP_ALIVE_HEADER_SIZE);
		return false;
	}
	if (msg->type != 1) {
		LOGMeshModelPacketHelperDebug("type=%u != 1", msg->type);
		return false;
	}
	if (msg->timeout == 0) {
		LOGMeshModelPacketHelperDebug("timeout = 0");
		return false;
	}
	if (msg->count > MESH_MESH_KEEP_ALIVE_MAX_ITEM_COUNT) {
		LOGMeshModelPacketHelperDebug("count=%u > max=%u", msg->count, MESH_MESH_KEEP_ALIVE_MAX_ITEM_COUNT);
		return false;
	}
	if (msgSize < MESH_MESH_KEEP_ALIVE_HEADER_SIZE + msg->count * sizeof(cs_mesh_model_msg_keep_alive_item_t)) {
		LOGMeshModelPacketHelperDebug("size=%u < header=%u + count=%u * itemSize=%u", msgSize, MESH_MESH_KEEP_ALIVE_HEADER_SIZE, msg->count, sizeof(cs_mesh_model_msg_keep_alive_item_t));
		return false;
	}
	return true;
}

cs_mesh_model_msg_type_t getType(const uint8_t* meshMsg) {
	return (cs_mesh_model_msg_type_t)meshMsg[0];
}

void getPayload(uint8_t* meshMsg, size16_t meshMsgSize, uint8_t*& payload, size16_t& payloadSize) {
	payload = meshMsg + MESH_HEADER_SIZE;
	payloadSize = meshMsgSize - MESH_HEADER_SIZE;
}

size16_t getMeshMessageSize(size16_t payloadSize) {
	return MESH_HEADER_SIZE + payloadSize;
}

bool setMeshMessage(cs_mesh_model_msg_type_t type, const uint8_t* payload, size16_t payloadSize, uint8_t* meshMsg, size16_t& meshMsgSize) {
	if (meshMsgSize < getMeshMessageSize(payloadSize)) {
		return false;
	}
	meshMsg[0] = type;
	if (payloadSize) {
		memcpy(meshMsg + MESH_HEADER_SIZE, payload, payloadSize);
	}
	meshMsgSize = getMeshMessageSize(payloadSize);
	return true;
}

bool multiSwitchHasItem(cs_mesh_model_msg_multi_switch_t* packet, stone_id_t stoneId, cs_mesh_model_msg_multi_switch_item_t*& item) {
	bool found = false;
	for (int i=0; i<packet->count; ++i) {
		if (packet->items[i].id == stoneId) {
			item = &(packet->items[i]);
			found = true;
		}
	}
	return found;
}

bool keepAliveHasItem(cs_mesh_model_msg_keep_alive_t* packet, stone_id_t stoneId, cs_mesh_model_msg_keep_alive_item_t*& item) {
	bool found = false;
	for (int i=0; i<packet->count; ++i) {
		if (packet->items[i].id == stoneId) {
			item = &(packet->items[i]);
			found = true;
		}
	}
	return found;
}

} // namespace CSMeshModel
