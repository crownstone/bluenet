/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>
#include <mesh/cs_MeshDefines.h>

/**
 * Mesh utils without dependencies on mesh SDK.
 */
namespace MeshUtil {

struct __attribute__((__packed__)) cs_mesh_received_msg_t {
	uint16_t opCode;
	uint16_t srcAddress;
	uint8_t* msg;
	uint8_t msgSize;
	int8_t rssi;
	uint8_t hops;
};

struct __attribute__((__packed__)) cs_mesh_queue_item_meta_data_t {
	uint16_t id = 0; // ID that can freely be used to find similar items.
	uint8_t type = CS_MESH_MODEL_TYPE_UNKNOWN; // Mesh msg type.
	stone_id_t targetId = 0;   // 0 for broadcast
	bool priority : 1;
	bool reliable : 1;
	uint8_t transmissionsOrTimeout : 6; // Timeout in seconds when reliable, else number of transmissions.

	cs_mesh_queue_item_meta_data_t():
		priority(false),
		reliable(false),
		transmissionsOrTimeout(0)
	{}
//	cs_mesh_queue_item_meta_data_t(uint16_t id, uint8_t type, stone_id_t targetId, bool priority, bool reliable, uint8_t transmissionsOrTimeout):
//		id(id),
//		type(type),
//		targetId(targetId),
//		priority(priority),
//		reliable(reliable),
//		transmissionsOrTimeout(transmissionsOrTimeout)
//	{}
};

struct __attribute__((__packed__)) cs_mesh_queue_item_t {
	cs_mesh_queue_item_meta_data_t metaData;
	uint8_t payloadSize = 0;
	uint8_t* payloadPtr = nullptr;

//	cs_mesh_queue_item_t():
//		metaData()
//	{}
//	cs_mesh_queue_item_t(cs_mesh_queue_item_meta_data_t & metaData, uint8_t payloadSize, uint8_t* payload):
//		metaData(metaData),
//		payloadSize(payloadSize),
//		payloadPtr(payload)
//	{}
};

void printQueueItem(const char* prefix, const cs_mesh_queue_item_meta_data_t& metaData);

}
