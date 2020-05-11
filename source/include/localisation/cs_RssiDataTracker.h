/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <events/cs_EventListener.h>
#include <localisation/cs_RssiPingMessage.h>

#include <util/cs_Coroutine.h>


/**
 * Launches ping messages into the mesh and receives them.
 * Exposes a static API for other firmware components to
 * query the data.
 */
class RssiDataTracker : public EventListener {
public:
	RssiDataTracker();

	/**
	 * When receiving a ping msg from another crownstone,
	 * broadcast one more time with the id of this crownstone
	 * filled in.
	 *
	 * When receiving a ping message with both sender and receiver
	 * filled in, push data to test host.
	 *
	 */
	void handleEvent(event_t& evt);

	/**
	 * Obtains the stone_id_t of this crownstone to use for forwarded ping messages.
	 */
	void init();

	/**
	 * Sends a primary ping message over the mesh, containing only the 'ping counter'
	 * of this RssiDataTracker.
	 *
	 * Returns the number of ticks to wait before sending the next.
	 */
	uint32_t sendPrimaryPingMessage();
	void forwardPingMsgOverMesh(rssi_ping_message_t* primary_ping_msg);
	void forwardPingMsgToTestSuite(rssi_ping_message_t* secondary_ping_msg);


private:
	stone_id_t my_id = 0xff;
	uint32_t max_ping_msgs_per_s = 5;
	uint8_t ping_sample_index = 0;

	Coroutine loggingRoutine;
	Coroutine pingRoutine;

};
