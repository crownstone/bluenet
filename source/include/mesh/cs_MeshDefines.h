/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 13, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

// Debug logs
#define LOGMeshWarning LOGw
#define LOGMeshInfo LOGvv
#define LOGMeshDebug LOGvv
#define LOGMeshVerbose LOGvv

// Debug logs
#define LOGMeshModelInfo    LOGvv
#define LOGMeshModelDebug   LOGvv
#define LOGMeshModelVerbose LOGvv
#define LogLevelMeshModelVerbose SERIAL_VERY_VERBOSE

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
 * Interval at which acked messages are retried.
 * When using reliable messages, some more gets added for each hop.
 * Should be a multiple of TICK_INTERVAL_MS.
 */
#define MESH_MODEL_ACKED_RETRY_INTERVAL_MS 200

/**
 * Number of times an ack will be sent.
 */
#define MESH_MODEL_ACK_TRANSMISSIONS 1

/**
 * Number of messages sent each time processQueue() gets called.
 */
#define MESH_MODEL_QUEUE_BURST_COUNT 3

/**
 * Timeout in seconds for reliable msgs.
 */
#define MESH_MODEL_RELIABLE_TIMEOUT_DEFAULT 10

/**
 * Default number of transmissions for unreliable msgs.
 */
#define MESH_MODEL_TRANSMISSIONS_DEFAULT 3

/**
 * Max number of transmissions for unreliable msgs.
 */
#define MESH_MODEL_TRANSMISSIONS_MAX 31

/**
 * Group address used for multicast.
 */
#define MESH_MODEL_GROUP_ADDRESS 0xC51E

/**
 * Group address used for acked multicast.
 */
#define MESH_MODEL_GROUP_ADDRESS_ACKED 0xC51F

