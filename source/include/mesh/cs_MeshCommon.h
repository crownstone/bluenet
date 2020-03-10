/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "common/cs_Types.h"

/*
 * 0 to disable test.
 * 1 for unacked, unsegmented drop test.
 *   This assumes you have a node with id 2 (sending node).
 * 2 for targeted, segmented acked test.
 *   This assumes you have 2 nodes: one with id 1 (receiving node), and one with id 2 (sending node).
 */
#define MESH_MODEL_TEST_MSG 0

struct __attribute__((__packed__)) cs_mesh_model_queued_item_t {
	bool priority : 1;
	bool reliable : 1;
	uint8_t repeats : 6;
	stone_id_t targetId = 0;   // 0 for broadcast
	uint8_t type;
	uint16_t id; // ID that can freely be used to find similar items.
	uint8_t msgSize;
//	uint8_t msg[MAX_MESH_MSG_NON_SEGMENTED_SIZE];
	uint8_t* msgPtr = NULL;
};


