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
#include <localisation/cs_RssiDataMessage.h>

#include <util/cs_Coroutine.h>
#include <util/cs_Variance.h>

#include <map>

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
	 * input:
	 *  - EVT_MESH_RSSI_PING
	 *  - EVT_RECV_MESH_MSG
	 * output:
	 *  - CMD_SEND_MESH_MSG -> CS_MESH_MODEL_TYPE_RSSI_PING
	 */
	void handleEvent(event_t& evt);

	/**
	 * Obtains the stone_id_t of this crownstone to use for forwarded ping messages.
	 */
	void init();

private:
	stone_id_t my_id = 0xff;

	// stores the relevant history, per neighbor stone_id.
	std::map<stone_id_t,int8_t> last_received_rssi = {};
	std::map<stone_id_t,OnlineVarianceRecorder> variance_recorders = {};

	// --------------- Coroutine parameters ---------------

	Coroutine flushRoutine;

	/**
	 * When flushAggregatedRssiData is in the flushing phase,
	 * only recorders that have accumulated this many samples will
	 * be included.
	 */
	static constexpr uint8_t min_samples_to_trigger_burst = 20;

	/**
	 * Note: if the mesh is very active, setting this delay higher is risky.
	 * When a StonePair recorder accumulates more then min_samples_to_trigger_burst
	 * samples _during_ the burst phase, it will be propagated in same burst again.
	 * Hence a low value for that constant makes it possible to keep running in burst
	 * mode. (If multiple nodes are bursting, this effect will snowball!)
	 */
	static constexpr uint32_t burst_period_ms = 5;

	/**
	 * This value determines how often bursts occur. It is much less sensitive
	 * than burst_period_ms and min_samples_to_trigger_burst.
	 */
	static constexpr uint32_t accumulation_period_ms = 30 * 60 * 1000;

	// -------------------- Methods --------------------

	/**
	 * Coroutine method:
	 * To prevent the mesh from flooding, flushing is throttled.
	 * - Flush period: 30 minutes
	 * - Burst period: 5 milliseconds
	 *
	 * Broadcasts a rssi_data_message_t for each of the pairs of crownstones
	 * in the local maps, together with the (oriented) average rssi value
	 * between those stones. After that, clear all the maps.
	 *
	 */
	uint32_t flushAggregatedRssiData();

	// ------------- generating rssi data-------------

	/**
	 * Dispatches an event of type CMD_SEND_MESH_MSG
	 * in order to send a CS_MESH_MODEL_TYPE_RSSI_PING.
	 */
	void sendPingRequestOverMesh();

	/**
	 * Dispatches an event of type CMD_SEND_MESH_MSG
	 * in order to send a CMD_SEND_MESH_MSG_NOOP.
	 *
	 * (This nop will be handled by receiveGenericMeshMessage)
	 */
	void sendPingResponseOverMesh();

	/**
	 * If the ping message is a request that has not hopped,
	 * call sendPingResponseOverMesh(); Else do nothing.
	 */
	void receivePingMessage(rssi_ping_message_t* ping_msg);

	// ------------- communicating rssi data -------------

	/**
	 *
	 */
	void sendRssiDataOverMesh(rssi_data_message_t* rssi_data_message);

	/**
	 *
	 */
	void sendRssiDataOverUart(rssi_data_message_t* rssi_data_message);

	/**
	 * Any received rssi_data_message_t will be sendRssiDataOverUart
	 */
	void receiveRssiDataMessage(rssi_data_message_t* rssi_data_message);


	// ------------- recording mesh messages -------------

	/**
	 * If the hop count of the Records the rssi value data
	 */
	void receiveGenericMeshMessage(MeshMsgEvent* mesh_msg_evt);

	/**
	 * Saves rssi value to last received map and variance recorder map.
	 */
	void recordRssiValue(stone_id_t sender_id, int8_t rssi);
};
