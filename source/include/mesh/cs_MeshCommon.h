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

/**
 * Data needed in each model queue.
 */
struct __attribute__((__packed__)) cs_mesh_queue_item_meta_data_t {
	//! ID that can freely be used to find similar items.
	uint16_t id  = 0;

	//! Mesh msg type.
	uint8_t type = CS_MESH_MODEL_TYPE_UNKNOWN;

	/**
	 * Timeout in seconds when reliable, else number of transmissions.
	 * Set to 0 to use the default value.
	 */
	uint8_t transmissionsOrTimeout : 6;

	//! Whether this item has priority.
	bool priority : 1;

	//! Whether this message should be sent to direct neighbours only.
	bool doNotRelay : 1;

	cs_mesh_queue_item_meta_data_t() : transmissionsOrTimeout(0), priority(false), doNotRelay(false) {}
};

/**
 * Struct to queue an item.
 * Data is temporary, so has to be copied.
 */
struct cs_mesh_queue_item_t {
	//! Metadata
	cs_mesh_queue_item_meta_data_t metaData = {};

	//! Whether the message should be acked.
	bool acked                              = false;

	//! Whether the message should be broadcasted.
	bool broadcast                          = true;

	//! Whether this message should be sent to direct neighbours only.
	bool doNotRelay                         = false;

	/**
	 * If this mesh message is sent because of a Mesh command, then set this field to the control command
	 * payload of that mesh command.
	 * It is used to send back the result of the mesh command.
	 */
	cs_control_cmd_t controlCommand         = CTRL_CMD_UNKNOWN;

	//! Number of target stone IDs.
	uint8_t numStoneIds                     = 0;

	//! Pointer to the list of target stone IDs.
	stone_id_t* stoneIdsPtr                 = nullptr;

	//! The message payload.
	cs_data_t msgPayload                    = {};
};

#define printMeshQueueItem(modelName, meshQueueItemMetaData)                  \
	LOGMeshModelDebug(                                                        \
			modelName " id=%u type=%u priority=%u transmissionsOrTimeout=%u", \
			meshQueueItemMetaData.id,                                         \
			meshQueueItemMetaData.type,                                       \
			meshQueueItemMetaData.priority,                                   \
			meshQueueItemMetaData.transmissionsOrTimeout);

}  // namespace MeshUtil
