/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 30, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <protocol/cs_MeshTopologyPackets.h>

#include <cstdint>

#if BUILD_MESH_TOPOLOGY_RESEARCH == 1
#include <localisation/cs_MeshTopologyResearch.h>
#endif

/**
 * Keeps track of the rssi distance of this crownstone to
 * the crownstones in its direct environment.
 *
 * Several related commands are available over UART.
 */
class MeshTopology: EventListener {
	/**
	 * Set this to true to make every crownstone in the sphere to send a noop message
	 * on every tick, and make every crownstone respond with a neighbor message with that rssi.
	 *
	 * (This gives the highest possible frequency trip wire)
	 */
	static constexpr bool TripwireResearch = false;

public:
	/**
	 * Initializes the class:
	 * - Reads State.
	 * - Allocates buffer.
	 * - Starts listening for events.
	 */
	cs_ret_code_t init();

	/**
	 * Commands handled:
	 *  - CMD_MESH_TOPO_GET_RSSI
	 *  - CMD_MESH_TOPO_RESET
	 *  - CMD_MESH_TOPO_GET_MAC
	 *
	 * Internal usage:
	 *  - EVT_RECV_MESH_MSG
	 *  - EVT_TICK
	 */
	void handleEvent(event_t &evt);

	/**
	 * Get the MAC address of a neighbouring crownstone.
	 *
	 * @param[in] stoneId    The stone ID of the crownstone.
	 *
	 * @return ERR_WAIT_FOR_SUCCESS    When the MAC address is requested. Wait for EVT_MESH_TOPO_MAC_RESULT.
	 *                                 However, there will be no event if it times out.
	 * @return ERR_NOT_FOUND           When the stone is not a known neighbour.
	 */
	cs_ret_code_t getMacAddress(stone_id_t stoneId);

	// --------------------------------
	// ---------- parameters ----------
	// --------------------------------

	/**
	 * Maximum number of neighbours in the list.
	 */
	static constexpr uint8_t MAX_NEIGHBOURS                            = 50;

	/**
	 * Time after last seen, before a neighbour is removed from the list.
	 */
	static constexpr uint8_t TIMEOUT_SECONDS                           = 3 * 60;

	/**
	 * Interval at which a mesh messages is sent for each neighbour.
	 */
	static constexpr uint16_t SEND_INTERVAL_SECONDS_PER_NEIGHBOUR      = 5 * 60;
	static constexpr uint16_t SEND_INTERVAL_SECONDS_PER_NEIGHBOUR_FAST = 10;

	/**
	 * Interval at which a no-hop noop message is sent.
	 *
	 * Should be lower than TIMEOUT_SECONDS, so that a message is sent before timeout.
	 */
	static constexpr uint16_t SEND_NOOP_INTERVAL_SECONDS               = 1 * 60;
	static constexpr uint16_t SEND_NOOP_INTERVAL_SECONDS_FAST          = 10;

	/**
	 * After a reset, the FAST intervals will be used instead, for this amount of seconds.
	 */
	static constexpr uint16_t FAST_INTERVAL_TIMEOUT_SECONDS            = 5 * 60;


private:
	static constexpr uint8_t INDEX_NOT_FOUND = 0xFF;

	static constexpr int8_t RSSI_INIT        = 0;  // Should be in protocol

	struct __attribute__((__packed__)) neighbour_node_t {
		stone_id_t id;
		int8_t rssiChannel37;
		int8_t rssiChannel38;
		int8_t rssiChannel39;
		uint8_t lastSeenSecondsAgo;
	};

	// ---------------------------------------
	// ---------- runtime variables ----------
	// ---------------------------------------

	/**
	 * Stone ID of this crownstone, read on init.
	 */
	stone_id_t _myId              = 0;

	/**
	 * A list of all known neighbours, allocated on init.
	 */
	neighbour_node_t* _neighbours = nullptr;

	/**
	 * Number of neighbours in the list.
	 */
	uint8_t _neighbourCount       = 0;

	/**
	 * Next index of the neighbours list to send via the mesh.
	 */
	uint8_t _nextSendIndex        = 0;

	/**
	 * Countdown in seconds until sending the next mesh message.
	 */
	uint16_t _sendCountdown;

	/**
	 * Countdown in seconds until sending the next no hop ping mesh message.
	 */
	uint16_t _sendNoopCountdown;

	/**
	 * Countdown in seconds until sending at regular interval again.
	 */
	uint16_t _fastIntervalCountdown;

	/**
	 * Overflowing message counter.
	 */
	uint8_t _msgCount = 0;

	// -------------------------------------
	// ---------- private methods ----------
	// -------------------------------------

	/**
	 * Resets the stored topology.
	 */
	void reset();

	/**
	 * Add a neighbour to the list.
	 */
	void add(stone_id_t id, int8_t rssi, uint8_t channel);

	/**
	 * Update the data of a node in the list.
	 */
	void updateNeighbour(neighbour_node_t& node, stone_id_t id, int8_t rssi, uint8_t channel);

	/**
	 * sets the three rssi values to RSSI_INIT
	 */
	void clearNeighbourRssi(neighbour_node_t& node);

	/**
	 * Find a neighbour in the list.
	 */
	uint8_t find(stone_id_t id);

	/**
	 * Get the RSSI of given stone ID and put it in the result buffer.
	 */
	void getRssi(stone_id_t stoneId, cs_result_t& result);

	/**
	 * Send a no-hop noop mesh message.
	 */
	void sendNoop();

	/**
	 * Sends the RSSI of 1 neighbour over the mesh and UART.
	 */
	void sendNext();

	/**
	 * Sends a neighbour message for the given node over the mesh.
	 * No checks are executed.
	 *
	 * Returns the message struct for convenience.
	 */
	cs_mesh_model_msg_neighbour_rssi_t sendNeighbourMessageOverMesh(neighbour_node_t& node);

	/**
	 * Sends the RSSI to a neighbour over UART.
	 */
	void sendRssiToUart(stone_id_t reveiverId, cs_mesh_model_msg_neighbour_rssi_t& packet);

	// ------------------------------------
	// ---------- event handlers ----------
	// ------------------------------------

	void onNeighbourRssi(stone_id_t id, cs_mesh_model_msg_neighbour_rssi_t& packet);

	cs_ret_code_t onStoneMacMsg(MeshMsgEvent& meshMsg);

	/**
	 * Handles mesh messages:
	 *  - CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI
	 *  - CS_MESH_MODEL_TYPE_STONE_MAC
	 *
	 *  Also calls `add` if .hops == 0, regarless of the packet type.
	 */
	void onMeshMsg(MeshMsgEvent& packet, cs_result_t& result);

	/**
	 * neighbors are removed from the list when their individual countdown expires.
	 * See TIMEOUT_SECONDS.
	 */
	void onTickSecond();

	/**
	 * Prints all neighbours.
	 */
	void print();

#if BUILD_MESH_TOPOLOGY_RESEARCH == 1
	MeshTopologyResearch _research;
#endif
};
