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
#include "access_reliable.h"
#include "nrf_mesh.h"
}

#define LOGMeshModelDebug LOGnone


static void cs_mesh_model_msg_handler(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args) {
	MeshModel* meshModel = (MeshModel*)p_args;
	meshModel->handleMsg(p_message);
}

static void cs_mesh_model_reply_status_handler(access_model_handle_t model_handle, void * p_args, access_reliable_status_t status) {
	switch (status) {
	case ACCESS_RELIABLE_TRANSFER_SUCCESS:
		LOGMeshModelDebug("reliable msg success");
		break;
	case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
		LOGw("reliable msg timeout");
		break;
	case ACCESS_RELIABLE_TRANSFER_CANCELLED:
		LOGi("reliable msg cancelled");
		break;
	}
}

static const access_opcode_handler_t cs_mesh_model_opcode_handlers[] = {
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_MSG, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_RELIABLE_MSG, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_REPLY, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler},
};



void MeshModel::init() {
	uint32_t retVal;
	access_model_add_params_t accessParams;
	accessParams.model_id.company_id = CROWNSTONE_COMPANY_ID;
	accessParams.model_id.model_id = 0;
	accessParams.element_index = 0;
	accessParams.p_opcode_handlers = cs_mesh_model_opcode_handlers;
	accessParams.opcode_count = (sizeof(cs_mesh_model_opcode_handlers) / sizeof((cs_mesh_model_opcode_handlers)[0]));
	accessParams.p_args = this;
	accessParams.publish_timeout_cb = NULL;
	retVal = access_model_add(&accessParams, &_accessHandle);
	APP_ERROR_CHECK(retVal);
	retVal = access_model_subscription_list_alloc(_accessHandle);
	APP_ERROR_CHECK(retVal);
}

void MeshModel::setOwnAddress(uint16_t address) {
	_ownAddress = address;
}

access_model_handle_t MeshModel::getAccessModelHandle() {
	return _accessHandle;
}

/**
 * Send a message over the mesh via publish, without reply.
 * TODO: wait for NRF_MESH_EVT_TX_COMPLETE before sending next msg (in case of segmented msg?).
 * TODO: repeat publishing the msg.
 */
