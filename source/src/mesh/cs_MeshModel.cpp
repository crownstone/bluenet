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
#include "time/cs_SystemTime.h"
#include "util/cs_Utils.h"
#include "util/cs_BleError.h"
#include <cstddef> // For NULL

extern "C" {
#include "access.h"
#include "access_config.h"
#include "access_reliable.h"
#include "nrf_mesh.h"
#include "log.h"
}

#if TICK_INTERVAL_MS > MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS
#error "TICK_INTERVAL_MS must not be larger than MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS"
#endif

#define LOGMeshModelInfo    LOGi
#define LOGMeshModelDebug   LOGd
#define LOGMeshModelVerbose LOGnone


static void cs_mesh_model_msg_handler(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args) {
	MeshModel* meshModel = (MeshModel*)p_args;
	meshModel->handleMsg(p_message);
}

static void cs_mesh_model_reply_status_handler(access_model_handle_t model_handle, void * p_args, access_reliable_status_t status) {
	MeshModel* meshModel = (MeshModel*)p_args;
	meshModel->handleReliableStatus(status);
}

static const access_opcode_handler_t cs_mesh_model_opcode_handlers[] = {
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_MSG, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_RELIABLE_MSG, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_REPLY, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler},
};

MeshModel::MeshModel() {}


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



void MeshModel::handleMsg(const access_message_rx_t * accessMsg) {
	if (accessMsg->meta_data.p_core_metadata->source != NRF_MESH_RX_SOURCE_LOOPBACK) {
		LOGMeshModelVerbose("Handle mesh msg. opcode=%u appkey=%u subnet=%u ttl=%u rssi=%i", accessMsg->opcode.opcode, accessMsg->meta_data.appkey_handle, accessMsg->meta_data.subnet_handle, accessMsg->meta_data.ttl, getRssi(accessMsg->meta_data.p_core_metadata));
		printMeshAddress("  Src: ", &(accessMsg->meta_data.src));
		printMeshAddress("  Dst: ", &(accessMsg->meta_data.dst));
		LOGMeshModelVerbose("ownAddress=%u  Data:", _ownAddress);
#if CS_SERIAL_NRF_LOG_ENABLED == 1
		__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Handle mesh msg. opcode=%u appkey=%u subnet=%u ttl=%u rssi=%i\n", accessMsg->opcode.opcode, accessMsg->meta_data.appkey_handle, accessMsg->meta_data.subnet_handle, accessMsg->meta_data.ttl, getRssi(accessMsg->meta_data.p_core_metadata));
	}
	else {
		__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Handle mesh msg loopback\n");
#endif
	}
//	bool ownMsg = (_ownAddress == accessMsg->meta_data.src.value) && (accessMsg->meta_data.src.type == NRF_MESH_ADDRESS_TYPE_UNICAST);
	bool ownMsg = accessMsg->meta_data.p_core_metadata->source == NRF_MESH_RX_SOURCE_LOOPBACK;
	uint8_t* msg = (uint8_t*)(accessMsg->p_data);
	uint16_t size = accessMsg->length;
	if (ownMsg) {
		return;
	}
}







/**
 * Send a message over the mesh via publish, without reply.
 *
 * Note, repeats should be larger than zero. If not, nothing will be send.
 *
 * TODO: wait for NRF_MESH_EVT_TX_COMPLETE before sending next msg (especially in case of segmented msg).
 */
cs_ret_code_t MeshModel::_sendMsg(const uint8_t* data, uint16_t len, uint8_t repeats) {
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
			LOGMeshModelInfo("sendMsg failed: %u", status);
			break;
		}
	}
	return status;
}

cs_ret_code_t MeshModel::sendReply(const access_message_rx_t * accessMsg) {
//	size16_t msgSize = MESH_HEADER_SIZE;
//	uint8_t msg[msgSize];
	size16_t msgSize = sizeof(_replyMsg);
	uint8_t* msg = _replyMsg;
	MeshModelPacketHelper::setMeshMessage(CS_MESH_MODEL_TYPE_ACK, NULL, 0, msg, msgSize);

//	access_message_tx_t replyMsg;
	_accessReplyMsg.opcode.company_id = CROWNSTONE_COMPANY_ID;
	_accessReplyMsg.opcode.opcode = CS_MESH_MODEL_OPCODE_REPLY;
	_accessReplyMsg.p_buffer = msg;
	_accessReplyMsg.length = msgSize;
	_accessReplyMsg.force_segmented = false;
	_accessReplyMsg.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	_accessReplyMsg.access_token = nrf_mesh_unique_token_get();

	uint32_t retVal = access_model_reply(_accessHandle, accessMsg, &_accessReplyMsg);
	LOGMeshModelVerbose("send reply %u", retVal);
	return retVal;
}

