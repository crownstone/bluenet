/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 11, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <mesh/cs_MeshCommon.h>

#if TICK_INTERVAL_MS > MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS
#error "TICK_INTERVAL_MS must not be larger than MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS"
#endif
