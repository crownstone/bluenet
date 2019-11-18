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

#if TICK_INTERVAL_MS > MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS
#error "TICK_INTERVAL_MS must not be larger than MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS"
#endif

#define LOGMeshModelInfo    LOGnone
#define LOGMeshModelDebug   LOGnone
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

	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_ownId, sizeof(_ownId));
}

void MeshModel::setOwnAddress(uint16_t address) {
	_ownAddress = address;
}

access_model_handle_t MeshModel::getAccessModelHandle() {
	return _accessHandle;
}

//cs_ret_code_t MeshModel::sendMsg(uint8_t* data, uint16_t len, uint8_t repeats) {
cs_ret_code_t MeshModel::sendMsg(cs_mesh_msg_t *meshMsg) {
	if (!MeshModelPacketHelper::isValidMeshMessage(meshMsg)) {
		return ERR_INVALID_MESSAGE;
	}
	uint8_t repeats = meshMsg->reliability;
	bool priority = meshMsg->urgency == CS_MESH_URGENCY_HIGH;
//	cs_mesh_model_msg_type_t type = MeshModelPacketHelper::getType(meshMsg->msg);
//	uint8_t* payload = NULL;
//	size16_t payloadSize;
//	MeshModelPacketHelper::getPayload(meshMsg->msg, meshMsg->size, payload, payloadSize);
	remFromQueue(meshMsg->type, 0);
	return addToQueue(meshMsg->type, 0, meshMsg->payload, meshMsg->size, repeats, priority);
}



cs_ret_code_t MeshModel::sendMultiSwitchItem(const internal_multi_switch_item_t* item, uint8_t repeats) {
	cs_mesh_model_msg_multi_switch_item_t meshItem;
	meshItem.id = item->id;
	meshItem.switchCmd = item->cmd.switchCmd;
	meshItem.delay = item->cmd.delay;
	meshItem.source = item->cmd.source;
	remFromQueue(CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH, item->id);
	return addToQueue(CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH, item->id, (uint8_t*)(&meshItem), sizeof(meshItem), repeats, true);
}

cs_ret_code_t MeshModel::sendKeepAliveItem(const keep_alive_state_item_t* item, uint8_t repeats) {
	remFromQueue(CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE_STATE, item->id);
	return addToQueue(CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE_STATE, item->id, (uint8_t*)item, sizeof(*item), repeats, false);
}

cs_ret_code_t MeshModel::sendTime(const cs_mesh_model_msg_time_t* item, uint8_t repeats) {
	remFromQueue(CS_MESH_MODEL_TYPE_STATE_TIME, 0);
	return addToQueue(CS_MESH_MODEL_TYPE_STATE_TIME, 0, (uint8_t*)item, sizeof(*item), repeats, false);
}

cs_ret_code_t MeshModel::sendReliableMsg(const uint8_t* data, uint16_t len) {
	return ERR_NOT_IMPLEMENTED;
}

