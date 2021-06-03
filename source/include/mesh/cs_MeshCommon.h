/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>
#include <mesh/cs_MeshDefines.h>
#include <mesh/cs_MeshMsgEvent.h>

/**
 * Mesh utils without dependencies on mesh SDK.
 */
namespace MeshUtil {

struct __attribute__((__packed__)) cs_mesh_received_msg_t {
	uint16_t opCode;
	uint16_t srcAddress; // of the original sender
	bool macAddressValid; // True when the following MAC address is valid.
	uint8_t macAddress[MAC_ADDRESS_LEN]; // MAC address of the relaying node, which is the src in case of 0 hops.
	uint8_t* msg;
	uint8_t msgSize;
	int8_t rssi;
	uint8_t hops;
	uint8_t channel;
};

// Data needed in each model queue.
struct __attribute__((__packed__)) cs_mesh_queue_item_meta_data_t {
	uint16_t id = 0; // ID that can freely be used to find similar items.
	uint8_t type = CS_MESH_MODEL_TYPE_UNKNOWN; // Mesh msg type.
//	stone_id_t targetId = 0;   // 0 for broadcast
	uint8_t transmissionsOrTimeout : 6; // Timeout in seconds when reliable, else number of transmissions.
	bool priority : 1;
	bool noHop : 1;

	cs_mesh_queue_item_meta_data_t():
		transmissionsOrTimeout(0),
		priority(false),
		noHop(false)
	{}
};

/**
 * Struct to queue an item.
 * Data is temporary, so has to be copied.
 */
struct cs_mesh_queue_item_t {
	cs_mesh_queue_item_meta_data_t metaData;
	bool reliable = false;
	bool broadcast = true;
	bool noHop = false;
	uint8_t numIds = 0;
	stone_id_t* stoneIdsPtr = nullptr;
	cs_data_t msgPayload;
};

void printQueueItem(const char* prefix, const cs_mesh_queue_item_meta_data_t& metaData);

}
