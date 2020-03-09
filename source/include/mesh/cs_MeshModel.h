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

/*
 * 0 to disable test.
 * 1 for unacked, unsegmented drop test.
 *   This assumes you have a node with id 2 (sending node).
 * 2 for targeted, segmented acked test.
 *   This assumes you have 2 nodes: one with id 1 (receiving node), and one with id 2 (sending node).
 */
#define MESH_MODEL_TEST_MSG 0

#define MESH_MODEL_QUEUE_SIZE 20

/**
 * Interval at which processQueue() gets called.
 */
#define MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS 100

/**
 * Number of messages sent each time processQueue() gets called.
 */
#define MESH_MODEL_QUEUE_BURST_COUNT 3


struct __attribute__((__packed__)) cs_mesh_model_queued_item_t {
	bool priority : 1;
	bool reliable : 1;
	uint8_t repeats : 6;
	stone_id_t targetId = 0;   // 0 for broadcast
	uint8_t type;
	uint16_t id; // ID that can freely be used to find similar items.
	uint8_t msgSize;
//	uint8_t msg[MAX_MESH_MSG_NON_SEGMENTED_SIZE];
	uint8_t* msgPtr = NULL;
};

struct cs_mesh_model_ext_state_t {
	uint16_t address = 0;
	uint8_t partsReceivedBitmask = 0;
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

	cs_ret_code_t sendMultiSwitchItem(const internal_multi_switch_item_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOW);
	cs_ret_code_t sendTime(const cs_mesh_model_msg_time_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOWEST);
	cs_ret_code_t sendBehaviourSettings(const behaviour_settings_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOWEST);
	cs_ret_code_t sendProfileLocation(const cs_mesh_model_msg_profile_location_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOWEST);
	cs_ret_code_t sendTrackedDeviceRegister(const cs_mesh_model_msg_device_register_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOW);
	cs_ret_code_t sendTrackedDeviceToken(const cs_mesh_model_msg_device_token_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOW);
	cs_ret_code_t sendTrackedDeviceListSize(const cs_mesh_model_msg_device_list_size_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOW);

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

	TYPIFY(CONFIG_CROWNSTONE_ID) _ownId = 0;

	cs_mesh_model_msg_multi_switch_item_t _lastReceivedMultiSwitch = {0xFF};
	TYPIFY(CMD_SET_TIME) _lastReveivedSetTime = 0;
	cs_mesh_model_ext_state_t _lastReceivedState;

	cs_mesh_model_queued_item_t _queue[MESH_MODEL_QUEUE_SIZE] = {0};
	uint8_t _queueSendIndex = 0;
	cs_ret_code_t addToQueue(cs_mesh_model_msg_type_t type, stone_id_t targetId, uint16_t id, const uint8_t* payload, uint8_t payloadSize, uint8_t repeats, bool priority);
	cs_ret_code_t remFromQueue(cs_mesh_model_msg_type_t type, uint16_t id);
	int getNextItemInQueue(bool priority); // Returns -1 if none found.
	bool sendMsgFromQueue(); // Returns true when message was sent, false when no more messages to be sent.
	void processQueue();

	cs_ret_code_t sendTestMsg();

	// handlers for incoming mesh messages
	void handleTest(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleAck(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleStateTime(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleCmdTime(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleCmdNoop(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleCmdMultiSwitch(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleState0(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleState1(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleProfileLocation(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleSetBehaviourSettings(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleTrackedDeviceRegister(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleTrackedDeviceToken(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleTrackedDeviceListSize(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);
	void handleSyncRequest(const access_message_rx_t * accessMsg, uint8_t* payload, size16_t payloadSize);

	bool isFromSameState(uint16_t srcAddress, stone_id_t id, uint16_t partialTimestamp);
	void checkStateReceived(int8_t rssi, uint8_t ttl);

	cs_ret_code_t _sendMsg(const uint8_t* data, uint16_t len, uint8_t repeats=1);
	cs_ret_code_t _sendReliableMsg(const uint8_t* data, uint16_t len);
	cs_ret_code_t sendReply(const access_message_rx_t * accessMsg);
	int8_t getRssi(const nrf_mesh_rx_metadata_t* metaData);
	void printMeshAddress(const char* prefix, const nrf_mesh_address_t* addr);
};
