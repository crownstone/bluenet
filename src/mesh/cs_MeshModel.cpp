/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "mesh/cs_MeshModel.h"
#include "protocol/mesh/cs_MeshModelPackets.h"
#include "protocol/mesh/cs_MeshModelPacketHelper.h"
#include "protocol/cs_CommandTypes.h"
#include "cfg/cs_Config.h"
#include "common/cs_Types.h"
#include "drivers/cs_Serial.h"
#include "events/cs_EventDispatcher.h"
#include "storage/cs_State.h"
#include "util/cs_Utils.h"
#include "util/cs_BleError.h"
#include <cstddef> // For NULL

extern "C" {
#include "access.h"
#include "access_config.h"
#include "nrf_mesh.h"
}

static void cs_mesh_model_msg_handler(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args) {
	MeshModel* meshModel = (MeshModel*)p_args;
	meshModel->handleMsg(p_message);
}

static const access_opcode_handler_t cs_mesh_model_opcode_handlers[] = {
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_MSG, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler}
};



void MeshModel::init() {
	uint32_t retVal;
	access_model_add_params_t accessParams;
	accessParams.model_id.company_id = CROWNSTONE_COMPANY_ID;
	accessParams.model_id.model_id = 0;
	accessParams.element_index = 0;
	accessParams.p_opcode_handlers = cs_mesh_model_opcode_handlers;
	accessParams.opcode_count = 1;
	accessParams.p_args = this;
	accessParams.publish_timeout_cb = NULL;
	retVal = access_model_add(&accessParams, &_accessHandle);
	APP_ERROR_CHECK(retVal);
	retVal = access_model_subscription_list_alloc(_accessHandle);
	APP_ERROR_CHECK(retVal);
}

access_model_handle_t MeshModel::getAccessModelHandle() {
	return _accessHandle;
}

/**
 * Send a message over the mesh via publish, without reply.
 * TODO: wait for NRF_MESH_EVT_TX_COMPLETE before sending next msg (in case of segmented msg?).
 * TODO: repeat publishing the msg.
 */
cs_ret_code_t MeshModel::sendMsg(const uint8_t* data, uint16_t len, uint8_t repeats) {
	access_message_tx_t accessMsg;
	accessMsg.opcode.company_id = CROWNSTONE_COMPANY_ID;
	accessMsg.opcode.opcode = CS_MESH_MODEL_OPCODE_MSG;
	accessMsg.p_buffer = data;
	accessMsg.length = len;
	accessMsg.force_segmented = false;
	accessMsg.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	uint32_t status = NRF_SUCCESS;
	for (int i=0; i<repeats; ++i) {
		accessMsg.access_token = nrf_mesh_unique_token_get();
		status = access_model_publish(_accessHandle, &accessMsg);
		if (status != NRF_SUCCESS) {
			break;
		}
	}
	return status;
}

