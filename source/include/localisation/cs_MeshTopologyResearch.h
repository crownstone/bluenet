/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <events/cs_EventListener.h>
#include <map>
#include <protocol/cs_MeshTopologyPackets.h>
#include <structs/cs_PacketsInternal.h>
#include <util/cs_Coroutine.h>
#include <util/cs_Variance.h>

/**
 * This class/component keeps track of the rssi distance of a
 * crownstone to its neighbors (i.e. those within zero hop distance
 * on the mesh).
 *
 * At configurable intervals it will broadcast its acquired data in a burst
 * over the mesh so that a hub, or other interested connected devices
 * can retrieve it.
 */
class MeshTopologyResearch : public EventListener {
public:
	MeshTopologyResearch();

	/**
	 * Handles the following events:
	 * CS_TICK:
	 *   Coroutine for rssi data updates.
	 *
	 *
	 * CS_TYPE_EVT_RECV_MESH_MSG of types:
	 *   - CS_MESH_MODEL_TYPE_RSSI_PING
	 *   - CS_MESH_MODEL_TYPE_RSSI_DATA
	 *
	 * (Will propagate data over UART for the hub to use it.)
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

	// TODO: change maps to object for small ram memory footprint optimization
	std::map<stone_id_t,VarianceAggregator> variance_map[CHANNEL_COUNT] = {};

	// will be set to true by coroutine to flush data after startup.
	bool boot_sequence_finished = false;

	// loop variables to keep track of outside of coroutine for the burst loop
	stone_id_t last_stone_id_broadcasted_in_burst = 0;

	// --------------- Coroutine parameters ---------------

	Coroutine flushRoutine;

	struct TimingSettings {
		/**
		 * When flushAggregatedRssiData is in the flushing phase,
		 * only variance_map entries that have accumulated this many samples will
		 * be included.
		 */
		uint8_t min_samples_to_trigger_burst;

		/**
		 * Period between sending data in 'burst mode'.
		 */
		uint32_t burst_period_ms;

		/**
		 * This value determines the period between end and start of bursts.
		 */
		uint32_t accumulation_period_ms;

		/**
		 * When boot sequence period expires, a flush (burst) of the rssi data will be
		 * triggered.
		 */
		uint32_t boot_sequence_period_ms;
	};

	static constexpr TimingSettings Settings = {
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
	 * Returns the 3 bit descriptor of the given variance as defined
	 * in cs_PacketsInternal.h.
	 */
	inline uint8_t getVarianceRepresentation(float standard_deviation);

	/**
	 * Returns the 7 bit representation of the given mean as defined
	 * in cs_PacketsInternal.h.
	 */
	inline uint8_t getMeanRssiRepresentation(float mean);

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
