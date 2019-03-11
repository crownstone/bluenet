/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_NordicMesh.h>

#include <structs/cs_ScanResult.h>
#include <structs/cs_StreamBuffer.h>
#include <protocol/mesh/cs_MeshMessageCommon.h>
#include <protocol/mesh/cs_MeshMessageState.h>

enum MeshChannels {
	COMMAND_REPLY_CHANNEL    = 5,
	SCAN_RESULT_CHANNEL      = 6,
	BIG_DATA_CHANNEL         = 7,
	STATE_CHANNEL_0          = 9,
	STATE_CHANNEL_1          = 10,
	KEEP_ALIVE_CHANNEL       = 11,
	MULTI_SWITCH_CHANNEL     = 12,
	COMMAND_CHANNEL          = 13,
	INVALID_HANDLE           = 0xFFFF
};

#define MESH_HANDLE_COUNT 8

#define MESH_STATE_HANDLE_COUNT 2 //! Number of handles used to relay state

enum MeshCommandTypes {
	CONTROL_MESSAGE          = 0,
	BEACON_MESSAGE           = 1,
	CONFIG_MESSAGE           = 2,
	STATE_MESSAGE            = 3,
};

enum MeshCommandBitmask {
	REPLY_REQUEST            = 0,
};

enum MeshReplyTypes {
	STATUS_REPLY             = 0,
	CONFIG_REPLY             = 1,
	STATE_REPLY              = 2,
};

//! available number of bytes for a mesh message
//#define MAX_MESH_MESSAGE_LEN RBC_MESH_VALUE_MAX_LEN
/** number of bytes used for our mesh message header
 *   - target MAC address: 6B
 *   - message type: 2B
 */
//#define MAX_MESH_MESSAGE_HEADER_LENGTH (BLE_GAP_ADDR_LEN + sizeof(uint16_t))
////! available number of bytes for the mesh message payload. the payload depends on the message type
//#define MAX_MESH_MESSAGE_PAYLOAD_LENGTH (MAX_MESH_MESSAGE_LEN - MAX_MESH_MESSAGE_HEADER_LENGTH)
////! available number of bytes for the data of the message, for command and config messages
//#define MAX_MESH_MESSAGE_DATA_LENGTH (MAX_MESH_MESSAGE_PAYLOAD_LENGTH - SB_HEADER_SIZE)

/********************************************************************
 *
 ********************************************************************/

struct __attribute__((__packed__)) encrypted_mesh_message_t {
	//! Counter, used as message id, nonce, decryption validation, and conflict resolving
	uint32_t messageCounter;
	//! Random number used for nonce
	uint32_t random;
	uint8_t encrypted_payload[MAX_ENCRYPTED_PAYLOAD_LENGTH];
};

//! This struct will be encrypted, so the size has to be a multiple of 16
struct __attribute__((__packed__)) mesh_message_t {
	/**
	 * Counter, must be the same number as the one in the encrypted_mesh_message_t
	 * Counter should never be 0
	 */
	uint32_t messageCounter;
	uint8_t payload[MAX_MESH_MESSAGE_LENGTH];
};

/********************************************************************
 * KEEP ALIVE
 ********************************************************************/

enum KeepAliveType {
	SAME_TIMEOUT = 1,
};

struct __attribute__((__packed__)) keep_alive_cmd_t {
	uint8_t actionSwitchState;
	uint16_t timeout; // timeout in seconds
};

struct __attribute__((__packed__)) keep_alive_same_timeout_item_t {
	stone_id_t id;
	uint8_t actionSwitchState;
};

#define KEEP_ALIVE_HEADER_SIZE     (sizeof(uint8_t))
#define KEEPALIVE_PAYLOAD_SIZE     (MAX_MESH_MESSAGE_LENGTH - KEEP_ALIVE_HEADER_SIZE)