cs_ret_code_t MeshModel::sendMsg(const uint8_t* data, uint16_t len, uint8_t repeats, cs_mesh_model_opcode_t opcode) {
	access_message_tx_t accessMsg;
	accessMsg.opcode.company_id = CROWNSTONE_COMPANY_ID;
	accessMsg.opcode.opcode = opcode;
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

cs_ret_code_t MeshModel::sendReply(const access_message_rx_t * accessMsg) {
	size16_t msgSize = MESH_HEADER_SIZE;
	uint8_t msg[msgSize];
	MeshModelPacketHelper::setMeshMessage(CS_MESH_MODEL_TYPE_ACK, NULL, 0, msg, msgSize);

	access_message_tx_t replyMsg;
	replyMsg.opcode.company_id = CROWNSTONE_COMPANY_ID;
	replyMsg.opcode.opcode = CS_MESH_MODEL_OPCODE_REPLY;
	replyMsg.p_buffer = msg;
	replyMsg.length = msgSize;
	replyMsg.force_segmented = false;
	replyMsg.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	replyMsg.access_token = nrf_mesh_unique_token_get();

	uint32_t retVal = access_model_reply(_accessHandle, accessMsg, &replyMsg);
	LOGMeshModelDebug("send reply %u", retVal);
	return retVal;
}

cs_ret_code_t MeshModel::sendReliableMsg(const uint8_t* data, uint16_t len) {
	if (!access_reliable_model_is_free(_accessHandle)) {
		return ERR_BUSY;
	}
//	access_reliable_t accessReliableMsg;
	memcpy(_reliableMsg, data, len); // Data has to be valid until reply is received.
	access_message_tx_t* accessMsg = &(_accessReliableMsg.message);
	accessMsg->opcode.company_id = CROWNSTONE_COMPANY_ID;
	accessMsg->opcode.opcode = CS_MESH_MODEL_OPCODE_RELIABLE_MSG;
	accessMsg->p_buffer = _reliableMsg;
	accessMsg->length = len;
	accessMsg->force_segmented = false;
	accessMsg->transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	accessMsg->access_token = nrf_mesh_unique_token_get();

	_accessReliableMsg.model_handle = _accessHandle;
	_accessReliableMsg.reply_opcode.company_id = CROWNSTONE_COMPANY_ID;
	_accessReliableMsg.reply_opcode.opcode = CS_MESH_MODEL_OPCODE_REPLY;
	_accessReliableMsg.status_cb = cs_mesh_model_reply_status_handler;
	// Reliable message timeout in microseconds.
	_accessReliableMsg.timeout = 5000000; // MODEL_ACKNOWLEDGED_TRANSACTION_TIMEOUT
	return access_model_reliable_publish(&_accessReliableMsg);
}

void MeshModel::handleMsg(const access_message_rx_t * accessMsg) {
	if (accessMsg->meta_data.p_core_metadata->source != NRF_MESH_RX_SOURCE_LOOPBACK) {
		LOGMeshModelDebug("Handle mesh msg. opcode=%u appkey=%u subnet=%u ttl=%u rssi=%i", accessMsg->opcode.opcode, accessMsg->meta_data.appkey_handle, accessMsg->meta_data.subnet_handle, accessMsg->meta_data.ttl, getRssi(accessMsg->meta_data.p_core_metadata));
		printMeshAddress("  Src: ", &(accessMsg->meta_data.src));
		printMeshAddress("  Dst: ", &(accessMsg->meta_data.dst));
		LOGMeshModelDebug("ownAddress=%u  Data:", _ownAddress);
//		BLEutil::printArray(msg, size);
	}
//	bool ownMsg = (_ownAddress == accessMsg->meta_data.src.value) && (accessMsg->meta_data.src.type == NRF_MESH_ADDRESS_TYPE_UNICAST);
	bool ownMsg = accessMsg->meta_data.p_core_metadata->source == NRF_MESH_RX_SOURCE_LOOPBACK;
	uint8_t* msg = (uint8_t*)(accessMsg->p_data);
	uint16_t size = accessMsg->length;

	if (!ownMsg && accessMsg->opcode.opcode == CS_MESH_MODEL_OPCODE_RELIABLE_MSG) {
		LOGi("sendReply");
		sendReply(accessMsg);
	}
	if (!MeshModelPacketHelper::isValidMeshMessage(msg, size)) {
		LOGw("Invalid mesh message");
		BLEutil::printArray(msg, size);
		return;
	}
	cs_mesh_model_msg_type_t msgType = MeshModelPacketHelper::getType(msg);
	uint8_t* payload;
	size16_t payloadSize;
	MeshModelPacketHelper::getPayload(msg, size, payload, payloadSize);

	switch (msgType) {
#ifdef MESH_MODEL_TEST_MSG_DROP_ENABLED
	case CS_MESH_MODEL_TYPE_TEST: {
		if (ownMsg) {
			break;
		}
		cs_mesh_model_msg_test_t* test = (cs_mesh_model_msg_test_t*)payload;
		if (_lastReceivedCounter == 0) {
			_lastReceivedCounter = test->counter;
			break;
		}
		if (_lastReceivedCounter == test->counter) {
			// Ignore repeats
			break;
		}
		uint32_t expectedCounter = _lastReceivedCounter + 1;
		_lastReceivedCounter = test->counter;
		LOGMeshModelDebug("receivedCounter=%u expectedCounter=%u", _lastReceivedCounter, expectedCounter);
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
		LOGi("received=%u dropped=%u (%u%%)", _received, _dropped, (_dropped * 100) / (_received + _dropped));
		break;
	}
#endif
	case CS_MESH_MODEL_TYPE_ACK: {
		break;
	}
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
		cs_mesh_model_msg_multi_switch_item_t* item;
		if (MeshModelPacketHelper::multiSwitchHasItem(packet, myId, item)) {
			event_t event(CS_TYPE::CMD_MULTI_SWITCH, item, sizeof(*item));
			EventDispatcher::getInstance().dispatch(event);
		}
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE_STATE: {
		cs_mesh_model_msg_keep_alive_t* packet = (cs_mesh_model_msg_keep_alive_t*)payload;
		TYPIFY(CONFIG_CROWNSTONE_ID) myId;
		State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &myId, sizeof(myId));
		cs_mesh_model_msg_keep_alive_item_t* item;
		if (MeshModelPacketHelper::keepAliveHasItem(packet, myId, item)) {
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
			event_t event(CS_TYPE::EVT_KEEP_ALIVE_STATE, &keepAlive, sizeof(keepAlive));
			EventDispatcher::getInstance().dispatch(event);
		}
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE: {
		event_t event(CS_TYPE::EVT_KEEP_ALIVE);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	}
}

#ifdef MESH_MODEL_TEST_MSG_DROP_ENABLED
void MeshModel::sendTestMsg() {
	size16_t msgSize = MESH_HEADER_SIZE + sizeof(cs_mesh_model_msg_test_t);
	uint8_t msg[msgSize];
	cs_mesh_model_msg_test_t test;
	test.counter = _nextSendCounter;
	MeshModelPacketHelper::setMeshMessage(CS_MESH_MODEL_TYPE_TEST, (uint8_t*)&test, sizeof(test), msg, msgSize);
//	uint32_t retVal = sendMsg(msg, msgSize, 3);
	uint32_t retVal = sendReliableMsg(msg, msgSize);
	if (retVal != NRF_SUCCESS) {
		LOGw("sendTestMsg retVal=%u", retVal);
	}
	else {
		LOGMeshModelDebug("sendTestMsg retVal=%u", retVal);
	}
	if (retVal == NRF_SUCCESS) {
		++_nextSendCounter;
	}
}
#endif

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
		LOGMeshModelDebug("%s type=invalid", prefix);
		break;
	case NRF_MESH_ADDRESS_TYPE_UNICAST:
		LOGMeshModelDebug("%s type=unicast id=%u", prefix, addr->value);
		break;
	case NRF_MESH_ADDRESS_TYPE_VIRTUAL:{
		//128-bit virtual label UUID,
		__attribute__((unused)) uint32_t* uuid1 = (uint32_t*)(addr->p_virtual_uuid);
		__attribute__((unused)) uint32_t* uuid2 = (uint32_t*)(addr->p_virtual_uuid + 4);
		__attribute__((unused)) uint32_t* uuid3 = (uint32_t*)(addr->p_virtual_uuid + 8);
		__attribute__((unused)) uint32_t* uuid4 = (uint32_t*)(addr->p_virtual_uuid + 12);
		LOGMeshModelDebug("%s type=virtual id=%u uuid=%x%x%x%x", prefix, addr->value, uuid1, uuid2, uuid3, uuid4);
		break;
	}
	case NRF_MESH_ADDRESS_TYPE_GROUP:
		LOGMeshModelDebug("%s type=group id=%u", prefix, addr->value);
		break;
	}
}
