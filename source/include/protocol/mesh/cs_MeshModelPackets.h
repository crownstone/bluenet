/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "protocol/cs_Typedefs.h"
#include "protocol/cs_CmdSource.h"

/**
 * Message opcodes.
 *
 * Must be in the range 0xC0 - 0xFF
 * See access_opcode_t
 */
enum cs_mesh_model_opcode_t {
	CS_MESH_MODEL_OPCODE_MSG = 0xC0,
	CS_MESH_MODEL_OPCODE_RELIABLE_MSG = 0xC1,
	CS_MESH_MODEL_OPCODE_REPLY = 0xC2,
};

/**
 * Max message size.
 * TODO: define a max per type, since the mesh supports variable length messages.
 * When you send packets that are longer than 15 bytes (including opCode of 1-3B, and MIC of 4 or 8B), they will be sent
 * as segmented packets of 12? byte each.
 * See https://devzone.nordicsemi.com/f/nordic-q-a/32854/max-size-of-data-to-send-from-one-node-to-other-ble-mesh
 * Multi switch message with 5 items is 28 + 3 (opCode) + 4 (MIC) = 35, so 3 segments.
 * The minimum advertising interval that mesh are using now is 20ms, so each advertisement / segment, takes 20ms.
 */
#define MAX_MESH_MSG_SIZE (3 * 12 - 3 - 4)
#define MAX_MESH_MSG_NON_SEGMENTED_SIZE (15 - 4 - 3)

/**
 * Size of the header of each mesh model message.
 * 1B for the message type.
 */
#define MESH_HEADER_SIZE 1
enum cs_mesh_model_msg_type_t {
	CS_MESH_MODEL_TYPE_TEST = 0,                 // Payload: cs_mesh_model_msg_test_t
	CS_MESH_MODEL_TYPE_ACK = 1,                  // Payload: none
	CS_MESH_MODEL_TYPE_STATE_TIME = 2,           // Payload: cs_mesh_model_msg_time_t
	CS_MESH_MODEL_TYPE_CMD_TIME = 3,             // Payload: cs_mesh_model_msg_time_t
	CS_MESH_MODEL_TYPE_CMD_NOOP = 4,             // Payload: none
	CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH = 5,     // Payload: multi_switch_item_t
	CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE_STATE = 6, // Payload: keep_alive_state_item_t
	CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE = 7,       // Payload: none
	CS_MESH_MODEL_TYPE_STATE_0 = 8,              // Payload: cs_mesh_model_msg_state_0_t
	CS_MESH_MODEL_TYPE_STATE_1 = 9,              // Payload: cs_mesh_model_msg_state_1_t
	CS_MESH_MODEL_TYPE_PROFILE_LOCATION = 10,    // Payload: cs_mesh_model_msg_profile_location_t
};

struct __attribute__((__packed__)) cs_mesh_model_msg_test_t {
	uint32_t counter;
	uint8_t dummy[3]; // non segmented
//	uint8_t dummy[11]; // 2 segments
//	uint8_t dummy[23]; // 3 segments
};

struct __attribute__((__packed__)) cs_mesh_model_msg_time_t {
	uint32_t timestamp;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_profile_location_t {
	uint8_t profile;
	uint8_t location;
	stone_id_t stone_id;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_state_0_t {
	uint8_t switchState;
	uint8_t flags;
	int8_t powerFactor;
	int16_t powerUsageReal;
	uint16_t partialTimestamp;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_state_1_t {
	int8_t temperature;
	int32_t energyUsed;
	uint16_t partialTimestamp;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_multi_switch_item_t {
	stone_id_t id;
	uint8_t switchCmd;
	uint16_t delay;
	cmd_source_t source;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_keep_alive_item_t {
	stone_id_t id;
	uint8_t actionSwitchCmd;
};
/**
 * Size of the keep alive header.
 * 1B for keep alive type.
 * 2B for the timeout.
 * 1B for the item count.
 */
#define MESH_MESH_KEEP_ALIVE_HEADER_SIZE (1+2+1)
/**
 * Maximum number of keep alive items in the list.
 */
#define MESH_MESH_KEEP_ALIVE_MAX_ITEM_COUNT ((MAX_MESH_MSG_SIZE - MESH_HEADER_SIZE - MESH_MESH_KEEP_ALIVE_HEADER_SIZE) / sizeof(cs_mesh_model_msg_keep_alive_item_t))
struct __attribute__((__packed__)) cs_mesh_model_msg_keep_alive_t {
	uint8_t type; // Always 1 (type keep alive same timeout)
	uint16_t timeout;
	uint8_t count;
	cs_mesh_model_msg_keep_alive_item_t items[MESH_MESH_KEEP_ALIVE_MAX_ITEM_COUNT];
};