#define KEEP_ALIVE_SAME_TIMEOUT_HEADER_SIZE (sizeof(uint16_t) + sizeof(uint8_t))
#define KEEP_ALIVE_SAME_TIMEOUT_MAX_ITEMS   ((KEEPALIVE_PAYLOAD_SIZE - KEEP_ALIVE_SAME_TIMEOUT_HEADER_SIZE) / sizeof(keep_alive_same_timeout_item_t))

struct __attribute__((__packed__)) keep_alive_same_timeout_t {
	uint16_t timeout;
	uint8_t count;
	keep_alive_same_timeout_item_t list[KEEP_ALIVE_SAME_TIMEOUT_MAX_ITEMS];
};

struct __attribute__((__packed__)) keep_alive_message_t {
	uint8_t type;
	union {
		keep_alive_same_timeout_t sameTimeout;
		// More later
	};
};

inline bool is_valid_keep_alive_msg(keep_alive_message_t* msg, uint16_t length) {
	// First check if the header fits in the message
	if (length < KEEP_ALIVE_HEADER_SIZE) {
		return false;
	}
	switch (msg->type) {
	case SAME_TIMEOUT: {
		if (length < KEEP_ALIVE_HEADER_SIZE + KEEP_ALIVE_SAME_TIMEOUT_HEADER_SIZE) {
			return false;
		}
		// Can't set a timer of 0s.
		// TODO: don't use a timer instead
		if (msg->sameTimeout.timeout == 0) {
			return false;
		}
		if (length < KEEP_ALIVE_HEADER_SIZE + KEEP_ALIVE_SAME_TIMEOUT_HEADER_SIZE + msg->sameTimeout.count * sizeof(keep_alive_same_timeout_item_t)) {
			return false;
		}
		break;
	}
	default:
		return false;
	}

	// Check if the message is not too large
//	if (length > sizeof(keep_alive_message_t)) { this doesn't work, due to unused bytes
	if (length > MAX_MESH_MESSAGE_LENGTH) {
		return false;
	}
	return true;
}

/**
 * Returns true when given id is in the message
 * Sets cmd to the command for given id.
 */
inline bool has_keep_alive_item(keep_alive_message_t* message, stone_id_t id, keep_alive_cmd_t& cmd) {
	switch (message->type) {
	case SAME_TIMEOUT:{
		for (int i = 0; i < message->sameTimeout.count; ++i) {
			if (message->sameTimeout.list[i].id == id) {
				cmd.actionSwitchState = message->sameTimeout.list[i].actionSwitchState;
				cmd.timeout = message->sameTimeout.timeout;
				return true;
			}
		}
		break;
	}
	}
	return false;
}

/********************************************************************
 * KEEP ALIVE
 ********************************************************************/

enum MultiSwitchType {
	LIST = 0,     // multi_switch_list_t
};

enum MultiSwitchIntent {
	SPHERE_ENTER = 0,
	SPHERE_EXIT  = 1,
	ENTER        = 2,
	EXIT         = 3,
	MANUAL       = 4
};


struct __attribute__((__packed__)) multi_switch_cmd_t {
	uint8_t switchState;
	uint16_t timeout;
	uint8_t intent; // See MultiSwitchIntent
};

struct __attribute__((__packed__)) multi_switch_list_item_t {
	stone_id_t id;
	multi_switch_cmd_t cmd;
};

#define MULTI_SWITCH_HEADER_SIZE       (sizeof(uint8_t))
#define MULTI_SWITCH_MAX_PAYLOAD_SIZE  (MAX_MESH_MESSAGE_LENGTH - MULTI_SWITCH_HEADER_SIZE)

#define MULTI_SWITCH_LIST_HEADER_SIZE  (sizeof(uint8_t))
#define MULTI_SWITCH_LIST_MAX_ITEMS    ((MULTI_SWITCH_MAX_PAYLOAD_SIZE - MULTI_SWITCH_LIST_HEADER_SIZE) / sizeof(multi_switch_list_item_t))