/**
 * Send a message over the mesh via publish, without reply.
 * TODO: wait for NRF_MESH_EVT_TX_COMPLETE before sending next msg (in case of segmented msg?).
 * TODO: repeat publishing the msg.
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
		LOGMeshModelVerbose("Handle mesh msg. opcode=%u appkey=%u subnet=%u ttl=%u rssi=%i", accessMsg->opcode.opcode, accessMsg->meta_data.appkey_handle, accessMsg->meta_data.subnet_handle, accessMsg->meta_data.ttl, getRssi(accessMsg->meta_data.p_core_metadata));
		printMeshAddress("  Src: ", &(accessMsg->meta_data.src));
		printMeshAddress("  Dst: ", &(accessMsg->meta_data.dst));
		LOGMeshModelVerbose("ownAddress=%u  Data:", _ownAddress);
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
	case CS_MESH_MODEL_TYPE_TEST: {
#ifdef MESH_MODEL_TEST_MSG_DROP_ENABLED
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
			LOGi("received same counter");
			break;
		}
		uint32_t expectedCounter = _lastReceivedCounter + 1;
		_lastReceivedCounter = test->counter;
		LOGMeshModelVerbose("receivedCounter=%u expectedCounter=%u", _lastReceivedCounter, expectedCounter);
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
#endif
		break;
	}
	case CS_MESH_MODEL_TYPE_ACK: {
		break;
	}
	case CS_MESH_MODEL_TYPE_STATE_TIME: {
		if (ownMsg) {
			break;
		}
		event_t event(CS_TYPE::EVT_MESH_TIME, payload, payloadSize);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_TIME: {
		if (*(TYPIFY(CMD_SET_TIME)*)payload != _lastReveivedSetTime) {
			_lastReveivedSetTime = *(TYPIFY(CMD_SET_TIME)*)payload;
			event_t event(CS_TYPE::CMD_SET_TIME, payload, payloadSize);
			EventDispatcher::getInstance().dispatch(event);
		}
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_NOOP: {
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH: {
		if (ownMsg) {
			break;
		}
		cs_mesh_model_msg_multi_switch_item_t* item = (cs_mesh_model_msg_multi_switch_item_t*) payload;
		if (item->id == _ownId) {
//			LOGi("recieved multi switch for me");
			if (memcmp(&_lastReceivedMultiSwitch, item, sizeof(*item)) == 0) {
//				LOGd("ignore multi switch");
				break;
			}
			memcpy(&_lastReceivedMultiSwitch, item, sizeof(*item));

			TYPIFY(CMD_MULTI_SWITCH) internalItem;
			internalItem.id = item->id;
			internalItem.cmd.switchCmd = item->switchCmd;
			internalItem.cmd.delay = item->delay;
			internalItem.cmd.source = item->source;
			internalItem.cmd.source.flagExternal = true;

			LOGi("dispatch multi switch");
			event_t event(CS_TYPE::CMD_MULTI_SWITCH, &internalItem, sizeof(internalItem));
			EventDispatcher::getInstance().dispatch(event);
		}
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE_STATE: {
		keep_alive_state_item_t* item = (keep_alive_state_item_t*) payload;
		if (item->id == _ownId) {
//			LOGi("recieved keep alive for me");
			TYPIFY(EVT_KEEP_ALIVE_STATE)* cmd = &(item->cmd);
			if (memcmp(&_lastReceivedKeepAlive, cmd, sizeof(*cmd)) != 0) {
				memcpy(&_lastReceivedKeepAlive, cmd, sizeof(*cmd));
				LOGi("dispatch keep alive");
				event_t event(CS_TYPE::EVT_KEEP_ALIVE_STATE, cmd, sizeof(*cmd));
				EventDispatcher::getInstance().dispatch(event);
			}
			else {
//				LOGd("ignore keep alive");
			}
		}
		break;
	}
	case CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE: {
		event_t event(CS_TYPE::EVT_KEEP_ALIVE);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case CS_MESH_MODEL_TYPE_STATE_0: {
		if (ownMsg) {
			break;
		}
		cs_mesh_model_msg_state_0_t* packet = (cs_mesh_model_msg_state_0_t*) payload;
		uint8_t srcId = accessMsg->meta_data.src.value;
		LOGMeshModelDebug("id=%u switch=%u flags=%u powerFactor=%i powerUsage=%i ts=%u", srcId, packet->switchState, packet->flags, packet->powerFactor, packet->powerUsageReal, packet->partialTimestamp);
		_lastReceivedState.address = srcId;
		_lastReceivedState.partsReceived = 1;
		_lastReceivedState.state.data.state.id = srcId;
		_lastReceivedState.state.data.extState.switchState = packet->switchState;
		_lastReceivedState.state.data.extState.flags = packet->flags;
		_lastReceivedState.state.data.extState.powerFactor = packet->powerFactor;
		_lastReceivedState.state.data.extState.powerUsageReal = packet->powerUsageReal;
		_lastReceivedState.state.data.extState.partialTimestamp = packet->partialTimestamp;
		break;
	}
	case CS_MESH_MODEL_TYPE_STATE_1: {
		if (ownMsg) {
			break;
		}
		cs_mesh_model_msg_state_1_t* packet = (cs_mesh_model_msg_state_1_t*) payload;
		uint8_t srcId = accessMsg->meta_data.src.value;
		LOGMeshModelDebug("id=%u temp=%i energy=%i ts=%u", srcId, packet->temperature, packet->energyUsed, packet->partialTimestamp);
		if (	_lastReceivedState.address == accessMsg->meta_data.src.value &&
				_lastReceivedState.partsReceived == 1 &&
				_lastReceivedState.state.data.extState.id == srcId &&
				_lastReceivedState.state.data.extState.partialTimestamp == packet->partialTimestamp
				) {
			_lastReceivedState.state.data.extState.temperature = packet->temperature;
			_lastReceivedState.state.data.extState.energyUsed = packet->energyUsed;
			// We don't actually know the RSSI to the source of this message.
			// We only get RSSI to a mac address, but we don't know what id a mac address has.
			// So for now, just assume we don't have direct contact with the source stone.
			_lastReceivedState.state.data.extState.rssi = 0;
			_lastReceivedState.state.rssi = 0;
			_lastReceivedState.state.data.extState.validation = SERVICE_DATA_VALIDATION;
			_lastReceivedState.state.data.type = SERVICE_DATA_TYPE_EXT_STATE;
			LOGMeshModelInfo("received: id=%u switch=%u flags=%u temp=%i pf=%i power=%i energy=%i ts=%u",
					_lastReceivedState.state.data.extState.id,
					_lastReceivedState.state.data.extState.switchState,
					_lastReceivedState.state.data.extState.flags,
					_lastReceivedState.state.data.extState.temperature,
					_lastReceivedState.state.data.extState.powerFactor,
					_lastReceivedState.state.data.extState.powerUsageReal,
					_lastReceivedState.state.data.extState.energyUsed,
					_lastReceivedState.state.data.extState.partialTimestamp
			);

			event_t event(CS_TYPE::EVT_STATE_EXTERNAL_STONE, &(_lastReceivedState.state), sizeof(_lastReceivedState.state));
			EventDispatcher::getInstance().dispatch(event);
		}
		break;
	}
	}
}

void MeshModel::handleReliableStatus(access_reliable_status_t status) {
	switch (status) {
	case ACCESS_RELIABLE_TRANSFER_SUCCESS:
//		LOGMeshModelVerbose("reliable msg success");
		LOGi("reliable msg success");
//		_received++;
//		LOGi("received=%u dropped=%u (%u%%)", _received, _dropped, (_dropped * 100) / (_received + _dropped));
		break;
	case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
		LOGw("reliable msg timeout");
//		_dropped++;
//		LOGi("received=%u dropped=%u (%u%%)", _received, _dropped, (_dropped * 100) / (_received + _dropped));
		break;
	case ACCESS_RELIABLE_TRANSFER_CANCELLED:
		LOGi("reliable msg cancelled");
		break;
	}
}


#ifdef MESH_MODEL_TEST_MSG_DROP_ENABLED
void MeshModel::sendTestMsg() {
//	if (_ownAddress != 17) {
//		return;
//	}
	size16_t msgSize = MESH_HEADER_SIZE + sizeof(cs_mesh_model_msg_test_t);
	uint8_t msg[msgSize];
	cs_mesh_model_msg_test_t test;
	test.counter = _nextSendCounter;
	MeshModelPacketHelper::setMeshMessage(CS_MESH_MODEL_TYPE_TEST, (uint8_t*)&test, sizeof(test), msg, msgSize);
	uint32_t retVal = sendMsg(msg, msgSize, 8);
//	uint32_t retVal = sendReliableMsg(msg, msgSize);
	if (retVal != NRF_SUCCESS) {
		LOGw("sendTestMsg retVal=%u", retVal);
	}
	else {
		LOGMeshModelVerbose("sendTestMsg retVal=%u", retVal);
	}
	if (retVal == NRF_SUCCESS) {
		++_nextSendCounter;
	}
}
#endif

/**
 * Add a msg to an empty spot in the queue (repeats == 0).
 * Start looking at SendIndex, then reverse iterate over the queue.
 * Then set the new SendIndex at the newly added item, so that it will be send first.
 * We do the reverse iterate, so that the old SendIndex should be handled early (for a large enough queue).
 */
