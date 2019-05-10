/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "protocol/mesh/cs_MeshModelPacketHelper.h"
#include <cstring> // For memcpy

namespace CSMeshModel {

bool multiSwitchIsValid(const cs_mesh_model_msg_multi_switch_t* msg, size16_t msgSize) {
	if (msgSize < MESH_MESH_MULTI_SWITCH_HEADER_SIZE) {
		return false;
	}
	if (msg->type != 0) {
		return false;
	}
	if (msg->count > MESH_MESH_MULTI_SWITCH_MAX_ITEM_COUNT) {
		return false;
	}
	if (MESH_MESH_MULTI_SWITCH_HEADER_SIZE + msg->count * sizeof(cs_mesh_model_msg_multi_switch_item_t) > msgSize) {
		return false;
	}
	return true;
}

bool keepAliveIsValid(const cs_mesh_model_msg_keep_alive_t* msg, size16_t msgSize) {
	if (msgSize < MESH_MESH_KEEP_ALIVE_HEADER_SIZE) {
		return false;
	}
	if (msg->type != 1) {
		return false;
	}
	if (msg->timeout == 0) {
		return false;
	}
	if (msg->count > MESH_MESH_KEEP_ALIVE_MAX_ITEM_COUNT) {
		return false;
	}
	if (MESH_MESH_KEEP_ALIVE_HEADER_SIZE + msg->count * sizeof(cs_mesh_model_msg_keep_alive_item_t) > msgSize) {
		return false;
	}
	return true;
}

size16_t getMeshMessageSize(cs_mesh_model_msg_type_t type) {
	switch (type) {
		case CS_MESH_MODEL_TYPE_STATE_TIME:
		case CS_MESH_MODEL_TYPE_CMD_TIME:
			return MESH_HEADER_SIZE + sizeof(cs_mesh_model_msg_time_t);
		case CS_MESH_MODEL_TYPE_CMD_NOOP:
			return MESH_HEADER_SIZE + 0;
		case CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH:
			return MESH_HEADER_SIZE + sizeof(cs_mesh_model_msg_multi_switch_t);
		case CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE:
			return MESH_HEADER_SIZE + sizeof(cs_mesh_model_msg_keep_alive_t);
		}
	return 0;
}

bool setMeshMessage(cs_mesh_model_msg_type_t type, const uint8_t* payload, size16_t payloadSize, uint8_t* meshMsg, size16_t& meshMsgSize) {
	if (meshMsgSize < getMeshMessageSize(type)) {
		return false;
	}
	meshMsg[0] = type;
	if (payloadSize) {
		memcpy(meshMsg + MESH_HEADER_SIZE, payload, payloadSize);
	}
	meshMsgSize = getMeshMessageSize(type);
	return true;
}

bool multiSwitchHasItem(cs_mesh_model_msg_multi_switch_t* packet, stone_id_t stoneId, cs_mesh_model_msg_multi_switch_item_t* item) {
	bool found = false;
	for (int i=0; i<packet->count; ++i) {
		if (packet->items[i].id == stoneId) {
			item = &(packet->items[i]);
			found = true;
		}
	}
	return found;
}

bool keepAliveHasItem(cs_mesh_model_msg_keep_alive_t* packet, stone_id_t stoneId, cs_mesh_model_msg_keep_alive_item_t* item) {
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
