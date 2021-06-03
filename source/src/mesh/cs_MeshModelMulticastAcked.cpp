/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 1, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <mesh/cs_MeshCommon.h>
#include <mesh/cs_MeshModelMulticastAcked.h>
#include <mesh/cs_MeshUtil.h>
#include <uart/cs_UartHandler.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>
#include <storage/cs_State.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

extern "C" {
#include <access.h>
#include <access_config.h>
//#include <nrf_mesh.h>
#include <log.h>
}

static void staticMshHandler(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args) {
	MeshModelMulticastAcked* meshModel = (MeshModelMulticastAcked*)p_args;
	meshModel->handleMsg(p_message);
}

static const access_opcode_handler_t opcodeHandlers[] = {
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_MULTICAST_RELIABLE_MSG, CROWNSTONE_COMPANY_ID), staticMshHandler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_MULTICAST_REPLY, CROWNSTONE_COMPANY_ID), staticMshHandler},
};

void MeshModelMulticastAcked::registerMsgHandler(const callback_msg_t& closure) {
	_msgCallback = closure;
}

void MeshModelMulticastAcked::init(uint16_t modelId) {
	assert(_msgCallback != nullptr, "Callback not set");
	uint32_t retVal;
	access_model_add_params_t accessParams;
	accessParams.model_id.company_id = CROWNSTONE_COMPANY_ID;
	accessParams.model_id.model_id = modelId;
	accessParams.element_index = 0;
	accessParams.p_opcode_handlers = opcodeHandlers;
	accessParams.opcode_count = (sizeof(opcodeHandlers) / sizeof((opcodeHandlers)[0]));
	accessParams.p_args = this;
	accessParams.publish_timeout_cb = NULL;
	retVal = access_model_add(&accessParams, &_accessModelHandle);
	APP_ERROR_CHECK(retVal);
	retVal = access_model_subscription_list_alloc(_accessModelHandle);
	APP_ERROR_CHECK(retVal);

	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_ownStoneId, sizeof(_ownStoneId));
}

void MeshModelMulticastAcked::configureSelf(dsm_handle_t appkeyHandle) {
	uint32_t retCode;
	retCode = dsm_address_publish_add(MESH_MODEL_GROUP_ADDRESS_ACKED, &_groupAddressHandle);
	APP_ERROR_CHECK(retCode);
	retCode = dsm_address_subscription_add_handle(_groupAddressHandle);
	APP_ERROR_CHECK(retCode);

	retCode = access_model_application_bind(_accessModelHandle, appkeyHandle);
	APP_ERROR_CHECK(retCode);
	retCode = access_model_publish_application_set(_accessModelHandle, appkeyHandle);
	APP_ERROR_CHECK(retCode);

	retCode = access_model_publish_address_set(_accessModelHandle, _groupAddressHandle);
	APP_ERROR_CHECK(retCode);
	retCode = access_model_subscription_add(_accessModelHandle, _groupAddressHandle);
	APP_ERROR_CHECK(retCode);
}

void MeshModelMulticastAcked::handleMsg(const access_message_rx_t * accessMsg) {
	if (accessMsg->meta_data.p_core_metadata->source != NRF_MESH_RX_SOURCE_LOOPBACK) {
//	if (true) {
		LOGMeshModelVerbose("Handle mesh msg. opcode=%u appkey=%u subnet=%u ttl=%u rssi=%i loopback=%u",
				accessMsg->opcode.opcode,
				accessMsg->meta_data.appkey_handle,
				accessMsg->meta_data.subnet_handle,
				accessMsg->meta_data.ttl,
				MeshUtil::getRssi(accessMsg->meta_data.p_core_metadata),
				accessMsg->meta_data.p_core_metadata->source == NRF_MESH_RX_SOURCE_LOOPBACK
		);
		MeshUtil::printMeshAddress("  Src: ", &(accessMsg->meta_data.src));
		MeshUtil::printMeshAddress("  Dst: ", &(accessMsg->meta_data.dst));
//		LOGMeshModelVerbose("ownAddress=%u  Data:", _ownAddress);
#if CS_SERIAL_NRF_LOG_ENABLED == 1
		__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Handle mesh msg. opcode=%u appkey=%u subnet=%u ttl=%u rssi=%i\n",
				accessMsg->opcode.opcode,
				accessMsg->meta_data.appkey_handle,
				accessMsg->meta_data.subnet_handle,
				accessMsg->meta_data.ttl,
				MeshUtil::getRssi(accessMsg->meta_data.p_core_metadata)
		);
	}
	else {
		__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Handle mesh msg loopback\n");
#endif
	}
