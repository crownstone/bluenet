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
#include <util/cs_Variance.h>

#include <map>
#include <utility>  // for pair

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

private:
	stone_id_t my_id = 0xff;
	uint8_t ping_sample_index = 0;

	Coroutine pingRoutine;

	// stores the relevant history, the stone id pairs are order dependent: (sender, receiver)
	template <class T>
	using StonePairMap = std::map<std::pair<stone_id_t,stone_id_t>, T>;

	StonePairMap<uint8_t> last_received_sample_indices = {};
	StonePairMap<int8_t> last_received_rssi = {};
	StonePairMap<OnlineVarianceRecorder> variance_recorders = {};

	/**
	 * Sends a primary ping message over the mesh, containing only the 'ping counter'
	 * of this RssiDataTracker.
	 *
	 * Returns the number of ticks to wait before sending the next.
	 */
	uint32_t sendPrimaryPingMessage();

	/**
	 * Forwards message over mesh
	 * Records ping message
	 */
	void handlePrimaryPingMessage(rssi_ping_message_t* ping_msg);

	/**
	 * Forwards message to test suite
	 * Records ping message
	 */
	void handleSecondaryPingMessage(rssi_ping_message_t* ping_msg);


	/**
	 * Store ping message information in the tracking maps and send
	 * the ping message to host.
	 */
	void recordPingMsg(rssi_ping_message_t* ping_msg);

	/**
	 * Filters out ping messages that have been seen before.
	 *
	 * If p has a sample index that was received recently, return nullptr
	 * else return p.
	 */
	rssi_ping_message_t* filterSampleIndex(rssi_ping_message_t* p);

	// mapping utils
	// Note: the stone id pairs are order dependent: (sender, receiver)
	inline std::pair<stone_id_t,stone_id_t> getKey(stone_id_t i, stone_id_t j){
		return {i,j};
	}
	inline std::pair<stone_id_t,stone_id_t> getKey(rssi_ping_message_t* p){
		return getKey(p->sender_id, p->recipient_id);
	}
};