// If you send segmented packets to a unicast address, the receiver have to ACK all the packets and if they aren't received they have to be sent again.
// If you send the packets to a group address, there will be no ACK so the packets have to be sent multiple times to compensate.
// Number of retries are set in  TRANSPORT_SAR_TX_RETRIES_DEFAULT (4) and can be adjusted with NRF_MESH_OPT_TRS_SAR_TX_RETRIES in nrf_mesh_opt.h.
// You can also adjust the timing in nrf_mesh_opt.h.
cs_ret_code_t MeshModel::_sendReliableMsg(const uint8_t* data, uint16_t len) {
	if (!access_reliable_model_is_free(_accessHandle)) {
		LOGw("Busy");
		return ERR_BUSY;
	}
//	access_reliable_t accessReliableMsg;
	assert(len <= sizeof(_reliableMsg), "reliable msg too large");
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
	// Reliable message timeout in microseconds. Usually MODEL_ACKNOWLEDGED_TRANSACTION_TIMEOUT
	_accessReliableMsg.timeout = 10 * 1000 * 1000;
	uint32_t retVal = access_model_reliable_publish(&_accessReliableMsg);
	LOGd("reliable send ret=%u", retVal);
	return retVal;
}

void MeshModel::handleReliableStatus(access_reliable_status_t status) {
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
}



/**
 * Add a msg to an empty spot in the queue (repeats == 0).
 * Start looking at SendIndex, then reverse iterate over the queue.
 * Then set the new SendIndex at the newly added item, so that it will be send first.
 * We do the reverse iterate, so that the old SendIndex should be handled early (for a large enough queue).
 */
