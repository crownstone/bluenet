/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 11, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_Serial.h>
#include <mesh/cs_MeshCommon.h>

#if TICK_INTERVAL_MS > MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS
#error "TICK_INTERVAL_MS must not be larger than MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS"
#endif

namespace MeshUtil {

void printQueueItem(const char* prefix, const cs_mesh_queue_item_meta_data_t& metaData) {
	LOGMeshModelDebug("%s id=%u type=%u targetId=%u priority=%u reliable=%u repeats=%u",
			metaData.id,
			metaData.type,
			metaData.targetId,
			metaData.priority,
			metaData.reliable,
			metaData.repeats
	);
}

}
