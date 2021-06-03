/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 11, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <mesh/cs_MeshCommon.h>
#include <mesh/cs_MeshModelUnicast.h>
#include <mesh/cs_MeshUtil.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>
#include <uart/cs_UartHandler.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

extern "C" {
//#include <access.h>
#include <access_config.h>
#include <access_reliable.h>
//#include <nrf_mesh.h>
#include <log.h>
}

static void staticMsgHandler(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args) {
	MeshModelUnicast* meshModel = (MeshModelUnicast*)p_args;
	meshModel->handleMsg(p_message);
}

static void staticReliableStatusHandler(access_model_handle_t model_handle, void * p_args, access_reliable_status_t status) {
	MeshModelUnicast* meshModel = (MeshModelUnicast*)p_args;
	meshModel->handleReliableStatus(status);
}

static const access_opcode_handler_t opcodeHandlers[] = {
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_UNICAST_RELIABLE_MSG, CROWNSTONE_COMPANY_ID), staticMsgHandler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_UNICAST_REPLY, CROWNSTONE_COMPANY_ID), staticMsgHandler},
};

void MeshModelUnicast::registerMsgHandler(const callback_msg_t& closure) {
	_msgCallback = closure;
}

void MeshModelUnicast::init(uint16_t modelId) {
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
}

void MeshModelUnicast::configureSelf(dsm_handle_t appkeyHandle) {
	uint32_t retCode;
	// No need to call dsm_address_subscription_add_handle(), as we're only subscribed to unicast address.

	retCode = access_model_application_bind(_accessModelHandle, appkeyHandle);
	APP_ERROR_CHECK(retCode);
	retCode = access_model_publish_application_set(_accessModelHandle, appkeyHandle);
	APP_ERROR_CHECK(retCode);

	// No need to call access_model_subscription_add(), as we're only subscribed to unicast address.
}

cs_ret_code_t MeshModelUnicast::setPublishAddress(stone_id_t id) {
	LOGMeshModelVerbose("setPublishAddress %u", id);
	// First clean up the previous one.
	uint32_t nrfCode = dsm_address_publish_remove(_publishAddressHandle);
	switch (nrfCode) {
		case NRF_SUCCESS:
		case NRF_ERROR_NOT_FOUND: {
			break;
		}
		default: {
			LOGw("Failed to remove publish address: nrfCode=%u", nrfCode);
			return ERR_UNSPECIFIED;
		}
	}

	// All addresses with first 2 bits 0, are unicast addresses.
	uint16_t address = id;
	nrfCode = dsm_address_publish_add(address, &_publishAddressHandle);
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Failed to add publish address: nrfCode=%u", nrfCode);
		return ERR_UNSPECIFIED;
	}
	nrfCode = access_model_publish_address_set(_accessModelHandle, _publishAddressHandle);
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Failed to set publish address: nrfCode=%u", nrfCode);
		return ERR_UNSPECIFIED;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MeshModelUnicast::setTtl(uint8_t ttl, bool temp) {
	LOGMeshModelVerbose("setTtl %u", ttl);
	uint32_t nrfCode = access_model_publish_ttl_set(_accessModelHandle, ttl);
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Failed to set TTL: nrfCode=%u", nrfCode);
		return ERR_UNSPECIFIED;
	}
	if (!temp) {
		_ttl = ttl;
	}
	return ERR_SUCCESS;
}

