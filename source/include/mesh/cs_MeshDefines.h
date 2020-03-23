/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 13, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#define LOGMeshInfo LOGi
#define LOGMeshDebug LOGnone

#define LOGMeshModelInfo    LOGi
#define LOGMeshModelDebug   LOGnone
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
