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

#define MESH_MODEL_TEST_MSG_DROP_ENABLED

#define MESH_MODEL_QUEUE_SIZE 5

/**
 * Mesh message of max size.
 */
struct __attribute__((__packed__)) cs_mesh_model_queued_msg_t {
	bool reliable : 1;
	uint8_t repeats : 7;
	size16_t msgSize;
	uint8_t msg[MAX_MESH_MSG_SIZE];
};

class MeshModel {
public:
	MeshModel();
	void init();
	void setOwnAddress(uint16_t address);
	cs_ret_code_t sendMsg(const uint8_t* data, uint16_t len, uint8_t repeats=1, cs_mesh_model_opcode_t opcode=CS_MESH_MODEL_OPCODE_MSG);
	cs_ret_code_t sendReliableMsg(const uint8_t* data, uint16_t len);

	access_model_handle_t getAccessModelHandle();

	void tick(TYPIFY(EVT_TICK) tickCount);
	/** Internal usage */
	void handleMsg(const access_message_rx_t * accessMsg);
	/** Internal usage */
	void handleReliableStatus(access_reliable_status_t status);

#ifdef MESH_MODEL_TEST_MSG_DROP_ENABLED
	void sendTestMsg();
	uint32_t _nextSendCounter = 1;
	uint32_t _lastReceivedCounter = 0;
	uint32_t _received = 0;
	uint32_t _dropped = 0;
#endif

private:
	access_model_handle_t _accessHandle = ACCESS_HANDLE_INVALID;
	uint16_t _ownAddress = 0;
	access_reliable_t _accessReliableMsg;
	uint8_t _reliableMsg[MAX_MESH_MSG_SIZE]; // Need to keep msg in ram until reply is received.
	access_message_tx_t _accessReplyMsg;
	uint8_t _replyMsg[MESH_HEADER_SIZE + 0];

	cs_mesh_model_queued_msg_t _queue[MESH_MODEL_QUEUE_SIZE] = {0};
	uint8_t _queueSendIndex = 0;
	cs_ret_code_t addToQueue(const uint8_t* msg, size16_t msgSize, uint8_t repeats, bool reliable);
	void processQueue();

	cs_ret_code_t _sendMsg(const uint8_t* data, uint16_t len, uint8_t repeats=1, cs_mesh_model_opcode_t opcode=CS_MESH_MODEL_OPCODE_MSG);
	cs_ret_code_t _sendReliableMsg(const uint8_t* data, uint16_t len);
	cs_ret_code_t sendReply(const access_message_rx_t * accessMsg);
	int8_t getRssi(const nrf_mesh_rx_metadata_t* metaData);
	void printMeshAddress(const char* prefix, const nrf_mesh_address_t* addr);
};