void MeshModelUnicast::handleMsg(const access_message_rx_t * accessMsg) {
	if (accessMsg->meta_data.p_core_metadata->source != NRF_MESH_RX_SOURCE_LOOPBACK) {
		LOGMeshModelVerbose("Handle mesh msg. opcode=%u appkey=%u subnet=%u ttl=%u rssi=%i",
				accessMsg->opcode.opcode,
				accessMsg->meta_data.appkey_handle,
				accessMsg->meta_data.subnet_handle,
				accessMsg->meta_data.ttl,
				MeshUtil::getRssi(accessMsg->meta_data.p_core_metadata)
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

	bool ownMsg = accessMsg->meta_data.p_core_metadata->source == NRF_MESH_RX_SOURCE_LOOPBACK;
	if (ownMsg) {
		return;
	}
	MeshUtil::cs_mesh_received_msg_t msg = MeshUtil::fromAccessMessageRX(*accessMsg);

	if (msg.opCode == CS_MESH_MODEL_OPCODE_UNICAST_REPLY) {
		// Handle the message, don't send a reply.
		_replyReceived = true;
		_msgCallback(msg, nullptr);
		checkDone();
		return;
	}

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
		LOGw("Data size was set to %u", reply.dataSize);
		reply.dataSize = sizeof(replyMsg) - MESH_HEADER_SIZE;
	}
	replyMsg[0] = reply.type;
	sendReply(accessMsg, replyMsg, MESH_HEADER_SIZE + reply.dataSize);
}

cs_ret_code_t MeshModelUnicast::sendReply(const access_message_rx_t* accessMsg, const uint8_t* msg, uint16_t msgSize) {
	access_message_tx_t accessReplyMsg;
	accessReplyMsg.opcode.company_id = CROWNSTONE_COMPANY_ID;
	accessReplyMsg.opcode.opcode = CS_MESH_MODEL_OPCODE_UNICAST_REPLY;
	accessReplyMsg.p_buffer = msg;
	accessReplyMsg.length = msgSize;
	accessReplyMsg.force_segmented = false;
	accessReplyMsg.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	accessReplyMsg.access_token = nrf_mesh_unique_token_get();

	// Publish address is taken from the received accessMsg.
	// TTL is only taken from the received accessMsg if it's 0, else it uses the current model TTL.
	if (accessMsg->meta_data.ttl) {
		setTtl(CS_MESH_DEFAULT_TTL, true);
	}

	uint32_t nrfCode = access_model_reply(_accessModelHandle, accessMsg, &accessReplyMsg);
	LOGMeshModelVerbose("send reply nrfCode=%u", nrfCode);

	// Restore TTL
	if (accessMsg->meta_data.ttl) {
		setTtl(_ttl, true);
	}

	if (nrfCode != NRF_SUCCESS) {
		LOGw("Failed to send reply: nrfCode=%u", nrfCode);
		return ERR_UNSPECIFIED;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MeshModelUnicast::sendMsg(const uint8_t* msg, uint16_t msgSize, uint32_t timeoutUs) {
	if (!access_reliable_model_is_free(_accessModelHandle)) {
		LOGw("Busy");
		return ERR_BUSY;
	}
	access_message_tx_t* accessMsg = &(_accessReliableMsg.message);
	accessMsg->opcode.company_id = CROWNSTONE_COMPANY_ID;
	accessMsg->opcode.opcode = CS_MESH_MODEL_OPCODE_UNICAST_RELIABLE_MSG;
	accessMsg->p_buffer = msg;
	accessMsg->length = msgSize;
	accessMsg->force_segmented = false;
	accessMsg->transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	accessMsg->access_token = nrf_mesh_unique_token_get();

	_accessReliableMsg.model_handle = _accessModelHandle;
	_accessReliableMsg.reply_opcode.company_id = CROWNSTONE_COMPANY_ID;
	_accessReliableMsg.reply_opcode.opcode = CS_MESH_MODEL_OPCODE_UNICAST_REPLY;
	_accessReliableMsg.status_cb = staticReliableStatusHandler;
	_accessReliableMsg.timeout = timeoutUs;

	uint32_t nrfCode = access_model_reliable_publish(&_accessReliableMsg);
	LOGd("reliable send nrfCode=%u", nrfCode);
	if (nrfCode != NRF_SUCCESS) {
		LOGw("Failed to send msg: nrfCode=%u", nrfCode);
		return ERR_UNSPECIFIED;
	}
	return ERR_SUCCESS;
}

void MeshModelUnicast::handleReliableStatus(access_reliable_status_t status) {
	if (_queueIndexInProgress == queue_index_none) {
		LOGe("No index in progress");
		return;
	}

	switch (status) {
		case ACCESS_RELIABLE_TRANSFER_SUCCESS: {
			LOGi("reliable msg success");
			MeshUtil::printQueueItem("", _queue[_queueIndexInProgress].metaData);
#if MESH_MODEL_TEST_MSG == 2
			_acked++;
			LOGi("acked=%u timedout=%u canceled=%u (acked=%u%%)", _acked, _timedout, _canceled, (_acked * 100) / (_acked + _timedout + _canceled));
#endif
			break;
		}
		case ACCESS_RELIABLE_TRANSFER_TIMEOUT: {
			LOGw("reliable msg timeout");
			MeshUtil::printQueueItem("", _queue[_queueIndexInProgress].metaData);
#if MESH_MODEL_TEST_MSG == 2
			_timedout++;
			LOGi("acked=%u timedout=%u canceled=%u (acked=%u%%)", _acked, _timedout, _canceled, (_acked * 100) / (_acked + _timedout + _canceled));
#endif
			break;
		}
		case ACCESS_RELIABLE_TRANSFER_CANCELLED: {
			LOGw("reliable msg cancelled");
#if MESH_MODEL_TEST_MSG == 2
			_canceled++;
			LOGi("acked=%u timedout=%u canceled=%u (acked=%u%%)", _acked, _timedout, _canceled, (_acked * 100) / (_acked + _timedout + _canceled));
#endif
			break;
		}
	}
	_reliableStatus = status;
	checkDone();
}

void MeshModelUnicast::checkDone() {
	bool done = false;
	switch (_reliableStatus) {
		case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
			sendFailedResultToUart(
					_queue[_queueIndexInProgress].targetId,
					(cs_mesh_model_msg_type_t)_queue[_queueIndexInProgress].metaData.type,
					ERR_TIMEOUT
			);
			done = true;
			break;
		case ACCESS_RELIABLE_TRANSFER_CANCELLED: {
			sendFailedResultToUart(
					_queue[_queueIndexInProgress].targetId,
					(cs_mesh_model_msg_type_t)_queue[_queueIndexInProgress].metaData.type,
					ERR_CANCELED
			);
			done = true;
			break;
		}
		case ACCESS_RELIABLE_TRANSFER_SUCCESS:
			if (_replyReceived) {
				// TODO: get cmd type from payload in case of CS_MESH_MODEL_TYPE_CTRL_CMD
				CommandHandlerTypes cmdType = MeshUtil::getCtrlCmdType((cs_mesh_model_msg_type_t)_queue[_queueIndexInProgress].metaData.type);
				result_packet_header_t ackResult(cmdType, ERR_SUCCESS);
				UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_ACK_ALL_RESULT, (uint8_t*)&ackResult, sizeof(ackResult));
				LOGMeshModelDebug("all success");
				done = true;
			}
			break;
		default:
			break;
	}

	if (done) {
		LOGMeshModelDebug("rem item");
		remQueueItem(_queueIndexInProgress);
		_queueIndexInProgress = queue_index_none;
	}
}

void MeshModelUnicast::sendFailedResultToUart(stone_id_t id, cs_mesh_model_msg_type_t msgType, cs_ret_code_t retCode) {
	// TODO: get cmd type from payload in case of CS_MESH_MODEL_TYPE_CTRL_CMD
	CommandHandlerTypes cmdType = MeshUtil::getCtrlCmdType(msgType);

	uart_msg_mesh_result_packet_header_t resultHeader;
	resultHeader.resultHeader.commandType = cmdType;
	resultHeader.resultHeader.returnCode = retCode;
	resultHeader.stoneId = id;
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_RESULT, (uint8_t*)&resultHeader, sizeof(resultHeader));
	LOGMeshModelDebug("failed id=%u", id);

	result_packet_header_t ackResult(cmdType, ERR_TIMEOUT);
	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MESH_ACK_ALL_RESULT, (uint8_t*)&ackResult, sizeof(ackResult));
	LOGMeshModelDebug("all failed");
}

