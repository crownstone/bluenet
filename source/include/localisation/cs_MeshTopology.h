/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 30, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <events/cs_EventListener.h>
#include <protocol/cs_MeshTopologyPackets.h>

#if BUILD_MESH_TOPOLOGY_RESEARCH == 1
#include <localisation/cs_MeshTopologyResearch.h>
#endif

class MeshTopology: EventListener {
public:
	/**
	 * Maximum number of neighbours in the list.
	 */
	static constexpr uint8_t MAX_NEIGHBOURS = 50;

	/**
	 * Time after last seen, before a neighbour is removed from the list.
	 */
	static constexpr uint8_t TIMEOUT_SECONDS = 50;

	/**
	 * Interval at which a mesh messages is sent for each neighbour.
	 */
	static constexpr uint16_t SEND_INTERVAL_SECONDS_PER_NEIGHBOUR = 1*60;

	/**
	 * Initializes the class:
	 * - Reads State.
	 * - Allocates buffer.
	 * - Starts listening for events.
	 */
	cs_ret_code_t init();

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

private:
	static constexpr uint8_t INDEX_NOT_FOUND = 0xFF;

	/**
	 * Stone ID of this crownstone, read on init.
	 */
	stone_id_t _myId = 0;

	/**
	 * A list of all known neighbours, allocated on init.
	 */
	cs_mesh_model_msg_neighbour_rssi_t* _neighbours = nullptr;

	/**
	 * Number of neighbours in the list.
	 */
	uint8_t _neighbourCount = 0;

	/**
	 * Next index of the neighbours list to send via the mesh.
	 */
	uint8_t _nextSendIndex = 0;

	/**
	 * Countdown in seconds until sending the next mesh message.
	 */
	uint16_t _sendCountdown = SEND_INTERVAL_SECONDS_PER_NEIGHBOUR;

	/**
	 * Add a neighbour to the list.
	 */
	void add(stone_id_t id, int8_t rssi, uint8_t channel);

	/**
	 * Find a neighbour in the list.
	 */
	uint8_t find(stone_id_t id);

	/**
	 * Sends the RSSI of 1 neighbour over the mesh and UART.
	 */
	void sendNext();

	/**
	 * Sends the RSSI to a neighbour over UART.
	 */
	void sendRssiToUart(stone_id_t reveiverId, cs_mesh_model_msg_neighbour_rssi_t& packet);

	// Event handlers
	void onNeighbourRssi(stone_id_t id, cs_mesh_model_msg_neighbour_rssi_t& packet);
	cs_ret_code_t onStoneMacMsg(stone_id_t id, cs_mesh_model_msg_stone_mac_t& packet, mesh_reply_t* result);
	void onMeshMsg(MeshMsgEvent& packet, cs_result_t& result);
	void onTickSecond();

	/**
	 * Print all neighbours.
	 */
	void print();

public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t &evt);

#if BUILD_MESH_TOPOLOGY_RESEARCH == 1
private:
	MeshTopologyResearch _research;
#endif
};


