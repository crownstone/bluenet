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
 * Launches ping messages into the mesh and receives them in order to
 * push Rssi information to a (debug) host, and allow other firmware
 * components to query for that information.
 *
 * Three primary mechanisms drive this component:
 * - Primary pingmessages:
 *   These are mesh messages of type CS_MESH_MODEL_TYPE_RSSI_PING,
 *   where the payload is a partially filled rssi_ping_message_t struct.
 *   (Only the sample_id is filled to enable duplication filter.)
 *
 *   When a node in the mesh receives a primary ping message, it will
 *   respond by constructing a secondary ping message with the given
 *   sample_id and sending that back.
 *
 * - Secondary pingmessages:
 *   These are mesh messages of type CS_MESH_MODEL_TYPE_RSSI_PING,
 *   where the payload is a completely filled rssi_ping_message_t struct.
 *   i.e.: .rssi != 0 && .sender_id != 0 && .recipient_id != 0.
 *
 *   When a node receives a secondary ping message it will record it to
 *   its 'local data base' for duplicate filtering, tracking the
 *   rssi data per sender-recipient pair and sending that data over UART
 *   to a (debugging) host.
 *
 * - Generic mesh messages:
 *   The RssiDataTracker responds to EVT_RECV_MESH_MSG events and
 *   captures the sender, receiver and rssi values of those events.
 *   These values will aggregated and stored in its the 'local data base'
 *   in order to compute longer term averages and variance.
 *
 * - Database flushing:
 *   Periodically the local data base will be flushed onto the mesh in order
 *   to push the information to a (debug) host. This happens in the form
 *   of a series of secondary ping messages. This burst is throttled to
 *   ensure the RssiDataTracker-s don't flood the mesh.
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
	Coroutine flushRoutine;

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
	 * Coroutine method:
	 * - Ping period: 5 minutes
	 */
	uint32_t sendPrimaryPingMessage();

	/**
	 * Checks is ping_msg contains all the data to be called a secondary
	 * ping message and send it if it suffices.
	 *
	 * Return true if it was a secondary message, else false.
	 */
	bool sendSecondaryPingMsg(rssi_ping_message_t* ping_msg);

	/**
	 * Sends the given ping_msg over the mesh without any further checking.
	 */
	void sendPingMsg(rssi_ping_message_t* ping_msg);

	/**
	 * Sends a secondary ping message for each of the pairs of crownstones
	 * in the local maps, together with the (oriented) average rssi value
	 * between those stones. After that, clear all the maps.
	 *
	 * Coroutine method:
	 * To prevent the mesh from flooding, flushing is throttled.
	 * - Flush period: 30 minutes
	 * - Burst period: 5 milliseconds
	 */
	uint32_t flushAggregatedRssiData();

	void pushPingMsgToHost(rssi_ping_message_t* ping_msg);

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
	 * We forge a ping_message, bypass duplicate filtering and
	 * call recordRssiValue(ping_msg).
	 */
	void handleGenericMeshMessage(MeshMsgEvent* mesh_msg_evt);

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
