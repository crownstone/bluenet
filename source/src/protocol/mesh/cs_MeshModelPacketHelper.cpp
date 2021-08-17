/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cstring> // For memcpy
#include <localisation/cs_Nearestnearestwitnessreport.h>
#include <logging/cs_Logger.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>

#define LOGMeshModelPacketHelperDebug LOGnone
#define LOGMeshModelPacketHelperWarn LOGw

namespace MeshUtil {

bool isValidMeshMessage(cs_mesh_msg_t* meshMsg) {
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
		case CS_MESH_MODEL_TYPE_CMD_TIME:
			return timeIsValid((cs_mesh_model_msg_time_t*)payload, payloadSize);
		case CS_MESH_MODEL_TYPE_CMD_NOOP:
			return noopIsValid(payload, payloadSize);
		case CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH:
			return multiSwitchIsValid(payload, payloadSize);
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
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_HEARTBEAT:
			return payloadSize == sizeof(cs_mesh_model_msg_device_heartbeat_t);
		case CS_MESH_MODEL_TYPE_TRACKED_DEVICE_LIST_SIZE:
			return payloadSize == sizeof(cs_mesh_model_msg_device_list_size_t);
		case CS_MESH_MODEL_TYPE_SYNC_REQUEST:
			return payloadSize == sizeof(cs_mesh_model_msg_sync_request_t);
		case CS_MESH_MODEL_TYPE_STATE_SET:
			return payloadSize >= sizeof(cs_mesh_model_msg_state_header_ext_t);
		case CS_MESH_MODEL_TYPE_RESULT:
			return payloadSize >= sizeof(cs_mesh_model_msg_result_header_t);
		case CS_MESH_MODEL_TYPE_SET_IBEACON_CONFIG_ID:
			return payloadSize >= sizeof(set_ibeacon_config_id_packet_t);
		case CS_MESH_MODEL_TYPE_RSSI_PING:
			return payloadSize >= sizeof(rssi_ping_message_t);
		case CS_MESH_MODEL_TYPE_RSSI_DATA:
			return payloadSize >= sizeof(rssi_data_message_t);
		case CS_MESH_MODEL_TYPE_TIME_SYNC:
			return payloadSize == sizeof(cs_mesh_model_msg_time_sync_t);
		case CS_MESH_MODEL_TYPE_REPORT_ASSET_MAC:
			return payloadSize == sizeof(report_asset_mac_t);
		case CS_MESH_MODEL_TYPE_REPORT_ASSET_ID:
			return payloadSize == sizeof(report_asset_id_t);
		case CS_MESH_MODEL_TYPE_STONE_MAC:
			return payloadSize == sizeof(cs_mesh_model_msg_stone_mac_t);
		case CS_MESH_MODEL_TYPE_ASSET_FILTER_VERSION:
			return payloadSize == sizeof(cs_mesh_model_msg_asset_filter_version_t);
		case CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC:
			return payloadSize == sizeof(cs_mesh_model_msg_asset_rssi_mac_t);
		case CS_MESH_MODEL_TYPE_ASSET_RSSI_SID:
			return payloadSize == sizeof(cs_mesh_model_msg_asset_rssi_sid_t);
		case CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI:
			return payloadSize == sizeof(cs_mesh_model_msg_neighbour_rssi_t);
		case CS_MESH_MODEL_TYPE_CTRL_CMD:
			return payloadSize >= sizeof(cs_mesh_model_msg_ctrl_cmd_header_t);
		case CS_MESH_MODEL_TYPE_UNKNOWN:
			return false;
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

bool multiSwitchIsValid(const uint8_t* packet, size16_t size) {
	return (size == sizeof(cs_mesh_model_msg_multi_switch_item_t)) && (((cs_mesh_model_msg_multi_switch_item_t*)packet)->id != 0);
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

cs_data_t getPayload(uint8_t* meshMsg, size16_t meshMsgSize) {
	return cs_data_t(meshMsg + MESH_HEADER_SIZE, meshMsgSize - MESH_HEADER_SIZE);
}

size16_t getMeshMessageSize(size16_t payloadSize) {
	return MESH_HEADER_SIZE + payloadSize;
}

bool setMeshMessage(cs_mesh_model_msg_type_t type, const uint8_t* payload, size16_t payloadSize, uint8_t* meshMsg, size16_t meshMsgSize) {
	if (meshMsgSize < getMeshMessageSize(payloadSize)) {
		return false;
	}
	meshMsg[0] = type;
	if (payloadSize) {
		memcpy(meshMsg + MESH_HEADER_SIZE, payload, payloadSize);
	}
	return true;
}



CommandHandlerTypes getCtrlCmdType(cs_mesh_model_msg_type_t meshType) {
	switch (meshType) {
		case CS_MESH_MODEL_TYPE_CMD_TIME:
			return CTRL_CMD_SET_TIME;
		case CS_MESH_MODEL_TYPE_STATE_SET:
			return CTRL_CMD_STATE_SET;
		case CS_MESH_MODEL_TYPE_SET_IBEACON_CONFIG_ID:
			return CTRL_CMD_SET_IBEACON_CONFIG_ID;
		default:
			return CTRL_CMD_UNKNOWN;
	}
}



bool canShortenStateType(uint16_t type) {
	return type < 0xFF;
}

bool canShortenStateId(uint16_t id) {
	return id < ((1 << 6) - 1);
}

bool canShortenPersistenceMode(uint8_t id) {
	return id < ((1 << 2) - 1);
}

bool canShortenRetCode(cs_ret_code_t retCode) {
	return retCode < 0xFF;
}

uint8_t getShortenedRetCode(cs_ret_code_t retCode) {
	if (retCode > 0xFF) {
		return 0xFF;
	}
	return retCode;
}

cs_ret_code_t getInflatedRetCode(uint8_t retCode) {
	if (retCode == 0xFF) {
		return ERR_UNSPECIFIED;
	}
	return retCode;
}

bool canShortenAccessLevel(EncryptionAccessLevel accessLevel) {
	// 3 bits --> max 7
	switch (accessLevel) {
		case ADMIN:
		case MEMBER:
		case BASIC:
		case SETUP:
			return true;
		case SERVICE_DATA:
		case LOCALIZATION:
		case NOT_SET:
		case ENCRYPTION_DISABLED:
		case NO_ONE:
			return false;
	}
	return false;
}

uint8_t getShortenedAccessLevel(EncryptionAccessLevel accessLevel) {
	// 3 bits --> max 7
	switch (accessLevel) {
		case ADMIN:
		case MEMBER:
		case BASIC:
			return (uint8_t) accessLevel;
		case SETUP:
			return 6;
		case SERVICE_DATA:
		case LOCALIZATION:
		case NOT_SET:
		case ENCRYPTION_DISABLED:
		case NO_ONE:
			break;
	}
	return 7;
}

EncryptionAccessLevel getInflatedAccessLevel(uint8_t accessLevel) {
	switch (accessLevel) {
		case 0:
		case 1:
		case 2:
			return (EncryptionAccessLevel) accessLevel;
		case 6:
			return SETUP;
		default:
			return NOT_SET;
	}
}

bool canShortenSource(const cmd_source_with_counter_t& source) {
	// 5 bits -- max 31
	switch (source.source.type) {
		case CS_CMD_SOURCE_TYPE_ENUM: {
			switch (source.source.id) {
				case CS_CMD_SOURCE_NONE:
				case CS_CMD_SOURCE_INTERNAL:
//				case CS_CMD_SOURCE_UART:
				case CS_CMD_SOURCE_CONNECTION:
				case CS_CMD_SOURCE_SWITCHCRAFT:
					return true;
				default:
					return false;
			}
			break;
		}
		case CS_CMD_SOURCE_TYPE_BROADCAST:
			return true;
		case CS_CMD_SOURCE_TYPE_UART:
			return true;
		default:
			return false;
	}
}

uint8_t getShortenedSource(const cmd_source_with_counter_t& source) {
	// 5 bits --> max 31
	switch (source.source.type) {
		case CS_CMD_SOURCE_TYPE_ENUM: {
			switch (source.source.id) {
				case CS_CMD_SOURCE_NONE:
				case CS_CMD_SOURCE_INTERNAL:
//				case CS_CMD_SOURCE_UART:
				case CS_CMD_SOURCE_CONNECTION:
				case CS_CMD_SOURCE_SWITCHCRAFT:
					return source.source.id;
				default:
					return CS_CMD_SOURCE_NONE;
			}
			break;
		}
		case CS_CMD_SOURCE_TYPE_UART:
			return 29;
		case CS_CMD_SOURCE_TYPE_BROADCAST: {
			return 30;
		}
		default:
			return CS_CMD_SOURCE_NONE;
	}
}

cmd_source_with_counter_t getInflatedSource(uint8_t sourceId) {
	cmd_source_with_counter_t source;
	source.source.flagExternal = true;
	switch (sourceId) {
		case CS_CMD_SOURCE_NONE:
		case CS_CMD_SOURCE_INTERNAL:
//		case CS_CMD_SOURCE_UART:
		case CS_CMD_SOURCE_CONNECTION:
		case CS_CMD_SOURCE_SWITCHCRAFT:
			source.source.type = CS_CMD_SOURCE_TYPE_ENUM;
			source.source.id = sourceId;
			break;
		case 29:
			// We can't reconstruct the device id, nor the counter.
			// So let's just set the default.
			source.source.type = CS_CMD_SOURCE_TYPE_UART;
			source.source.id = 0;
			break;
		case 30:
			// We can't reconstruct the device id, nor the counter.
			// So let's just set the default.
			source.source.type = CS_CMD_SOURCE_TYPE_BROADCAST;
			source.source.id = 0;
			break;
		default:
			source.source.type = CS_CMD_SOURCE_TYPE_ENUM;
			source.source.id = CS_CMD_SOURCE_NONE;
			break;
	}
	return source;
}

}
