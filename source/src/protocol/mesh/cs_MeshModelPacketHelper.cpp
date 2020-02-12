/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "protocol/mesh/cs_MeshModelPacketHelper.h"
#include "drivers/cs_Serial.h"
#include <cstring> // For memcpy

#define LOGMeshModelPacketHelperDebug LOGnone

namespace MeshModelPacketHelper {

bool isValidMeshMessage(cs_mesh_msg_t* meshMsg) {
	if (meshMsg->reliability == CS_MESH_RELIABILITY_INVALID || meshMsg->size > MAX_MESH_MSG_NON_SEGMENTED_SIZE - MESH_HEADER_SIZE) {
		return false;
	}
	return isValidMeshPayload(meshMsg->type, meshMsg->payload, meshMsg->size);
}

bool isValidMeshMessage(uint8_t* meshMsg, size16_t msgSize) {
	if (msgSize < MESH_HEADER_SIZE) {
		return false;
	}
	uint8_t* payload = NULL;
	size16_t payloadSize;
	getPayload(meshMsg, msgSize, payload, payloadSize);
	LOGMeshModelPacketHelperDebug("meshMsg=%p size=%u type=%u payload=%p size=%u", meshMsg, msgSize, getType(meshMsg), payload, payloadSize);
	return isValidMeshPayload(getType(meshMsg), payload, payloadSize);
}

bool isValidMeshPayload(cs_mesh_model_msg_type_t type, uint8_t* payload, size16_t payloadSize) {
	switch (type) {
		case CS_MESH_MODEL_TYPE_TEST:
			return testIsValid((cs_mesh_model_msg_test_t*)payload, payloadSize);
		case CS_MESH_MODEL_TYPE_ACK:
			return ackIsValid(payload, payloadSize);
		case CS_MESH_MODEL_TYPE_STATE_TIME:
			return timeIsValid((cs_mesh_model_msg_time_t*)payload, payloadSize);
		case CS_MESH_MODEL_TYPE_CMD_TIME:
			return timeIsValid((cs_mesh_model_msg_time_t*)payload, payloadSize);
		case CS_MESH_MODEL_TYPE_CMD_NOOP:
			return noopIsValid(payload, payloadSize);
		case CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH:
	//		return multiSwitchIsValid((cs_mesh_model_msg_multi_switch_t*)payload, payloadSize);
			return cs_multi_switch_item_is_valid((internal_multi_switch_item_t*)payload, payloadSize);
		case CS_MESH_MODEL_TYPE_STATE_0:
			return state0IsValid((cs_mesh_model_msg_state_0_t*)payload, payloadSize);
		case CS_MESH_MODEL_TYPE_STATE_1:
			return state1IsValid((cs_mesh_model_msg_state_1_t*)payload, payloadSize);
		case CS_MESH_MODEL_TYPE_PROFILE_LOCATION:
			return profileLocationIsValid((cs_mesh_model_msg_profile_location_t*)payload, payloadSize);
		case CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS:
			return setBehaviourSettingsIsValid((behaviour_settings_t*)payload, payloadSize);
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_REGISTER:
			return payloadSize == sizeof(cs_mesh_model_msg_device_register_t);
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_TOKEN:
			return payloadSize == sizeof(cs_mesh_model_msg_device_token_t);
		case CS_MESH_MODEL_TYPE_SYNC_REQUEST:
			return payloadSize == sizeof(cs_mesh_model_msg_sync_request_t);
		case CS_MESH_MODEL_TYPE_SYNC_RESPONSE:
			return payloadSize == sizeof(cs_mesh_model_msg_sync_response_t);
	}

	return false;
}

bool testIsValid(const cs_mesh_model_msg_test_t* packet, size16_t size) {
	return size == sizeof(cs_mesh_model_msg_test_t);
}

bool ackIsValid(const uint8_t* packet, size16_t size) {
	return size == 0;
}

bool timeIsValid(const cs_mesh_model_msg_time_t* packet, size16_t size) {
	return size == sizeof(cs_mesh_model_msg_time_t);
}

bool noopIsValid(const uint8_t* packet, size16_t size) {
	return size == 0;
}

bool state0IsValid(const cs_mesh_model_msg_state_0_t* packet, size16_t size) {
	return size == sizeof(cs_mesh_model_msg_state_0_t);
}

bool state1IsValid(const cs_mesh_model_msg_state_1_t* packet, size16_t size) {
	return size == sizeof(cs_mesh_model_msg_state_1_t);
}

bool profileLocationIsValid(const cs_mesh_model_msg_profile_location_t* packet, size16_t size) {
	return size == sizeof(cs_mesh_model_msg_profile_location_t);
}

bool setBehaviourSettingsIsValid(const behaviour_settings_t* packet, size16_t size) {
	return size == sizeof(behaviour_settings_t);
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

} // namespace CSMeshModel