//	bool ownMsg = (_ownAddress == accessMsg->meta_data.src.value) && (accessMsg->meta_data.src.type == NRF_MESH_ADDRESS_TYPE_UNICAST);
	bool ownMsg = accessMsg->meta_data.p_core_metadata->source == NRF_MESH_RX_SOURCE_LOOPBACK;
	if (ownMsg) {
		return;
	}

	MeshUtil::cs_mesh_received_msg_t msg = MeshUtil::fromAccessMessageRX(*accessMsg);

	switch (msg.opCode) {
		case CS_MESH_MODEL_OPCODE_MULTICAST_REPLY: {
//			if (!ownMsg) {
				handleReply(msg);
//			}
			return;
		}
		case CS_MESH_MODEL_OPCODE_MULTICAST_RELIABLE_MSG: {
			// Prepare a reply message.
			uint8_t replyMsg[MAX_MESH_MSG_NON_SEGMENTED_SIZE];

			mesh_reply_t reply = {
					.type = CS_MESH_MODEL_TYPE_UNKNOWN,
					.buf = cs_data_t(replyMsg + MESH_HEADER_SIZE, sizeof(replyMsg) - MESH_HEADER_SIZE),
					.dataSize = 0
			};

			// Handle the message, get the reply msg.
			_msgCallback(msg, &reply);

			// Send the reply.
			if (reply.dataSize > sizeof(replyMsg) - MESH_HEADER_SIZE) {
				reply.dataSize = sizeof(replyMsg) - MESH_HEADER_SIZE;
			}
			replyMsg[0] = reply.type;
			sendReply(accessMsg, replyMsg, MESH_HEADER_SIZE + reply.dataSize);
			break;
		}
		default:
			return;
	}
}

cs_ret_code_t MeshModelMulticastAcked::sendMsg(const uint8_t* data, uint16_t len) {
	access_message_tx_t accessMsg;
	accessMsg.opcode.company_id = CROWNSTONE_COMPANY_ID;
	accessMsg.opcode.opcode = CS_MESH_MODEL_OPCODE_MULTICAST_RELIABLE_MSG;
	accessMsg.p_buffer = data;
	accessMsg.length = len;
	accessMsg.force_segmented = false;
	accessMsg.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	accessMsg.access_token = nrf_mesh_unique_token_get();

	uint32_t status = NRF_SUCCESS;
	status = access_model_publish(_accessModelHandle, &accessMsg);
	if (status != NRF_SUCCESS) {
		LOGw("sendMsg failed: %u", status);
	}
	return status;
}

void MeshModelMulticastAcked::sendReply(const access_message_rx_t* accessMsg, const uint8_t* data, uint16_t len) {
	access_message_tx_t accessReplyMsg;
	accessReplyMsg.opcode.company_id = CROWNSTONE_COMPANY_ID;
	accessReplyMsg.opcode.opcode = CS_MESH_MODEL_OPCODE_MULTICAST_REPLY;
	accessReplyMsg.p_buffer = data;
	accessReplyMsg.length = len;
	accessReplyMsg.force_segmented = false;
	accessReplyMsg.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;

	uint32_t status = NRF_SUCCESS;
	for (int i = 0; i < MESH_MODEL_ACK_TRANSMISSIONS; ++i) {
		accessReplyMsg.access_token = nrf_mesh_unique_token_get();
		status = access_model_reply(_accessModelHandle, accessMsg, &accessReplyMsg);
		if (status != NRF_SUCCESS) {
			LOGw("sendReply failed: %u", status);
			break;
		}
	}
	LOGMeshModelDebug("Sent reply to id=%u", accessMsg->meta_data.src.value);
}

void MeshModelMulticastAcked::handleReply(MeshUtil::cs_mesh_received_msg_t & msg) {
	if (_queueIndexInProgress == queue_index_none) {
		return;
	}

	stone_id_t srcId = msg.srcAddress;

	// Find stone ID in list of stone IDs.
	auto item = _queue[_queueIndexInProgress];
	uint16_t stoneIndex = 0xFFFF;
	for (uint8_t i = 0; i < item.numIds; ++i) {
		if (item.stoneIdsPtr[i] == srcId) {
			stoneIndex = i;
			break;
		}
	}
	if (stoneIndex == 0xFFFF) {
		LOGMeshModelInfo("Stone id %u not in list", srcId);
		return;
	}

	// Check if stone ID has already been marked as acked, and thus already been handled.
	if (_ackedStonesBitmask.isSet(stoneIndex)) {
		LOGMeshModelVerbose("Already received ack from id %u", srcId);
		return;
	}

	// Handle reply message.
	_msgCallback(msg, nullptr);

	// Mark id as acked.
	LOGMeshModelDebug("Set acked bit %u", stoneIndex);
	_ackedStonesBitmask.setBit(stoneIndex);
}

