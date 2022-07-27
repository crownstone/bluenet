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
	cs_mesh_queue_item_meta_data_t metaData = {};
	bool reliable                           = false;
	bool broadcast                          = true;
	bool noHop                              = false;
	cs_control_cmd_t controlCommand         = CTRL_CMD_UNKNOWN;
	uint8_t numIds                          = 0;
	stone_id_t* stoneIdsPtr                 = nullptr;
	cs_data_t msgPayload                    = {};
};

#define printMeshQueueItem(modelName, meshQueueItemMetaData) \
		LOGMeshModelDebug(modelName " id=%u type=%u priority=%u transmissionsOrTimeout=%u", \
			meshQueueItemMetaData.id,                                                       \
			meshQueueItemMetaData.type,                                                     \
			meshQueueItemMetaData.priority,                                                 \
			meshQueueItemMetaData.transmissionsOrTimeout                                    \
	);

}
