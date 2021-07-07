/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 1, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <mesh/cs_MeshCommon.h>
#include <third/std/function.h>
#include <util/cs_BitmaskVarSize.h>

extern "C" {
#include <access.h>
}

/**
 * Class that:
 * - Sends and receives multicast acked messages.
 * - Queues messages to be sent.
 * - Handles queue 1 by 1.
 */
class MeshModelMulticastAcked {
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
	 * Add a msg to an empty spot in the queue (transmissions == 0).
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

private:
	const static uint8_t queue_size = 5;

	const static uint8_t queue_index_none = 255;

	struct __attribute__((__packed__)) cs_multicast_acked_queue_item_t {
		MeshUtil::cs_mesh_queue_item_meta_data_t metaData;
		uint8_t numIds;
		stone_id_t* stoneIdsPtr;

		uint8_t msgSize;
		uint8_t* msgPtr = nullptr;
	};

	access_model_handle_t _accessModelHandle = ACCESS_HANDLE_INVALID;

	dsm_handle_t _groupAddressHandle = DSM_HANDLE_INVALID;

	callback_msg_t _msgCallback = nullptr;

	cs_multicast_acked_queue_item_t _queue[queue_size];

	/**
	 * Queue index of message currently being sent.
	 */
	uint8_t _queueIndexInProgress = queue_index_none;

	/**
	 * Next index in queue to send.
	 */
	uint8_t _queueIndexNext = 0;

	/**
	 * Bitmask of acked stones.
	 * If the Nth bit is set, the ack of Nth stone ID in the list has been received.
	 */
	BitmaskVarSize _ackedStonesBitmask;

//	/**
//	 * Whether the current message has been handled by this stone yet.
//	 */
//	bool _handledSelf = false;

	/**
	 *
	 */
	TYPIFY(CONFIG_CROWNSTONE_ID) _ownStoneId = 0;

	/**
	 * Number of processQueue() calls left until timeout.
	 */
	uint16_t _processCallsLeft;

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
	 * Prepare for sending a new message.
	 */
	bool prepareForMsg(cs_multicast_acked_queue_item_t* item);

	/**
	 * Send a message over the mesh via publish, without reply.
	 */
	cs_ret_code_t sendMsg(const uint8_t* data, uint16_t len);

	/**
	 * Send an ack message.
	 */
	void sendReply(const access_message_rx_t* accessMsg, const uint8_t* data, uint16_t len);

	/**
	 * Handle an ack message.
	 */
	void handleReply(MeshUtil::cs_mesh_received_msg_t & msg);

	/**
	 * Check if ack from every stone ID in the list has been received.
	 * Also check if timed out.
	 */
	void checkDone();

	/**
	 * Retry sending (parts of) the message.
	 */
	void retryMsg();
};