cs_ret_code_t MeshModelMulticastAcked::addToQueue(MeshUtil::cs_mesh_queue_item_t& item) {
	MeshUtil::printQueueItem("MulticastAcked addToQueue", item.metaData);
#if MESH_MODEL_TEST_MSG != 0
	if (item.metaData.type != CS_MESH_MODEL_TYPE_TEST) {
		return ERR_SUCCESS;
	}
#endif

	size16_t msgSize = MeshUtil::getMeshMessageSize(item.msgPayload.len);
	// Only unsegmented for now.
	if (msgSize == 0 || msgSize > MAX_MESH_MSG_NON_SEGMENTED_SIZE) {
		LOGw("Wrong payload length: %u", msgSize);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	// Checks that should've been performed already.
	assert(item.msgPayload.data != nullptr || item.msgPayload.len == 0, "Null pointer");
	assert(item.broadcast == true, "Multicast only");
	assert(item.reliable == true, "Reliable only");

	// Find an empty spot in the queue (transmissions == 0).
	// Start looking at _queueIndexNext, then reverse iterate over the queue.
	// Then set the new _queueIndexNext at the newly added item, so that it will be sent next.
	// We do the reverse iterate, so that the chance is higher that
	// the old _queueIndexNext will be sent quickly after this newly added item.
	uint8_t index;
	for (int i = _queueIndexNext; i < _queueIndexNext + queue_size; ++i) {
		index = i % queue_size;
		cs_multicast_acked_queue_item_t* it = &(_queue[index]);
		if (it->metaData.transmissionsOrTimeout == 0) {

			// Allocate and copy msg.
			it->msgPtr = (uint8_t*)malloc(msgSize);
			LOGMeshModelVerbose("msg alloc %p size=%u", it->msgPtr, msgSize);
			if (it->msgPtr == NULL) {
				return ERR_NO_SPACE;
			}
			if (!MeshUtil::setMeshMessage((cs_mesh_model_msg_type_t)item.metaData.type, item.msgPayload.data, item.msgPayload.len, it->msgPtr, msgSize)) {
				LOGMeshModelVerbose("msg free %p", it->msgPtr);
				free(it->msgPtr);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

			// Allocate and copy stone ids.
			it->stoneIdsPtr = (stone_id_t*)malloc(item.numIds * sizeof(stone_id_t));
			LOGMeshModelVerbose("ids alloc %p size=%u", it->stoneIdsPtr, item.numIds * sizeof(stone_id_t));
			if (it->stoneIdsPtr == NULL) {
				LOGMeshModelVerbose("msg free %p", it->msgPtr);
				free(it->msgPtr);
				return ERR_NO_SPACE;
			}
			memcpy(it->stoneIdsPtr, item.stoneIdsPtr, item.numIds * sizeof(stone_id_t));

			// Copy meta data.
			memcpy(&(it->metaData), &(item.metaData), sizeof(item.metaData));
			it->numIds = item.numIds;
			it->msgSize = msgSize;

			LOGMeshModelVerbose("added to ind=%u", index);
			_logArray(LogLevelMeshModelVerbose, true, it->msgPtr, it->msgSize);

			// If queue was empty, we can start sending this item.
			sendMsgFromQueue();
			return ERR_SUCCESS;
		}
	}
	LOGw("queue is full");
	return ERR_BUSY;
}

cs_ret_code_t MeshModelMulticastAcked::remFromQueue(cs_mesh_model_msg_type_t type, uint16_t id) {
	cs_ret_code_t retCode = ERR_NOT_FOUND;
	for (int i = 0; i < queue_size; ++i) {
		if (_queue[i].metaData.id == id && _queue[i].metaData.type == type && _queue[i].metaData.transmissionsOrTimeout != 0) {
			cancelQueueItem(i);
			remQueueItem(i);
			retCode = ERR_SUCCESS;
		}
	}
	return retCode;
}

void MeshModelMulticastAcked::cancelQueueItem(uint8_t index) {
	if (_queueIndexInProgress == index) {
		LOGe("TODO: Cancel progress");
		_queueIndexInProgress = queue_index_none;
	}
}

void MeshModelMulticastAcked::remQueueItem(uint8_t index) {
	_queue[index].metaData.transmissionsOrTimeout = 0;
	LOGMeshModelVerbose("msg free %p", _queue[index].msgPtr);
	free(_queue[index].msgPtr);
	LOGMeshModelVerbose("ids free %p", _queue[index].stoneIdsPtr);
	free(_queue[index].stoneIdsPtr);
	_ackedStonesBitmask.setNumBits(0);
	LOGMeshModelVerbose("removed from queue: ind=%u", index);
}

int MeshModelMulticastAcked::getNextItemInQueue(bool priority) {
	int index;
	for (int i = _queueIndexNext; i < _queueIndexNext + queue_size; i++) {
		index = i % queue_size;
		if ((!priority || _queue[index].metaData.priority) && _queue[index].metaData.transmissionsOrTimeout > 0) {
			return index;
		}
	}
	return -1;
}

bool MeshModelMulticastAcked::sendMsgFromQueue() {
	if (_queueIndexInProgress != queue_index_none) {
		return false;
	}
	int index = getNextItemInQueue(true);
	if (index == -1) {
		index = getNextItemInQueue(false);
	}
	if (index == -1) {
		return false;
	}

	cs_multicast_acked_queue_item_t* item = &(_queue[index]);
	if (!prepareForMsg(item)) {
		return false;
	}

	cs_ret_code_t retCode = sendMsg(item->msgPtr, item->msgSize);
	if (retCode != ERR_SUCCESS) {
		return false;
	}
	_queueIndexInProgress = index;
	LOGMeshModelInfo("sent ind=%u timeout=%u type=%u id=%u", index, item->metaData.transmissionsOrTimeout, item->metaData.type, item->metaData.id);

	// Next item will be sent next, so that items are sent interleaved.
	_queueIndexNext = (index + 1) % queue_size;
	return true;
}

bool MeshModelMulticastAcked::prepareForMsg(cs_multicast_acked_queue_item_t* item) {
	_processCallsLeft = item->metaData.transmissionsOrTimeout * 1000 / MESH_MODEL_ACKED_RETRY_INTERVAL_MS;
	if (!_ackedStonesBitmask.setNumBits(item->numIds)) {
		return false;
	}
//	_handledSelf = false;

	// Mark own stone ID as acked.
	for (uint8_t i=0; i < item->numIds; ++i) {
		if (item->stoneIdsPtr[i] == _ownStoneId) {
			_ackedStonesBitmask.setBit(i);
			break;
		}
	}
	return true;
}

void MeshModelMulticastAcked::checkDone() {
	if (_queueIndexInProgress == queue_index_none) {
		return;
	}
	auto item = _queue[_queueIndexInProgress];

	// Check acks.
	if (_ackedStonesBitmask.isAllBitsSet()) {
		LOGi("Received ack from all stones.");
		MeshUtil::printQueueItem(" ", item.metaData);

		// TODO: get cmd type from payload in case of CS_MESH_MODEL_TYPE_CTRL_CMD
		CommandHandlerTypes cmdType = MeshUtil::getCtrlCmdType((cs_mesh_model_msg_type_t)item.metaData.type);

		result_packet_header_t ackResult(cmdType, ERR_SUCCESS);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_ACK_ALL_RESULT, (uint8_t*)&ackResult, sizeof(ackResult));
		LOGMeshModelDebug("all success");

		remQueueItem(_queueIndexInProgress);
		_queueIndexInProgress = queue_index_none;
	}

	// Check for timeout.
	if (_processCallsLeft == 0) {
		LOGi("Timeout.");
		MeshUtil::printQueueItem(" ", item.metaData);

		// TODO: get cmd type from payload in case of CS_MESH_MODEL_TYPE_CTRL_CMD
		CommandHandlerTypes cmdType = MeshUtil::getCtrlCmdType((cs_mesh_model_msg_type_t)item.metaData.type);

		// Timeout all remaining stones.
		uart_msg_mesh_result_packet_header_t resultHeader;
		resultHeader.resultHeader.commandType = cmdType;
		resultHeader.resultHeader.returnCode = ERR_TIMEOUT;
		for (uint8_t i = 0; i < item.numIds; ++i) {
			if (!_ackedStonesBitmask.isSet(i)) {
				resultHeader.stoneId = item.stoneIdsPtr[i];
				UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_RESULT, (uint8_t*)&resultHeader, sizeof(resultHeader));
				LOGi("timeout id=%u", resultHeader.stoneId);
			}
		}

		result_packet_header_t ackResult(cmdType, ERR_TIMEOUT);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_ACK_ALL_RESULT, (uint8_t*)&ackResult, sizeof(ackResult));
		LOGMeshModelDebug("all timeout");

		remQueueItem(_queueIndexInProgress);
		_queueIndexInProgress = queue_index_none;
	}
	else {
		--_processCallsLeft;
	}
}

void MeshModelMulticastAcked::retryMsg() {
	if (_queueIndexInProgress == queue_index_none) {
		return;
	}
	auto item = _queue[_queueIndexInProgress];
	sendMsg(item.msgPtr, item.msgSize);
}

void MeshModelMulticastAcked::processQueue() {
	checkDone();
	retryMsg();
	sendMsgFromQueue();
}

void MeshModelMulticastAcked::tick(uint32_t tickCount) {
	// Process only at retry interval.
	if (tickCount % (MESH_MODEL_ACKED_RETRY_INTERVAL_MS / TICK_INTERVAL_MS) == 0) {
		processQueue();
	}
}
