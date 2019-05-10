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
 * When you send packets that are longer than 11 bytes, they will be sent as segmented packets.
 * The minimum advertising interval that mesh are using now is 20ms, so you can't send any faster than 20ms.
 * About 3 parts, fits a multi switch message with 5 items.
 */
#define MAX_MESH_MSG_SIZE 30

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
	CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE = 4,    // Payload: cs_mesh_model_msg_keep_alive_t
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