struct __attribute__((__packed__)) multi_switch_list_t {
	uint8_t count;
	multi_switch_list_item_t list[MULTI_SWITCH_LIST_MAX_ITEMS];
};

struct __attribute__((__packed__)) multi_switch_message_t {
	uint8_t type;
	union {
		multi_switch_list_t list;
		// More later..
	};
};

inline bool is_valid_multi_switch_message(multi_switch_message_t* msg, uint16_t length) {
	// First check if the header fits in the message
	if (length < MULTI_SWITCH_HEADER_SIZE) {
		return false;
	}

	switch (msg->type) {
	case LIST: {
		if (length < MULTI_SWITCH_HEADER_SIZE + MULTI_SWITCH_LIST_HEADER_SIZE) {
			return false;
		}
		if (msg->list.count > MULTI_SWITCH_LIST_MAX_ITEMS) {
			return false;
		}
		if (length < MULTI_SWITCH_HEADER_SIZE + MULTI_SWITCH_LIST_HEADER_SIZE + msg->list.count * sizeof(multi_switch_list_item_t)) {
			return false;
		}
		break;
	}
	default: {
		return false;
	}
	}

	// Check if the message is not too large
//	if (length > sizeof(multi_switch_message_t)) { this doesn't work, due to unused bytes
	if (length > MAX_MESH_MESSAGE_LENGTH) {
		return false;
	}
	return true;
}

/**
 * Returns true when given id is in the message
 * Sets cmd to the command for given id.
 */
inline bool has_multi_switch_item(multi_switch_message_t* message, stone_id_t id, multi_switch_cmd_t& cmd) {
	switch(message->type) {
	case LIST:{
		for (int i = 0; i < message->list.count; ++i) {
			if (message->list.list[i].id == id) {
				cmd.switchState = message->list.list[i].cmd.switchState;
				cmd.timeout = message->list.list[i].cmd.timeout;
				cmd.intent = message->list.list[i].cmd.intent;
				return true;
			}
		}
		break;
	}
	}
	return false;
}

/********************************************************************
 * STATE
 ********************************************************************/

/*
 * HELPER FUNCTIONS
 */

inline void clear_state_msg(state_message_t* message) {
	memset(message, 0, sizeof(state_message_t));
}

inline bool is_valid_state_msg(state_message_t* message) {
	if (message->size > MAX_STATE_ITEMS || message->head > MAX_STATE_ITEMS || message->tail > MAX_STATE_ITEMS) {
		return false;
	}
	if (message->tail == message->head) {
		return (message->size == 0 || message->size == MAX_STATE_ITEMS);
	}
	if ((message->tail + MAX_STATE_ITEMS - message->head) % MAX_STATE_ITEMS != message->size) {
		return false;
	}
	return true;
}

inline bool is_valid_state_msg(state_message_t* msg, uint16_t length) {
//	// First check if the header fits in the message
//	if (length < STATE_HEADER_SIZE) {
//		return false;
//	}
//	// Then check the header
//	if (length < STATE_HEADER_SIZE + msg->size * sizeof(state_item_t)) {
//		return false;
//	}
//	// Check if the message is not too large
//	if (length > sizeof(state_message_t)) {
//		return false;
//	}

	// Since this message can't be send via characteristic, the size should always be >= the message size
	if (length < sizeof(state_message_t)) {
		return false;
	}
	return is_valid_state_msg(msg);
}

inline void push_state_item(state_message_t* message, state_item_t* item) {
	if (++message->size > MAX_STATE_ITEMS) {
		message->head = (message->head + 1) % MAX_STATE_ITEMS;
		--message->size;
	}
	memcpy(&message->list[message->tail], item, sizeof(state_item_t));
	message->tail = (message->tail + 1) % MAX_STATE_ITEMS;
}

