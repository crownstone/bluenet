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
	Coroutine logRoutine;

	// stores the relevant history, the stone id pairs are order dependent: (sender, receiver)
	using StonePair = std::pair<stone_id_t,stone_id_t>;

	template <class T>
	using StonePairMap = std::map<StonePair, T>;

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
	 * Sends an update to the host containing the gathered information
	 * about all the StonePairs currently know to this device.
	 */
	uint32_t sendUpdateToHost();

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
	 * Extract StonePair and then logs:
	 * - recordSampleId
	 * - recordRssi
	 */
	void recordPingMsg(rssi_ping_message_t* ping_msg);

	void recordSampleId(StonePair, uint8_t sample_id);
	void recordRssiValue(StonePair stone_pair, int8_t rssi);

	/**
	 * Filters out ping messages that have been seen recently.
	 *
	 * Extracts stonepair = {sender, recipient} from p and if
	 * p also has a sample index equal to the last one logged
	 * with recordSampleId, nullptr is returned.
	 * Else p is returned.
	 */
	rssi_ping_message_t* filterSampleIndex(rssi_ping_message_t* p);

	// mapping utils
	// Note: the stone id pairs are order dependent: (sender, receiver)
	inline StonePair getKey(stone_id_t i, stone_id_t j){
		return {i,j};
	}
	inline StonePair getKey(rssi_ping_message_t* p){
		return getKey(p->sender_id, p->recipient_id);
	}
};
