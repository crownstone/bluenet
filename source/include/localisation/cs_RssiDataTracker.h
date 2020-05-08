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


/**
 * Launches ping messages into the mesh and receives them.
 * Exposes a static API for other firmware components to
 * query the data.
 */
class RssiDataTracker : public EventListener {
public:
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


	void forwardPingMsgOverMesh(rssi_ping_message_t* primary_ping_msg);

	void forwardPingMsgToTestSuite(rssi_ping_message_t* secondary_ping_msg);


	stone_id_t my_id = 0xff;
	uint32_t max_ping_msgs_per_s = 5;

};