/* Use this function to loop over items from oldest to newest. Start with index -1, then keep calling this function.
 * Example:
 *     int16_t idx = -1;
 *	   state_item_t* p_stateItem;
 *	   while (peek_next_state_item(msg, &p_stateItem, idx))
 */
inline bool peek_next_state_item(state_message_t* message, state_item_t** item, int16_t& index) {
	if (message->size == 0) {
		return false;
	}
	if (index == -1) {
		index = message->head;
	} else {
		index = (index + 1) % MAX_STATE_ITEMS;
		if (index == message->tail) return false;
	}
	*item = &message->list[index];
	return true;
}

/* Use this function to loop over items from newest to oldest. Start with index -1, then keep calling this function.
 * Example:
 *     int16_t idx = -1;
 *	   state_item_t* p_stateItem;
 *	   while (peek_prev_state_item(msg, &p_stateItem, idx))
 */
inline bool peek_prev_state_item(state_message_t* message, state_item_t** item, int16_t& index) {
	if (message->size == 0) {
		return false;
	}
	if (index == -1) {
		index = (message->head + message->size - 1) % MAX_STATE_ITEMS;
		*item = &message->list[index];
		return true;
	} else {
		index = (index - 1 + MAX_STATE_ITEMS) % MAX_STATE_ITEMS;
	}
	if (index == message->head) {
		return false;
	}
	*item = &message->list[index];
	return true;
}

inline bool pop_state_item(state_message_t* message, state_item_t* item) {
	if (message->size > 0) {
		memcpy(item, &message->list[message->head], sizeof(state_item_t));
		message->head = (message->head + 1) % MAX_STATE_ITEMS;
		--message->size;
		return true;
	} else {
		return false;
	}
}

/********************************************************************
 * COMMAND
 ********************************************************************/

//! Minimum header size, excludes id list.
#define MIN_COMMAND_HEADER_SIZE (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t))

//! Maximum size of id list + command payload
#define MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH (MAX_MESH_MESSAGE_LENGTH - MIN_COMMAND_HEADER_SIZE)

using control_mesh_message_t = stream_buffer_t<uint8_t, (MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH - SB_HEADER_SIZE)>;
using config_mesh_message_t =  stream_buffer_t<uint8_t, (MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH - SB_HEADER_SIZE)>;
using state_mesh_message_t =   stream_buffer_t<uint8_t, (MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH - SB_HEADER_SIZE)>;

/** Beacon mesh message
 */
struct __attribute__((__packed__)) beacon_mesh_message_t {
	uint16_t major;
	uint16_t minor;
	ble_uuid128_t uuid;
	int8_t txPower;
};

/** A command message that can be sent over the mesh.
 *
 * A command message exists out of a message type, the number of node identifiers, and data. The data can be in
 * structured or raw form (size MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH). The structure are the node identifiers themselves
 * and the payload stemming from these nodes. The payload is a message: a beacon message, command message, or
 * configuration message.
 *
 * TODO: The construction of this struct makes use of nested anonymous unions. It surprisingly compiles with g++, but
 * will probably fail with other compilers. Both ids[] and payload[] are dynamic (so the size of the struct is hard to
 * extract). Moreover, the size of beacon_mesh_message_t does not seem to be forced to be the same as that of
 * control_mesh_message_t.
 */
struct __attribute__((__packed__)) command_message_t {
	uint8_t messageType;
	uint8_t bitmask;
	uint8_t idCount;
	union {
		struct {
			stone_id_t ids[0];
			union {
				uint8_t payload[0];
				beacon_mesh_message_t beaconMsg;
				control_mesh_message_t commandMsg;
				config_mesh_message_t configMsg;
			};
		} data;
		uint8_t raw[MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH]; // dummy item, makes the command_message_t a fixed size (for allocation)
	};
};

