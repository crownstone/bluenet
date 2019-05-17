/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 12, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include "cfg/cs_Config.h"
#include "protocol/cs_CommandTypes.h"
#include "protocol/cs_ScheduleEntries.h"
#include "protocol/cs_Typedefs.h"
#include "protocol/mesh/cs_MeshModelPackets.h"

/**
 * Packets (structs) that are used over the air.
 *
 * These should be plain structs, without constructors, or member functions.
 * Instead of a constructor, you can add a function that fills a struct.
 *
 * If the definition becomes large, move it to its own file and include it in this file.
 */

enum EncryptionAccessLevel {
	ADMIN               = 0,
	MEMBER              = 1,
	GUEST               = 2,
	SETUP               = 100,
	NOT_SET             = 201,
	ENCRYPTION_DISABLED = 254,
	NO_ONE              = 255
};

/**
 * Header of a stream buffer
 */
struct __attribute__((__packed__)) stream_buffer_header_t {
	uint8_t type;
	uint8_t opCode; //! can be used as op code, see <OpCode>
	uint16_t length;
};

/**
 * State errors: collection of errors that influence the switch behaviour.
 */
union __attribute__((__packed__)) state_errors_t {
	struct __attribute__((packed)) {
		bool overCurrent: 1;
		bool overCurrentDimmer: 1;
		bool chipTemp: 1;
		bool dimmerTemp: 1;
		bool dimmerOn: 1;
		bool dimmerOff: 1;
	} errors;
	uint32_t asInt;
};
/**
 * Sets the state errors to the default.
 */
constexpr void cs_state_errors_set_default(state_errors_t *state) {
	state->asInt = STATE_ERRORS_DEFAULT;
}

/**
 * Switch state: combination of relay and dimmer state.
 * Relay:  0 = off, 1 = on
 * Dimmer: 0 = off, 100 = fully on
 */
union __attribute__((__packed__)) __attribute__((__aligned__(4))) switch_state_t {
	struct __attribute__((packed)) {
		uint8_t dimmer : 7;
		uint8_t relay  : 1;
	} state;
	uint8_t asInt;
};
/**
 * Sets the switch state to the default.
 */
constexpr void cs_switch_state_set_default(switch_state_t *state) {
	state->asInt = STATE_SWITCH_STATE_DEFAULT;
}


/**
 * A single multi switch item.
 * switchCmd: 0 = off, 100 = fully on.
 */
struct __attribute__((packed)) multi_switch_item_t {
	stone_id_t id;
	uint8_t switchCmd;
};
/**
 * Returns true when a multi switch item is valid.
 */
inline bool cs_multi_switch_item_is_valid(multi_switch_item_t* item, size16_t size) {
	return (size == sizeof(multi_switch_item_t) && item->id != 0);
}

/**
 * Multi switch packet.
 */
struct __attribute__((packed)) multi_switch_t {
	uint8_t count;
	multi_switch_item_t items[5];
};
/**
 * Returns true when a multi switch packet is valid.
 */
inline bool cs_multi_switch_packet_is_valid(multi_switch_t* packet, size16_t size) {
	return (size == 1 + packet->count * sizeof(multi_switch_item_t));
}



struct __attribute__((__packed__)) switch_message_payload_t {
	uint8_t switchState;
};

struct __attribute__((__packed__)) opcode_message_payload_t {
	uint8_t opCode;
};

struct __attribute__((__packed__)) enable_message_payload_t {
	BOOL enable;
};

struct __attribute__((__packed__)) enable_scanner_message_payload_t {
	bool enable;
	uint16_t delay;
};

struct __attribute__((__packed__)) factory_reset_message_payload_t {
	uint32_t resetCode;
};

struct __attribute__((__packed__)) led_message_payload_t {
	uint8_t led;
	bool enable;
};

struct __attribute__((__packed__)) schedule_command_t {
	uint8_t id;
	schedule_entry_t entry;
};



#define SESSION_NONCE_LENGTH 5
struct __attribute__((packed)) session_nonce_t {
	uint8_t data[SESSION_NONCE_LENGTH];
};