cs_ret_code_t MeshModelUnicast::addToQueue(MeshUtil::cs_mesh_queue_item_t& item) {
	MeshUtil::printQueueItem("Unicast addToQueue", item.metaData);
#if MESH_MODEL_TEST_MSG != 0
	if (item.metaData.type != CS_MESH_MODEL_TYPE_TEST) {
		return ERR_SUCCESS;
	}
#endif
	size16_t msgSize = MeshUtil::getMeshMessageSize(item.msgPayload.len);
	if (msgSize == 0 || msgSize > MAX_MESH_MSG_SIZE) {
		LOGw("Wrong payload length: %u", msgSize);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	// Checks that should've been performed already.
	assert(item.msgPayload.data != nullptr || item.msgPayload.len == 0, "Null pointer");
	assert(item.numIds == 1, "Single ID only");
	assert(item.broadcast == false, "Unicast only");
	assert(item.reliable == true, "Reliable only");

	// Find an empty spot in the queue (transmissions == 0).
	// Start looking at _queueIndexNext, then iterate over the queue.
	uint8_t index;
	for (int i = _queueIndexNext; i < _queueIndexNext + queue_size; ++i) {
		index = i % queue_size;
		cs_unicast_queue_item_t* it = &(_queue[index]);
		if (it->metaData.transmissionsOrTimeout == 0) {
			it->msgPtr = (uint8_t*)malloc(msgSize);
			LOGMeshModelVerbose("alloc %p size=%u", it->msgPtr, msgSize);
			if (it->msgPtr == NULL) {
				return ERR_NO_SPACE;
			}
			if (!MeshUtil::setMeshMessage((cs_mesh_model_msg_type_t)item.metaData.type, item.msgPayload.data, item.msgPayload.len, it->msgPtr, msgSize)) {
				LOGMeshModelVerbose("free %p", it->msgPtr);
				free(it->msgPtr);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			memcpy(&(it->metaData), &(item.metaData), sizeof(item.metaData));
			it->targetId = item.stoneIdsPtr[0];
			it->msgSize = msgSize;
			it->metaData.noHop = item.noHop;
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

cs_ret_code_t MeshModelUnicast::remFromQueue(cs_mesh_model_msg_type_t type, uint16_t id) {
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

void MeshModelUnicast::cancelQueueItem(uint8_t index) {
	if (_queueIndexInProgress == index) {
		LOGe("TODO: Cancel progress");
		_queueIndexInProgress = queue_index_none;
	}
}

void MeshModelUnicast::remQueueItem(uint8_t index) {
	_queue[index].metaData.transmissionsOrTimeout = 0;
	LOGMeshModelVerbose("free %p", _queue[index].msgPtr);
	free(_queue[index].msgPtr);
	LOGMeshModelVerbose("removed from queue: ind=%u", index);
}

int MeshModelUnicast::getNextItemInQueue(bool priority) {
	int index;
	for (int i = _queueIndexNext; i < _queueIndexNext + queue_size; ++i) {
		index = i % queue_size;
		if ((!priority || _queue[index].metaData.priority) && _queue[index].metaData.transmissionsOrTimeout > 0) {
			return index;
		}
	}
	return -1;
}

bool MeshModelUnicast::sendMsgFromQueue() {
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

	_replyReceived = false;
	_reliableStatus = 255;

	cs_unicast_queue_item_t* item = &(_queue[index]);
	cs_ret_code_t retCode = setPublishAddress(item->targetId);
	if (retCode != ERR_SUCCESS) {
		return false;
	}

	retCode = setTtl(item->metaData.noHop ? 0 : CS_MESH_DEFAULT_TTL);
	if (retCode != ERR_SUCCESS) {
		return false;
	}

	retCode = sendMsg(item->msgPtr, item->msgSize, item->metaData.transmissionsOrTimeout * 1000 * 1000);
	if (retCode != ERR_SUCCESS) {
		return false;
	}
	_queueIndexInProgress = index;
	LOGMeshModelInfo("sent ind=%u timeout=%u type=%u id=%u targetId=%u", index, item->metaData.transmissionsOrTimeout, item->metaData.type, item->metaData.id, item->targetId);

	// Next item will be sent next.
	// Order might be messed up when some items are prioritized.
	_queueIndexNext = (index + 1) % queue_size;
	return true;
}

void MeshModelUnicast::processQueue() {
	sendMsgFromQueue();
}

void MeshModelUnicast::tick(uint32_t tickCount) {
	if (tickCount % (MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS / TICK_INTERVAL_MS) == 0) {
		processQueue();
	}
}