//! Only checks if the ids array fits in the message, not if the payload fits
inline bool is_valid_command_message(command_message_t* msg, uint16_t length) {
	// First check if the header fits in the message
	if (length < MIN_COMMAND_HEADER_SIZE) {
		return false;
	}
	// Then check the header
	if (length < MIN_COMMAND_HEADER_SIZE + msg->idCount * sizeof(stone_id_t)) {
		return false;
	}
	// Check if the message is not too large
//	if (length > sizeof(command_message_t)) { this doesn't work, due to unused bytes
	if (length > MAX_MESH_MESSAGE_LENGTH) {
		return false;
	}
	return true;
}

inline bool is_valid_command_control_mesh_message(control_mesh_message_t* msg, uint16_t length) {
	if (length < SB_HEADER_SIZE) {
		return false;
	}
	if (length < SB_HEADER_SIZE + msg->header.length) {
		return false;
	}
	return true;
}

inline bool is_broadcast_command(command_message_t* message) {
	return message->idCount == 0;
}

inline bool is_command_for_us(command_message_t* message, stone_id_t id) {

	if (is_broadcast_command(message)) {
		return true;
	}

	stone_id_t* p_id;
	p_id = message->data.ids;
	for (int i = 0; i < message->idCount; ++i) {
		if (*p_id == id) {
			return true;
		}
		++p_id;
	}

	// Can't do this, because in the struct it says: ids[0]
//	for (int i = 0; i < message->idCount; ++i) {
//		if (message->data.ids[i] == id) {
//			return true;
//		}
//	}

	return false;
}

/**
 * Gets the payload data.
 * Sets payloadLength to the length of the payload, based on the command message size.
 * Assumes already checked if is_valid_command_message()!
 */
inline void get_command_msg_payload(command_message_t* message, uint16_t messageLength, uint8_t** payload, uint16_t& payloadLength) {
	payloadLength =  messageLength - MIN_COMMAND_HEADER_SIZE - message->idCount * sizeof(stone_id_t);
	*payload = (uint8_t*)message + MIN_COMMAND_HEADER_SIZE + message->idCount * sizeof(stone_id_t);
}

/********************************************************************
 * REPLY
 ********************************************************************/

#define REPLY_HEADER_SIZE (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t))

#define MAX_REPLY_LIST_SIZE (MAX_MESH_MESSAGE_LENGTH - REPLY_HEADER_SIZE)

/** Reply to a status message.
 */
struct __attribute__((__packed__)) status_reply_item_t {
	stone_id_t id;
	uint16_t status;
};

#define MAX_STATUS_REPLY_ITEMS (MAX_REPLY_LIST_SIZE / sizeof(status_reply_item_t))


#define MAX_CONFIG_REPLY_DATA_LENGTH (MAX_REPLY_LIST_SIZE - sizeof(stone_id_t) - SB_HEADER_SIZE)
#define MAX_CONFIG_REPLY_ITEMS 1 // the safe is to request config one by one, but depending on the length of the config

/** Reply to a configuration message.
 */
struct __attribute__((__packed__)) config_reply_item_t {
	stone_id_t id;
	stream_buffer_t<uint8_t, MAX_CONFIG_REPLY_DATA_LENGTH> data;
};

#define MAX_STATE_REPLY_DATA_LENGTH (MAX_REPLY_LIST_SIZE - sizeof(stone_id_t) - SB_HEADER_SIZE)
#define MAX_STATE_REPLY_ITEMS 1 // the safe is to request state one by one, but depending on the length of the state
                                // data that is requested, several states might fit in one mesh message

/** Reply to a state message.
 */
struct __attribute__((__packed__)) state_reply_item_t {
	stone_id_t id;
	stream_buffer_t<uint8_t, MAX_STATE_REPLY_DATA_LENGTH> data;
};

/** Reply for any type of message.
 */
