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
#include <util/cs_BleError.h>

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
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_MSG, CROWNSTONE_COMPANY_ID), staticMsgHandler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_RELIABLE_MSG, CROWNSTONE_COMPANY_ID), staticMsgHandler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_REPLY, CROWNSTONE_COMPANY_ID), staticMsgHandler},
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
//	retCode = dsm_address_publish_add(0xC51E, &_publishAddressHandle);
//	APP_ERROR_CHECK(retCode);
	// No need to call dsm_address_subscription_add_handle(), as we're only subscribed to unicast address.

	retCode = access_model_application_bind(_accessModelHandle, appkeyHandle);
	APP_ERROR_CHECK(retCode);
	retCode = access_model_publish_application_set(_accessModelHandle, appkeyHandle);
	APP_ERROR_CHECK(retCode);

//	retCode = access_model_publish_address_set(_accessModelHandle, _publishAddressHandle);
//	APP_ERROR_CHECK(retCode);
	// No need to call access_model_subscription_add(), as we're only subscribed to unicast address.
}

cs_ret_code_t MeshModelUnicast::setPublishAddress(stone_id_t id) {
	uint32_t retCode;

	// First clean up the previous one.
	dsm_address_publish_remove(_publishAddressHandle);

	// All addresses with first 2 bits 0, are unicast addresses.
	uint16_t address = id;
	retCode = dsm_address_publish_add(address, &_publishAddressHandle);
	assert(retCode == NRF_SUCCESS, "failed to add publish address");
	if (retCode != NRF_SUCCESS) {
		return ERR_UNSPECIFIED;
	}
	retCode = access_model_publish_address_set(_accessModelHandle, _publishAddressHandle);
	assert(retCode == NRF_SUCCESS, "failed to set publish address");
	if (retCode != NRF_SUCCESS) {
		return ERR_UNSPECIFIED;
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
//	bool ownMsg = (_ownAddress == accessMsg->meta_data.src.value) && (accessMsg->meta_data.src.type == NRF_MESH_ADDRESS_TYPE_UNICAST);
	bool ownMsg = accessMsg->meta_data.p_core_metadata->source == NRF_MESH_RX_SOURCE_LOOPBACK;
	if (ownMsg) {
		return;
	}
	MeshUtil::cs_mesh_received_msg_t msg;
	msg.opCode = accessMsg->opcode.opcode;
	msg.srcAddress = accessMsg->meta_data.src.value;
	msg.msg = (uint8_t*)(accessMsg->p_data);
	msg.msgSize = accessMsg->length;
	msg.rssi = MeshUtil::getRssi(accessMsg->meta_data.p_core_metadata);
	msg.hops = ACCESS_DEFAULT_TTL - accessMsg->meta_data.ttl;
	_msgCallback(msg);
}

cs_ret_code_t MeshModelUnicast::sendMsg(const uint8_t* msg, uint16_t msgSize) {
	if (!access_reliable_model_is_free(_accessModelHandle)) {
		LOGMeshModelVerbose("Busy");
		return ERR_BUSY;
	}
	access_message_tx_t* accessMsg = &(_accessReliableMsg.message);
	accessMsg->opcode.company_id = CROWNSTONE_COMPANY_ID;
	accessMsg->opcode.opcode = CS_MESH_MODEL_OPCODE_RELIABLE_MSG;
	accessMsg->p_buffer = msg;
	accessMsg->length = msgSize;
	accessMsg->force_segmented = false;
	accessMsg->transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	accessMsg->access_token = nrf_mesh_unique_token_get();

	_accessReliableMsg.model_handle = _accessModelHandle;
	_accessReliableMsg.reply_opcode.company_id = CROWNSTONE_COMPANY_ID;
	_accessReliableMsg.reply_opcode.opcode = CS_MESH_MODEL_OPCODE_REPLY;
	_accessReliableMsg.status_cb = staticReliableStatusHandler;
	_accessReliableMsg.timeout = _ackTimeoutUs;

	uint32_t retVal = access_model_reliable_publish(&_accessReliableMsg);
	LOGd("reliable send ret=%u", retVal);
	return retVal;
}

void MeshModelUnicast::handleReliableStatus(access_reliable_status_t status) {
	switch (status) {
		case ACCESS_RELIABLE_TRANSFER_SUCCESS:
			LOGi("reliable msg success");
#if MESH_MODEL_TEST_MSG == 2
			_acked++;
			LOGi("acked=%u timedout=%u canceled=%u (acked=%u%%)", _acked, _timedout, _canceled, (_acked * 100) / (_acked + _timedout + _canceled));
#endif
			break;
	case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
			LOGw("reliable msg timeout");
#if MESH_MODEL_TEST_MSG == 2
			_timedout++;
			LOGi("acked=%u timedout=%u canceled=%u (acked=%u%%)", _acked, _timedout, _canceled, (_acked * 100) / (_acked + _timedout + _canceled));
#endif
			break;
	case ACCESS_RELIABLE_TRANSFER_CANCELLED:
			LOGw("reliable msg cancelled");
#if MESH_MODEL_TEST_MSG == 2
			_canceled++;
			LOGi("acked=%u timedout=%u canceled=%u (acked=%u%%)", _acked, _timedout, _canceled, (_acked * 100) / (_acked + _timedout + _canceled));
#endif
			break;
	}
	if (_queueIndexInProgress == -1) {
		LOGe("No index in progress");
		return;
	}
	remQueueItem(_queueIndexInProgress);
}

cs_ret_code_t MeshModelUnicast::addToQueue(MeshUtil::cs_mesh_queue_item_t& item) {
	MeshUtil::printQueueItem("Unicast addToQueue", item.metaData);
#if MESH_MODEL_TEST_MSG != 0
	if (item.metaData.type != CS_MESH_MODEL_TYPE_TEST) {
		return ERR_SUCCESS;
	}
#endif
	size16_t msgSize = MeshUtil::getMeshMessageSize(item.payloadSize);
	assert(item.payloadPtr != nullptr || item.payloadSize == 0, "Null pointer");
	assert(msgSize != 0, "Empty message");
	assert(msgSize <= MAX_MESH_MSG_SIZE, "Message too large");
	assert(item.metaData.targetId != 0, "Unicast only");
	uint8_t index;
	for (int i = _queueIndexNext + _queueSize; i > _queueIndexNext; --i) {
		index = i % _queueSize;
		cs_unicast_queue_item_t* it = &(_queue[index]);
		if (it->metaData.repeats == 0) {
			it->msgPtr = (uint8_t*)malloc(msgSize);
			LOGMeshModelVerbose("alloc %p size=%u", it->msgPtr, msgSize);
			if (it->msgPtr == NULL) {
				return ERR_NO_SPACE;
			}
			if (!MeshUtil::setMeshMessage((cs_mesh_model_msg_type_t)item.metaData.type, item.payloadPtr, item.payloadSize, it->msgPtr, msgSize)) {
				LOGMeshModelVerbose("free %p", it->msgPtr);
				free(it->msgPtr);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
//			memcpy(it->msgPtr, item.msgPtr, item.metaData.msgSize);
			memcpy(&(it->metaData), &(item.metaData), sizeof(item.metaData));
			it->msgSize = msgSize;
			LOGMeshModelVerbose("added to ind=%u", index);
			_queueIndexNext = index;

			// TODO: immediately start sending from queue.
			// sendMsgFromQueue can keep up how many msgs have been sent this tick, so it knows how many can still be sent.
			return ERR_SUCCESS;
		}
	}
	LOGw("queue is full");
	return ERR_BUSY;
}

cs_ret_code_t MeshModelUnicast::remFromQueue(cs_mesh_model_msg_type_t type, uint16_t id) {
	cs_ret_code_t retCode = ERR_NOT_FOUND;
	for (int i = 0; i < _queueSize; ++i) {
		if (_queue[i].metaData.id == id && _queue[i].metaData.type == type && _queue[i].metaData.repeats != 0) {
			remQueueItem(i);
			retCode = ERR_SUCCESS;
		}
	}
	return retCode;
}

void MeshModelUnicast::remQueueItem(uint8_t index) {
	_queue[index].metaData.repeats = 0;
	LOGMeshModelVerbose("free %p", _queue[index].msgPtr);
	free(_queue[index].msgPtr);
	LOGMeshModelVerbose("removed from queue: ind=%u", index);
}

int MeshModelUnicast::getNextItemInQueue(bool priority) {
	int index;
	for (int i = _queueIndexNext; i < _queueIndexNext + _queueSize; i++) {
		index = i % _queueSize;
		if ((!priority || _queue[index].metaData.priority) && _queue[index].metaData.repeats > 0) {
			return index;
		}
	}
	return -1;
}

bool MeshModelUnicast::sendMsgFromQueue() {
	if (_queueIndexInProgress == -1) {
		return false;
	}
	int index = getNextItemInQueue(true);
	if (index == -1) {
		index = getNextItemInQueue(false);
	}
	if (index == -1) {
		return false;
	}
	cs_unicast_queue_item_t* item = &(_queue[index]);
	setPublishAddress(item->metaData.targetId);
	cs_ret_code_t retCode = sendMsg(item->msgPtr, item->msgSize);
	if (retCode != ERR_SUCCESS) {
		return false;
	}
	_queueIndexInProgress = index;
	LOGMeshModelInfo("send ind=%u repeats_left=%u type=%u id=%u", index, item->metaData.repeats, item->metaData.type, item->metaData.id);

	// Next item will be sent next, so that items are sent interleaved.
	_queueIndexNext = (index + 1) % _queueSize;
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
