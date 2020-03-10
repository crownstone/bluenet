/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "common/cs_Types.h"

extern "C" {
#include "access.h"
#include "access_reliable.h"
}






class MeshModel {
public:
	MeshModel();
	void init();
	void setOwnAddress(uint16_t address);

	access_model_handle_t getAccessModelHandle();

	void tick(TYPIFY(EVT_TICK) tickCount);
	/** Internal usage */
	void handleMsg(const access_message_rx_t * accessMsg);
	/** Internal usage */
	void handleReliableStatus(access_reliable_status_t status);

#if MESH_MODEL_TEST_MSG == 1
	uint32_t _nextSendCounter = 1;
	uint32_t _lastReceivedCounter = 0;
	uint32_t _received = 0;
	uint32_t _dropped = 0;
#elif MESH_MODEL_TEST_MSG == 2
	uint32_t _nextSendCounter = 1;
	uint32_t _acked = 0;
	uint32_t _timedout = 0;
	uint32_t _canceled = 0;
#endif

private:
	access_model_handle_t _accessHandle = ACCESS_HANDLE_INVALID;
	uint16_t _ownAddress = 0;
	access_reliable_t _accessReliableMsg;
	uint8_t _reliableMsg[MAX_MESH_MSG_SIZE]; // Need to keep msg in ram until reply is received.
	access_message_tx_t _accessReplyMsg;
	uint8_t _replyMsg[MESH_HEADER_SIZE + 0];

	cs_mesh_model_queued_item_t _queue[MESH_MODEL_QUEUE_SIZE] = {0};
	uint8_t _queueSendIndex = 0;
	cs_ret_code_t addToQueue(cs_mesh_model_msg_type_t type, stone_id_t targetId, uint16_t id, const uint8_t* payload, uint8_t payloadSize, uint8_t repeats, bool priority);
	cs_ret_code_t remFromQueue(cs_mesh_model_msg_type_t type, uint16_t id);
	int getNextItemInQueue(bool priority); // Returns -1 if none found.
	bool sendMsgFromQueue(); // Returns true when message was sent, false when no more messages to be sent.
	void processQueue();





	bool isFromSameState(uint16_t srcAddress, stone_id_t id, uint16_t partialTimestamp);
	void checkStateReceived(int8_t rssi, uint8_t ttl);

	cs_ret_code_t _sendMsg(const uint8_t* data, uint16_t len, uint8_t repeats=1);
	cs_ret_code_t _sendReliableMsg(const uint8_t* data, uint16_t len);
	cs_ret_code_t sendReply(const access_message_rx_t * accessMsg);
	int8_t getRssi(const nrf_mesh_rx_metadata_t* metaData);
	void printMeshAddress(const char* prefix, const nrf_mesh_address_t* addr);
};