struct __attribute__((__packed__)) reply_message_t {
	uint8_t messageType;
	uint8_t reserved;
	uint32_t messageCounter;
	uint8_t itemCount;
	union {
//		reply_item_t list[];
		status_reply_item_t statusList[MAX_STATUS_REPLY_ITEMS];
		config_reply_item_t configList[MAX_CONFIG_REPLY_ITEMS];
		state_reply_item_t stateList[MAX_STATE_REPLY_ITEMS];
		uint8_t rawList[MAX_REPLY_LIST_SIZE]; // dummy item, makes the reply_message_t a fixed size (for allocation)
	};
};

inline void cear_reply_msg(reply_message_t* msg) {
	memset(msg, 0, sizeof(reply_message_t));
}

inline bool is_valid_reply_msg(reply_message_t* msg) {
	switch (msg->messageType) {
	case STATUS_REPLY:
		return (msg->itemCount <= MAX_STATUS_REPLY_ITEMS);
	case CONFIG_REPLY:
		return (msg->itemCount <= MAX_CONFIG_REPLY_ITEMS);
	case STATE_REPLY:
		return (msg->itemCount <= MAX_STATE_REPLY_ITEMS);
	default:
		return false;
	}
	return true;
}

inline bool is_valid_reply_msg(reply_message_t* msg, uint16_t length) {
	// First check if the header fits in the message
	if (length < REPLY_HEADER_SIZE) {
		return false;
	}
	// Check if the message is not too large
//	if (length > sizeof(reply_message_t)) { this doesn't work, due to unused bytes
	if (length > MAX_MESH_MESSAGE_LENGTH) {
		return false;
	}
	// Check array size
	if (!is_valid_reply_msg(msg)) {
		return false;
	}

	switch (msg->messageType) {
	case STATUS_REPLY:
		if (length < REPLY_HEADER_SIZE + msg->itemCount * sizeof(status_reply_item_t)) {
			return false;
		}
		break;
	case CONFIG_REPLY:
		if (length < REPLY_HEADER_SIZE + msg->itemCount * sizeof(config_reply_item_t)) {
			return false;
		}
		break;
	case STATE_REPLY:
		if (length < REPLY_HEADER_SIZE + msg->itemCount * sizeof(state_reply_item_t)) {
			return false;
		}
		break;
	default:
		return false;
	}
	return true;
}

inline bool push_status_reply_item(reply_message_t* message, status_reply_item_t* item) {
	if (message->itemCount == MAX_STATUS_REPLY_ITEMS) {
		return false;
	} else {
		memcpy(&message->statusList[message->itemCount++], item, sizeof(status_reply_item_t));
		return true;
	}
}

/********************************************************************
 * SCAN RESULT
 ********************************************************************/

struct __attribute__((__packed__)) scan_result_item_t {
	stone_id_t id;
	uint8_t address[BLE_GAP_ADDR_LEN];
	int8_t rssi;
};

#define SCAN_RESULT_HEADER_SIZE (sizeof(uint8_t) + sizeof(uint8_t))
#define MAX_SCAN_RESULT_ITEMS ((MAX_MESH_MESSAGE_LENGTH - SCAN_RESULT_HEADER_SIZE) / sizeof(scan_result_item_t))

struct __attribute__((__packed__)) scan_result_message_t {
	uint8_t numOfResults;
	uint8_t reserved[3];
	scan_result_item_t list[MAX_SCAN_RESULT_ITEMS];
};

inline bool is_valid_scan_result_message(scan_result_message_t* msg, uint16_t length) {
	// First check if the header fits in the message
	if (length < SCAN_RESULT_HEADER_SIZE) {
		return false;
	}
	// Then check the header
	if (msg->numOfResults > MAX_SCAN_RESULT_ITEMS) {
		return false;
	}
	if (length < SCAN_RESULT_HEADER_SIZE + msg->numOfResults * sizeof(scan_result_item_t)) {
		return false;
	}
	// Check if the message is not too large
//	if (length > sizeof(scan_result_message_t)) { this doesn't work, due to unused bytes
	if (length > MAX_MESH_MESSAGE_LENGTH) {
		return false;
	}
	return true;
}