cs_ret_code_t MeshModel::addToQueue(cs_mesh_model_msg_type_t type, stone_id_t targetId, uint16_t id, const uint8_t* payload, uint8_t payloadSize, uint8_t repeats, bool priority) {
	LOGMeshModelDebug("addToQueue type=%u id=%u size=%u repeats=%u priority=%u", type, id, payloadSize, repeats, priority);
#if MESH_MODEL_TEST_MSG != 0
	if (type != CS_MESH_MODEL_TYPE_TEST) {
		return ERR_SUCCESS;
	}
#endif
	size16_t msgSize = MeshModelPacketHelper::getMeshMessageSize(payloadSize);
//	assert(payloadSize <= (MAX_MESH_MSG_NON_SEGMENTED_SIZE), "No segmented msgs for now");
//	assert(payloadSize <= sizeof(_queue[0].msg), "Payload too large");
	assert(msgSize <= MAX_MESH_MSG_SIZE, "Message too large");
	uint8_t index;
//	for (int i = _queueSendIndex; i < _queueSendIndex + MESH_MODEL_QUEUE_SIZE; ++i) {
	for (int i = _queueSendIndex + MESH_MODEL_QUEUE_SIZE; i > _queueSendIndex; --i) {
		index = i % MESH_MODEL_QUEUE_SIZE;
		cs_mesh_model_queued_item_t* item = &(_queue[index]);
		if (item->repeats == 0) {
			item->msgPtr = (uint8_t*)malloc(msgSize);
			LOGMeshModelVerbose("alloc %p size=%u", item->msgPtr, msgSize);
			if (item->msgPtr == NULL) {
				return ERR_NO_SPACE;
			}
			if (!MeshModelPacketHelper::setMeshMessage(type, payload, payloadSize, item->msgPtr, msgSize)) {
				LOGMeshModelVerbose("free %p", item->msgPtr);
				free(item->msgPtr);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

			item->priority = priority;
			item->reliable = (msgSize > MAX_MESH_MSG_NON_SEGMENTED_SIZE); // Targeted segmented msgs are acked anyway, but now we actually get the result.
//			item->reliable = false;
			// return before setting repeats
			item->repeats = repeats;
			item->targetId = targetId;
			item->type = type;
			item->id = id;
			item->msgSize = msgSize;
			LOGMeshModelVerbose("added to ind=%u", index);
			_queueSendIndex = index;

			// TODO: start sending from queue.
			// sendMsgFromQueue can keep up how many msgs have been sent this tick, so it knows how many can still be sent.
			return ERR_SUCCESS;
		}
	}
	LOGw("queue is full");
	return ERR_BUSY;
}

cs_ret_code_t MeshModel::remFromQueue(cs_mesh_model_msg_type_t type, uint16_t id) {
	for (int i = 0; i < MESH_MODEL_QUEUE_SIZE; ++i) {
		if (_queue[i].id == id && _queue[i].type == type && _queue[i].repeats != 0) {
			_queue[i].repeats = 0;
			LOGMeshModelVerbose("free %p", _queue[i].msgPtr);
			free(_queue[i].msgPtr);
			LOGMeshModelDebug("removed from queue: ind=%u", i);
			return ERR_SUCCESS;
		}
	}
	return ERR_NOT_FOUND;
}

/**
 * Check if there is a msg in queue with more than 0 repeats.
 * If so, return that index.
 * Start looking at index SendIndex as that item should be sent first.
 */
int MeshModel::getNextItemInQueue(bool priority) {
	int index;
	for (int i = _queueSendIndex; i < _queueSendIndex + MESH_MODEL_QUEUE_SIZE; i++) {
		index = i % MESH_MODEL_QUEUE_SIZE;
		if ((!priority || _queue[index].priority) && _queue[index].repeats > 0) {
			return index;
		}
	}
	return -1;
}

/** Generate and send a message.
 * TODO: item->id can be used for unicast address.
 * TODO: use opcode for type.
 */
bool MeshModel::sendMsgFromQueue() {
	int index = getNextItemInQueue(true);
	if (index == -1) {
		index = getNextItemInQueue(false);
	}
	if (index == -1) {
		return false;
	}
	cs_mesh_model_queued_item_t* item = &(_queue[index]);
	if (item->type == CS_MESH_MODEL_TYPE_CMD_TIME) {
		Time time = SystemTime::posix();
		if (time.isValid()) {
			// Update time in set time command.
			uint8_t* payload = NULL;
			size16_t payloadSize = 0;
			MeshModelPacketHelper::getPayload(item->msgPtr, item->msgSize, payload, payloadSize);
			if (payloadSize == sizeof(cs_mesh_model_msg_time_t)) {
				cs_mesh_model_msg_time_t* timePayload = (cs_mesh_model_msg_time_t*) payload;
				timePayload->timestamp = time.timestamp();
			}
		}
	}
	if (item->reliable) {
		_sendReliableMsg(item->msgPtr, item->msgSize);
	}
	else {
		_sendMsg(item->msgPtr, item->msgSize, 1);
		// TOOD: check return code, maybe retry again later.
	}
	if (--(item->repeats) == 0) {
		LOGMeshModelVerbose("free %p", item->msgPtr);
		free(item->msgPtr);
	}
	LOGMeshModelInfo("sent ind=%u repeats_left=%u type=%u id=%u", index, item->repeats, item->type, item->id);
//	BLEutil::printArray(meshMsg.msg, meshMsg.size);

	// Next item will be sent next, so that items are sent interleaved.
	_queueSendIndex = (index + 1) % MESH_MODEL_QUEUE_SIZE;
	return true;
}


void MeshModel::processQueue() {
#if MESH_MODEL_TEST_MSG == 1
	if (_ownAddress == 2) {
		sendTestMsg();
	}
#endif
	for (int i=0; i<MESH_MODEL_QUEUE_BURST_COUNT; ++i) {
		if (!sendMsgFromQueue()) {
			break;
		}
	}
}


void MeshModel::tick(TYPIFY(EVT_TICK) tickCount) {
	if (tickCount % (MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS / TICK_INTERVAL_MS) == 0) {
		processQueue();
	}
#if MESH_MODEL_TEST_MSG == 2
	if (_ownAddress == 2 && tickCount % (1000 / TICK_INTERVAL_MS) == 0) {
		sendTestMsg();
	}
#endif
}

int8_t MeshModel::getRssi(const nrf_mesh_rx_metadata_t* metaData) {
	switch (metaData->source) {
	case NRF_MESH_RX_SOURCE_SCANNER:
		return metaData->params.scanner.rssi;
	case NRF_MESH_RX_SOURCE_GATT:
		// TODO: return connection rssi?
		return -10;
//	case NRF_MESH_RX_SOURCE_FRIEND:
//		// TODO: is this correct?
//		return metaData->params.scanner.rssi;
//		break;
//	case NRF_MESH_RX_SOURCE_LOW_POWER:
//		// TODO: is this correct?
//		return metaData->params.scanner.rssi;
//		break;
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
		LOGMeshModelVerbose("%s type=invalid", prefix);
		break;
	case NRF_MESH_ADDRESS_TYPE_UNICAST:
		LOGMeshModelVerbose("%s type=unicast id=%u", prefix, addr->value);
		break;
	case NRF_MESH_ADDRESS_TYPE_VIRTUAL:{
		//128-bit virtual label UUID,
		__attribute__((unused)) uint32_t* uuid1 = (uint32_t*)(addr->p_virtual_uuid);
		__attribute__((unused)) uint32_t* uuid2 = (uint32_t*)(addr->p_virtual_uuid + 4);
		__attribute__((unused)) uint32_t* uuid3 = (uint32_t*)(addr->p_virtual_uuid + 8);
		__attribute__((unused)) uint32_t* uuid4 = (uint32_t*)(addr->p_virtual_uuid + 12);
		LOGMeshModelVerbose("%s type=virtual id=%u uuid=%x%x%x%x", prefix, addr->value, uuid1, uuid2, uuid3, uuid4);
		break;
	}
	case NRF_MESH_ADDRESS_TYPE_GROUP:
		LOGMeshModelVerbose("%s type=group id=%u", prefix, addr->value);
		break;
	}
}
