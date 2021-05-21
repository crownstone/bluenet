/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 30, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <protocol/cs_Typedefs.h>
#include <util/cs_Rssi.h>

static constexpr uint8_t MESH_TOPOLOGY_CHANNEL_COUNT = 3;


struct __attribute__((packed)) mesh_topology_neighbour_rssi_t {
	// TODO: change this to include RSSI of all channels?
	stone_id_t receiverId;
	stone_id_t senderId;
	uint8_t type = 0;
	int8_t rssi;
	uint8_t channel;
//	int8_t rssi[MESH_TOPOLOGY_CHANNEL_COUNT];
	uint8_t lastSeenSecondsAgo; // How many seconds ago the sender was last seen by the receiver.
};


/**
 * Message format to be sent over uart.
 * This is the inflated counterpart of rssi_data_message_t.
 *
 * (Necessary since we have to fold in our own id anyway.)
 */
struct __attribute__((packed)) mesh_topology_neighbour_research_rssi_t {
	stone_id_t receiverId;
	stone_id_t senderId;
	uint8_t count[MESH_TOPOLOGY_CHANNEL_COUNT];
	uint8_t rssi[MESH_TOPOLOGY_CHANNEL_COUNT];
	uint8_t standardDeviation[MESH_TOPOLOGY_CHANNEL_COUNT];
};

