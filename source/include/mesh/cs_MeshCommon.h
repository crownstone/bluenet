/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>

#define LOGMeshInfo LOGi
#define LOGMeshDebug LOGnone

#define LOGMeshModelInfo    LOGi
#define LOGMeshModelDebug   LOGd
#define LOGMeshModelVerbose LOGnone

/*
 * 0 to disable test.
 * 1 for unacked, unsegmented drop test.
 *   This assumes you have a node with id 2 (sending node).
 * 2 for targeted, segmented acked test.
 *   This assumes you have 2 nodes: one with id 1 (receiving node), and one with id 2 (sending node).
 */
#define MESH_MODEL_TEST_MSG 0

/**
 * Interval at which processQueue() gets called.
 */
#define MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS 100

/**
 * Number of messages sent each time processQueue() gets called.
 */
#define MESH_MODEL_QUEUE_BURST_COUNT 3

/**
 * Mesh utils without dependencies on mesh SDK.
 */
namespace MeshUtil {

struct __attribute__((__packed__)) cs_mesh_queue_item_meta_data_t {
	uint16_t id; // ID that can freely be used to find similar items.
	uint8_t type;
	stone_id_t targetId = 0;   // 0 for broadcast
	bool priority : 1;
	bool reliable : 1;
	uint8_t repeats : 6;
	uint8_t msgSize = 0;
};

struct __attribute__((__packed__)) cs_mesh_queue_item_t {
	cs_mesh_queue_item_meta_data_t metaData;
	uint8_t* msgPtr = nullptr;
};

void printQueueItem(const char* prefix, const cs_mesh_queue_item_meta_data_t& metaData);

}