cs_ret_code_t MeshModel::addToQueue(cs_mesh_model_msg_type_t type, stone_id_t id, const uint8_t* payload, uint8_t payloadSize, uint8_t repeats, bool priority) {
	uint8_t index;
//	for (int i = _queueSendIndex; i < _queueSendIndex + MESH_MODEL_QUEUE_SIZE; ++i) {
	for (int i = _queueSendIndex + MESH_MODEL_QUEUE_SIZE; i > _queueSendIndex; --i) {
		index = i % MESH_MODEL_QUEUE_SIZE;
		cs_mesh_model_queued_item_t* item = &(_queue[index]);
		if (item->repeats == 0) {
			item->priority = priority;
			item->repeats = repeats;
			item->id = id;
			item->type = type;
			item->payloadSize = payloadSize;
			memcpy(item->payload, payload, payloadSize);
			LOGMeshModelDebug("added to ind=%u type=%u id=%u", index, type, id);
//			BLEutil::printArray(payload, payloadSize);
			_queueSendIndex = index;
			return ERR_SUCCESS;
		}
	}
	LOGw("queue is full");
	return ERR_BUSY;
}

cs_ret_code_t MeshModel::remFromQueue(cs_mesh_model_msg_type_t type, stone_id_t id) {
	for (int i = 0; i < MESH_MODEL_QUEUE_SIZE; ++i) {
		if (_queue[i].id == id && _queue[i].type == type && _queue[i].repeats != 0) {
			_queue[i].repeats = 0;
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
 * TODO: for now it's just 1 item per message, later we could add multiple items to a message.
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
	size16_t msgSize = MeshModelPacketHelper::getMeshMessageSize(item->payloadSize);
	uint8_t* msg = (uint8_t*)malloc(msgSize);
	if (item->type == CS_MESH_MODEL_TYPE_CMD_TIME) {
		// Update time in set time command.
		cs_mesh_model_msg_time_t* timePayload = (cs_mesh_model_msg_time_t*) item->payload;
		State::getInstance().get(CS_TYPE::STATE_TIME, &(timePayload->timestamp), sizeof(timePayload->timestamp));
	}
	bool success = MeshModelPacketHelper::setMeshMessage((cs_mesh_model_msg_type_t)item->type, item->payload, item->payloadSize, msg, msgSize);
	if (success) {
		_sendMsg(msg, msgSize, 1);
	}
	free(msg);
	--(item->repeats);
	LOGMeshModelInfo("sent ind=%u repeats_left=%u type=%u id=%u", index, item->repeats, item->type, item->id);
//	BLEutil::printArray(meshMsg.msg, meshMsg.size);

	// Next item will be sent next, so that items are sent interleaved.
	_queueSendIndex = (index + 1) % MESH_MODEL_QUEUE_SIZE;
	return true;
}


void MeshModel::processQueue() {
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
