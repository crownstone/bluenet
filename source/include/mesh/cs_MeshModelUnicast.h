/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <mesh/cs_MeshCommon.h>
#include <third/std/function.h>

extern "C" {
#include <access_reliable.h>
}

/**
 * Class that:
 * - Sends and receives targeted segmented messages.
 * - Queues messages to be sent.
 */
class MeshModelUnicast {
public:
	/** Callback function definition. */
	typedef function<void(const MeshUtil::cs_mesh_received_msg_t& msg)> callback_msg_t;

	/**
	 * Register a callback function that's called when a message from the mesh is received.
	 *
	 * Must be done before init.
	 */
	void registerMsgHandler(const callback_msg_t& closure);

	void init(uint16_t modelId);

	void configureSelf(dsm_handle_t appkeyHandle);

	/**
	 * Add a msg to an empty spot in the queue (repeats == 0).
	 * Start looking at SendIndex, then reverse iterate over the queue.
	 * Then set the new SendIndex at the newly added item, so that it will be send first.
	 * We do the reverse iterate, so that the old SendIndex should be handled early (for a large enough queue).
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
	const static uint8_t _queueSize = 5;

	struct __attribute__((__packed__)) cs_unicast_queue_item_t {
		MeshUtil::cs_mesh_queue_item_meta_data_t metaData;
		uint8_t* msgPtr = nullptr;
	};

	access_model_handle_t _accessModelHandle = ACCESS_HANDLE_INVALID;

	dsm_handle_t _publishAddressHandle = DSM_HANDLE_INVALID;

	callback_msg_t _msgCallback = nullptr;

	access_reliable_t _accessReliableMsg;

	/**
	 * Queue index of message currently being sent.
	 * -1 for none.
	 */
	uint8_t _queueIndexInProgress = -1;

	cs_unicast_queue_item_t _queue[_queueSize] = {0};

	uint32_t _ackTimeoutUs = 10 * 1000 * 1000; // MODEL_ACKNOWLEDGED_TRANSACTION_TIMEOUT

	/**
	 * Next index in queue to send.
	 */
	uint8_t _queueIndexNext = 0;

	/**
	 * Remove an item from the queue
	 */
	void remQueueItem(uint8_t index);

	/**
	 * Send messages from queue.
	 */
	void processQueue();

	/**
	 * Check if there is a msg in queue with more than 0 repeats.
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
	 * Send a unicast message over the mesh.
	 *
	 * This message will be have to acked or timed out,
	 * before the next message can be sent.
	 *
	 * Message data has to stay in ram until acked or timedout!
	 */
	cs_ret_code_t sendMsg(const uint8_t* msg, uint16_t msgSize);

	cs_ret_code_t setPublishAddress(stone_id_t id);
};
