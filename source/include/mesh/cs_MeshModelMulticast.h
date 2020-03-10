/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>
#include

#define MESH_MODEL_QUEUE_SIZE 20

/**
 * Interval at which processQueue() gets called.
 */
#define MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS 100

/**
 * Number of messages sent each time processQueue() gets called.
 */
#define MESH_MODEL_QUEUE_BURST_COUNT 3

/**
 * Class that:
 * - Sends and receives multicast non-segmented messages.
 * - Queues messages to be sent.
 */
class MeshModelMulticast {
public:
	void init();

	void tick(uint32_t tickCount);
};

