/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <events/cs_EventListener.h>
#include <structs/cs_PacketsInternal.h>
#include <localisation/cs_RssiDataMessage.h>

#include <util/cs_Coroutine.h>
#include <util/cs_Variance.h>

#include <map>

// REVIEW: Rename to MeshConnectivity or so?
/**
 * This component monitors bluetooth messages in order to keep track of the Rssi
 * distances between crownstones in the mesh. It regularly pushes rssi information
 * over UART.
 */
class RssiDataTracker : public EventListener {
public:
	RssiDataTracker();

	// REVIEW: Implementation details.
	/**
	 * CS_TICK:
	 *   Coroutine for rssi data updates.
	 *
	 *
	 * CS_TYPE_EVT_RECV_MESH_MSG:
	 *   if the hop count of the message is 0:
	 *    - recordRssiValue(args...)
	 *
	 *   if type is CS_MESH_MODEL_TYPE_RSSI_PING:
	 *   	 - if the hop count is 0:
	 *         - sendPingResponseOverMesh()
	 *
	 *   if type is CS_MESH_MODEL_TYPE_RSSI_DATA:
	 *     - sendRssiDataOverUart(arg...)
	 */
	void handleEvent(event_t& evt);

	/**
	 * Obtains the stone_id_t of this crownstone to use for forwarded ping messages.
	 */
	void init();

private:
	stone_id_t my_id = 0xff;

	// stores the relevant history, per neighbor stone_id.
	static constexpr uint8_t CHANNEL_COUNT = 3;
	static constexpr uint8_t CHANNEL_START = 37;

	// REVIEW: Aren't maps actually slower than arrays for a small number of elements?
	// REVIEW: Elements never time out, so these maps can keep on growing.
	// REVIEW: Last rssi map is not used.
	std::map<stone_id_t,int8_t> last_rssi_map[CHANNEL_COUNT] = {};
	std::map<stone_id_t,OnlineVarianceRecorder> recorder_map[CHANNEL_COUNT] = {}; // REVIEW: why not call it varianceMap or so?

	// will be set to true by coroutine to flush data after startup.
	bool boot_sequence_finished = false;

	// --------------- Coroutine parameters ---------------

	Coroutine flushRoutine;

	struct RssiDataTrackerTiming {
		/**
		 * When flushAggregatedRssiData is in the flushing phase,
		 * only recorders that have accumulated this many samples will
		 * be included.
		 */
		uint8_t min_samples_to_trigger_burst;

		// REVIEW: Doesn't say what this burst_period_ms does.
		// REVIEW: Maybe have a queue of stone ids to send? so you can just send them 1 by 1,
		//         with any delay in between (large for normal operation, smaller when data is requested).
		//         I'm not a big fan of the snowball risk.
		/**
		 * Note: if the mesh is very active, setting this delay higher is risky.
		 * When we accumulate more then min_samples_to_trigger_burst
		 * samples _during_ the burst phase, it will be propagated a second time in same burst.
		 * Hence a low value for that constant makes it possible to keep running in burst
		 * mode. (If multiple nodes are bursting, this effect will snowball!)
		 */
		uint32_t burst_period_ms;

		/**
		 * This value determines how often bursts occur. It is much less sensitive
		 * than burst_period_ms and min_samples_to_trigger_burst.
		 */
		uint32_t accumulation_period_ms;

		/**
		 * When boot sequence period expires, a flush of the rssi data will be
		 * triggered.
		 */
		uint32_t boot_sequence_period_ms;
	};

	static constexpr RssiDataTrackerTiming Settings = {
#ifdef DEBUG
		.min_samples_to_trigger_burst = 20,
		.burst_period_ms = 500,
		.accumulation_period_ms = 2 * 60 * 1000,
		.boot_sequence_period_ms = 1 * 60 * 1000
#else
		.min_samples_to_trigger_burst = 20,
		.burst_period_ms = 5,
		.accumulation_period_ms = 30 * 60 * 1000,
		.boot_sequence_period_ms = 1 * 60 * 1000,
#endif
	};

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

	/**
	 * Returns the 3 bit descriptor of the given standard deviation as defined
	 * in cs_PacketsInternal.h.
	 */
	inline uint8_t getStdDevRepresentation(float standard_deviation);

	/**
	 * Returns the 7 bit representation of the given mean as defined
	 * in cs_PacketsInternal.h.
	 */
	inline uint8_t getMeanRepresentation(float mean);

	/**
	 * Returns the 6 bit representation of the given count as defined
	 * in cs_PacketsInternal.h.
	 */
	inline uint8_t getCountRepresentation(uint32_t count);

	// --------------- generating rssi data --------------

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
	 *  - sendPingResponseOverMesh()
	 */
	void receivePingMessage(MeshMsgEvent& mesh_msg_evt);

	// ------------- communicating rssi data -------------

	/**
	 * Dispatches an event of type CMD_SEND_MESH_MSG
	 * in order to send a CS_MESH_MODEL_TYPE_RSSI_DATA.
	 */
	void sendRssiDataOverMesh(rssi_data_message_t* rssi_data_message);

	/**
	 * Writes a message of type UART_OPCODE_TX_RSSI_DATA_MESSAGE
	 * with the given parameter as data.
	 */
	void sendRssiDataOverUart(rssi_data_message_t* rssi_data_message);

	/**
	 * Any received rssi_data_message_t will be sendRssiDataOverUart.
	 *
	 * Assumes mesh_msg_event is of type CS_MESH_MODEL_TYPE_RSSI_DATA.
	 */
	void receiveRssiDataMessage(MeshMsgEvent& mesh_msg_evt);


	// ------------- recording mesh messages -------------

	/**
	 * If the event was no-hop:
	 * 	- recordRssiValue(args...)
	 */
	void receiveMeshMsgEvent(MeshMsgEvent& mesh_msg_evt);

	/**
	 * Saves rssi value to last received map and variance recorder map.
	 * If the long term recorder has accumulated a lot of data, it will
	 * be reduced to prevent overflow.
	 */
	void recordRssiValue(stone_id_t sender_id, int8_t rssi, uint8_t channel);
};
