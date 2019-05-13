/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "protocol/cs_Typedefs.h"

/**
 * Message opcodes.
 *
 * Must be in the range 0xC0 - 0xFF
 * See access_opcode_t
 */
enum cs_mesh_model_opcode_t {
	CS_MESH_MODEL_OPCODE_MSG = 0xC0,
};

/**
 * Max message size.
 * TODO: define a max per type, since the mesh supports variable length messages.
 * When you send packets that are longer than 15? bytes (including opCode of 1-3B, and MIC of 4 or 8B), they will be sent
 * as segmented packets of 12? byte each.
 * See https://devzone.nordicsemi.com/f/nordic-q-a/32854/max-size-of-data-to-send-from-one-node-to-other-ble-mesh
 * Multi switch message with 5 items is 28 + 3 (opCode) + 4 (MIC) = 35, so 3 segments.
 * The minimum advertising interval that mesh are using now is 20ms, so each advertisement / segment, takes 20ms.
 */
#define MAX_MESH_MSG_SIZE (3 * 12 - 3 - 4)

/**
 * Size of the header of each mesh model message.
 * 1B for the message type.
 */
#define MESH_HEADER_SIZE 1
enum cs_mesh_model_msg_type_t {
	CS_MESH_MODEL_TYPE_STATE_TIME = 0,        // Payload: cs_mesh_model_msg_time_t
	CS_MESH_MODEL_TYPE_CMD_TIME = 1,          // Payload: cs_mesh_model_msg_time_t
	CS_MESH_MODEL_TYPE_CMD_NOOP = 2,          // Payload: none
	CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH = 3,  // Payload: cs_mesh_model_msg_multi_switch_t
	CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE_STATE = 4, // Payload: cs_mesh_model_msg_keep_alive_t
	CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE = 5,    // Payload: none
};




struct __attribute__((__packed__)) cs_mesh_model_msg_time_t {
	uint32_t timestamp;
};




struct __attribute__((__packed__)) cs_mesh_model_msg_multi_switch_item_t {
	stone_id_t id;
	uint8_t switchCmd;
	uint16_t timeout;
	uint8_t intent;
};
/**
 * Size of the multi switch header.
 * 1B for multi switch type.
 * 1B for the item count.
 */
#define MESH_MESH_MULTI_SWITCH_HEADER_SIZE (1+1)
/**
 * Maximum number of multi switch items in the list.
 */
#define MESH_MESH_MULTI_SWITCH_MAX_ITEM_COUNT ((MAX_MESH_MSG_SIZE - MESH_HEADER_SIZE - MESH_MESH_MULTI_SWITCH_HEADER_SIZE) / sizeof(cs_mesh_model_msg_multi_switch_item_t))
struct __attribute__((__packed__)) cs_mesh_model_msg_multi_switch_t {
	uint8_t type; // Always 0 (type list).
	uint8_t count;
	cs_mesh_model_msg_multi_switch_item_t items[MESH_MESH_MULTI_SWITCH_MAX_ITEM_COUNT];
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
