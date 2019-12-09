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

//#define MESH_MODEL_TEST_MSG_DROP_ENABLED

#define MESH_MODEL_QUEUE_SIZE 20

/**
 * Interval at which processQueue() gets called.
 */
#define MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS 100

/**
 * Number of messages sent each time processQueue() gets called.
 */
#define MESH_MODEL_QUEUE_BURST_COUNT 3

/**
 * Mesh message of max size.
 */
struct __attribute__((__packed__)) cs_mesh_model_queued_msg_t {
	bool reliable : 1;
	uint8_t repeats : 7;
	size16_t msgSize;
	uint8_t msg[MAX_MESH_MSG_SIZE];
};

struct __attribute__((__packed__)) cs_mesh_model_queued_item_t {
	bool priority : 1;
	uint8_t repeats : 7;
	stone_id_t id;   // 0 for broadcast
	uint8_t type;
	uint8_t payloadSize;
	uint8_t payload[MAX_MESH_MSG_NON_SEGMENTED_SIZE];
};

struct cs_mesh_model_ext_state_t {
	uint16_t address = 0;
	uint8_t partsReceived = 0;
	TYPIFY(EVT_STATE_EXTERNAL_STONE) state;
};

class MeshModel {
public:
	MeshModel();
	void init();
	void setOwnAddress(uint16_t address);
//	cs_ret_code_t sendMsg(uint8_t* data, uint16_t len, uint8_t repeats=5);
	cs_ret_code_t sendMsg(cs_mesh_msg_t *meshMsg);
	cs_ret_code_t sendReliableMsg(const uint8_t* data, uint16_t len);

	cs_ret_code_t sendMultiSwitchItem(const internal_multi_switch_item_t* item, uint8_t repeats=10);
	cs_ret_code_t sendKeepAliveItem(const keep_alive_state_item_t* item, uint8_t repeats=5);
	cs_ret_code_t sendTime(const cs_mesh_model_msg_time_t* item, uint8_t repeats=1);
	
	cs_ret_code_t sendProfileLocation(const cs_mesh_model_msg_profile_location_t* item, uint8_t repeats=1);

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

	TYPIFY(CONFIG_CROWNSTONE_ID) _ownId = 0;

	cs_mesh_model_msg_multi_switch_item_t _lastReceivedMultiSwitch = {0xFF};
	TYPIFY(EVT_KEEP_ALIVE_STATE) _lastReceivedKeepAlive = {0xFF};
	TYPIFY(CMD_SET_TIME) _lastReveivedSetTime = 0;
	cs_mesh_model_ext_state_t _lastReceivedState;

	cs_mesh_model_queued_item_t _queue[MESH_MODEL_QUEUE_SIZE] = {0};
	uint8_t _queueSendIndex = 0;
	cs_ret_code_t addToQueue(cs_mesh_model_msg_type_t type, stone_id_t id, const uint8_t* payload, uint8_t payloadSize, uint8_t repeats, bool priority);
	cs_ret_code_t remFromQueue(cs_mesh_model_msg_type_t type, stone_id_t id);
	int getNextItemInQueue(bool priority); // Returns -1 if none found.
	bool sendMsgFromQueue(); // Returns true when message was sent, false when no more messages to be sent.
	void processQueue();


	cs_ret_code_t _sendMsg(const uint8_t* data, uint16_t len, uint8_t repeats=1);
	cs_ret_code_t _sendReliableMsg(const uint8_t* data, uint16_t len);
	cs_ret_code_t sendReply(const access_message_rx_t * accessMsg);
	int8_t getRssi(const nrf_mesh_rx_metadata_t* metaData);
	void printMeshAddress(const char* prefix, const nrf_mesh_address_t* addr);
};