void MeshModel::handleMsg(const access_message_rx_t * accessMsg) {
	LOGi("Handle mesh msg. appkey=%u subnet=%u ttl=%u rssi=%i", accessMsg->meta_data.appkey_handle, accessMsg->meta_data.subnet_handle, accessMsg->meta_data.ttl, getRssi(accessMsg->meta_data.p_core_metadata));
	printMeshAddress("  Src: ", &(accessMsg->meta_data.src));
	printMeshAddress("  Dst: ", &(accessMsg->meta_data.dst));
	LOGi("  Data:")
	uint8_t* msg = (uint8_t*)(accessMsg->p_data);
	uint16_t size = accessMsg->length;
	BLEutil::printArray(msg, size);
	if (!CSMeshModel::isValidMeshMessage(msg, size)) {
		LOGw("Invalid mesh message");
		return;
	}
	cs_mesh_model_msg_type_t msgType = CSMeshModel::getType(msg);
	uint8_t* payload = NULL;
	size16_t payloadSize;
	CSMeshModel::getPayload(msg, size, payload, payloadSize);

	switch (msgType) {
	case CS_MESH_MODEL_TYPE_STATE_TIME: {
		event_t event(CS_TYPE::EVT_MESH_TIME, payload, payloadSize);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_TIME: {
		event_t event(CS_TYPE::CMD_SET_TIME, payload, payloadSize);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_NOOP: {
		TYPIFY(CMD_CONTROL_CMD) streamHeader;
		streamHeader.type = CTRL_CMD_NOP;
		streamHeader.length = 0;
		event_t event(CS_TYPE::CMD_CONTROL_CMD, &streamHeader, sizeof(streamHeader));
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH: {
		cs_mesh_model_msg_multi_switch_t* packet = (cs_mesh_model_msg_multi_switch_t*)payload;
		TYPIFY(CONFIG_CROWNSTONE_ID) myId;
		State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &myId, sizeof(myId));
		cs_mesh_model_msg_multi_switch_item_t* item = NULL;
		if (CSMeshModel::multiSwitchHasItem(packet, myId, item)) {
			event_t event(CS_TYPE::CMD_MULTI_SWITCH, item, sizeof(*item));
			EventDispatcher::getInstance().dispatch(event);
		}
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE: {
		cs_mesh_model_msg_keep_alive_t* packet = (cs_mesh_model_msg_keep_alive_t*)payload;
		TYPIFY(CONFIG_CROWNSTONE_ID) myId;
		State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &myId, sizeof(myId));
		cs_mesh_model_msg_keep_alive_item_t* item = NULL;
		if (CSMeshModel::keepAliveHasItem(packet, myId, item)) {
			keep_alive_state_message_payload_t keepAlive;
			if (item->actionSwitchCmd == 255) {
				keepAlive.action = NO_CHANGE;
				keepAlive.switchState.switchState = 0;
			}
			else {
				keepAlive.action = CHANGE;
				keepAlive.switchState.switchState = item->actionSwitchCmd;
			}
			keepAlive.timeout = packet->timeout;
			event_t event(CS_TYPE::EVT_KEEP_ALIVE, &keepAlive, sizeof(keepAlive));
			EventDispatcher::getInstance().dispatch(event);
		}
		break;
	}
	}
}

int8_t MeshModel::getRssi(const nrf_mesh_rx_metadata_t* metaData) {
	switch (metaData->source) {
	case NRF_MESH_RX_SOURCE_SCANNER:
		return metaData->params.scanner.rssi;
	case NRF_MESH_RX_SOURCE_GATT:
		// TODO: return connection rssi?
		return -10;
	case NRF_MESH_RX_SOURCE_FRIEND:
		// TODO: is this correct?
		return metaData->params.scanner.rssi;
		break;
	case NRF_MESH_RX_SOURCE_LOW_POWER:
		// TODO: is this correct?
		return metaData->params.scanner.rssi;
		break;
	case NRF_MESH_RX_SOURCE_INSTABURST:
		return metaData->params.instaburst.rssi;
		break;
	case NRF_MESH_RX_SOURCE_LOOPBACK:
		return -1;
		break;
	}
	return 0;
}

void MeshModel::printMeshAddress(const char* prefix, const nrf_mesh_address_t* addr) {
	switch (addr->type) {
	case NRF_MESH_ADDRESS_TYPE_INVALID:
		LOGd("%s type=invalid", prefix);
		break;
	case NRF_MESH_ADDRESS_TYPE_UNICAST:
		LOGd("%s type=unicast id=%u", prefix, addr->value);
		break;
	case NRF_MESH_ADDRESS_TYPE_VIRTUAL:{
		//128-bit virtual label UUID,
		uint32_t* uuid1 = (uint32_t*)(addr->p_virtual_uuid);
		uint32_t* uuid2 = (uint32_t*)(addr->p_virtual_uuid + 4);
		uint32_t* uuid3 = (uint32_t*)(addr->p_virtual_uuid + 8);
		uint32_t* uuid4 = (uint32_t*)(addr->p_virtual_uuid + 12);
		LOGd("%s type=virtual id=%u uuid=%x%x%x%x", prefix, addr->value, uuid1, uuid2, uuid3, uuid4);
		break;
	}
	case NRF_MESH_ADDRESS_TYPE_GROUP:
		LOGd("%s type=group id=%u", prefix, addr->value);
		break;
	}
}
