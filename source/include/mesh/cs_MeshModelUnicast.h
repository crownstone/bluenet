/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <mesh/cs_MeshCommon.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <third/std/function.h>
#include <cfg/cs_Config.h>

extern "C" {
#include <access_reliable.h>
}

/**
 * Class that:
 * - Sends and receives targeted acked messages.
 * - Uses reliable segmented messages for this.
 * - Queues messages to be sent.
 * - Handles queue 1 by 1.
 */
class MeshModelUnicast {
public:
	/** Callback function definition. */
	typedef function<void(const MeshUtil::cs_mesh_received_msg_t& msg, mesh_reply_t* reply)> callback_msg_t;

	/**
	 * Register a callback function that's called when a message from the mesh is received.
	 */
	void registerMsgHandler(const callback_msg_t& closure);

	/**
	 * Init the model.
	 */
	void init(uint16_t modelId);

	/**
	 * Configure the model.
	 *
	 * Subscribes, and sets publish address.
	 */
	void configureSelf(dsm_handle_t appkeyHandle);

	/**
	 * Add a msg the queue.
	 */
	cs_ret_code_t addToQueue(MeshUtil::cs_mesh_queue_item_t& item);

	/**
	 * Remove a msg from the queue.
	 */
	cs_ret_code_t remFromQueue(cs_mesh_model_msg_type_t type, uint16_t id);

	/**
	 * To be called at a regular interval.
	 */
	void tick(uint32_t tickCount);

	/** Internal usage */
	void handleMsg(const access_message_rx_t * accessMsg);

	/** Internal usage */
	void handleReliableStatus(access_reliable_status_t status);

private:
	const static uint8_t queue_size = 5;

	const static uint8_t queue_index_none = 255;

	struct __attribute__((__packed__)) cs_unicast_queue_item_t {
		MeshUtil::cs_mesh_queue_item_meta_data_t metaData;
		stone_id_t targetId;
		uint8_t msgSize;
		uint8_t* msgPtr = nullptr;
	};

	access_model_handle_t _accessModelHandle = ACCESS_HANDLE_INVALID;

	dsm_handle_t _publishAddressHandle = DSM_HANDLE_INVALID;

	callback_msg_t _msgCallback = nullptr;

	access_reliable_t _accessReliableMsg;

#if MESH_MODEL_TEST_MSG == 2
	uint32_t _acked = 0;
	uint32_t _timedout = 0;
	uint32_t _canceled = 0;
#endif

	cs_unicast_queue_item_t _queue[queue_size];

	/**
	 * Queue index of message currently being sent.
	 */
	uint8_t _queueIndexInProgress = queue_index_none;

	/**
	 * Next index in queue to send.
	 */
	uint8_t _queueIndexNext = 0;

	/**
	 * Status of the reliable msg.
	 * 255 for no status.
	 */
	uint8_t _reliableStatus = 255;

	/**
	 * Whether the reply message has been received.
	 */
	bool _replyReceived = false;

	uint8_t _ttl = CS_MESH_DEFAULT_TTL;

	/**
	 * If item at index is in progress, cancel it.
	 */
	void cancelQueueItem(uint8_t index);

	/**
	 * Remove an item from the queue
	 */
	void remQueueItem(uint8_t index);

	/**
	 * Send messages from queue.
	 */
	void processQueue();

	/**
	 * Check if there is a msg in queue with more than 0 transmissions.
	 * If so, return that index.
	 * Start looking at index SendIndex as that item should be sent first.
	 * Returns -1 if none found.
	 */
	int getNextItemInQueue(bool priority);

	/**
	 * Get a msg from the queue, and send it.
	 * Returns true when message was sent, false when no more messages to be sent.
	 */
	bool sendMsgFromQueue();

	/**
	 * Check if a message is done (success or timed out).
	 */
	void checkDone();

	/**
	 * Send a unicast message over the mesh.
	 *
	 * This message will be have to acked or timed out,
	 * before the next message can be sent.
	 *
	 * Message data has to stay in ram until acked or timedout!
	 */
	cs_ret_code_t sendMsg(const uint8_t* msg, uint16_t msgSize, uint32_t timeoutUs);

	/**
	 * Send a reply when receiving a reliable message.
	 */
	cs_ret_code_t sendReply(const access_message_rx_t* accessMsg, const uint8_t* msg, uint16_t msgSize);

	/**
	 * Sets the publish address.
	 *
	 * Do this while no message is in progress.
	 */
	cs_ret_code_t setPublishAddress(stone_id_t id);

	/**
	 * Sets the TTL.
	 *
	 * Do this while no message is in progress.
	 */
	cs_ret_code_t setTtl(uint8_t ttl, bool temp = false);

	void sendFailedResultToUart(stone_id_t id, cs_mesh_model_msg_type_t msgType, cs_ret_code_t retCode);
};
