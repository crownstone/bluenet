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
#include "protocol/cs_ServiceDataPackets.h"
#include "protocol/cs_CmdSource.h"


#define LEGACY_MULTI_SWITCH_HEADER_SIZE (1+1)
#define LEGACY_MULTI_SWITCH_MAX_ITEM_COUNT 18

#define SESSION_NONCE_LENGTH 5

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
	BASIC               = 2,
	SETUP               = 100,
	SERVICE_DATA        = 101,
	LOCALIZATION        = 102, // Used for RC5 encryption.
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
 * Switch state: combination of relay and dimmer state.
 * Relay:  0 = off, 1 = on
 * Dimmer: 0 = off, 100 = fully on
 */
union __attribute__((__packed__)) switch_state_t {
	struct __attribute__((packed)) {
		uint8_t dimmer : 7;
		uint8_t relay  : 1;
	} state;
	uint8_t asInt;
};

/**
 * A single multi switch item.
 * switchCmd: 0 = off, 100 = fully on.
 */
struct __attribute__((packed)) multi_switch_item_t {
	stone_id_t id;
	uint8_t switchCmd;
};

/**
 * Multi switch packet.
 */
struct __attribute__((packed)) multi_switch_t {
	uint8_t count;
	multi_switch_item_t items[5];
};

struct __attribute__((__packed__)) cs_legacy_multi_switch_item_t {
	stone_id_t id;
	uint8_t switchCmd;
	uint16_t timeout;
	uint8_t intent;
};

struct __attribute__((__packed__)) cs_legacy_multi_switch_t {
	uint8_t type; // Always 0 (type list).
	uint8_t count;
	cs_legacy_multi_switch_item_t items[LEGACY_MULTI_SWITCH_MAX_ITEM_COUNT];
};

enum KeepAliveActionTypes {
	NO_CHANGE = 0,
	CHANGE    = 1
};

/**
 * A single keep alive command.
 * action: see KeepAliveActionTypes.
 * switchCmd: 0 = off, 100 = fully on.
 * timeout: delay in seconds.
 */
struct __attribute__((__packed__)) keep_alive_state_item_cmd_t {
	uint8_t action;
	uint8_t switchCmd;
	uint16_t timeout;
};


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

struct __attribute__((packed)) session_nonce_t {
	uint8_t data[SESSION_NONCE_LENGTH];
};

// ========================= functions =========================

/**
 * Sets the switch state to the default.
 */
constexpr void cs_switch_state_set_default(switch_state_t *state) {
	state->asInt = STATE_SWITCH_STATE_DEFAULT;
}

/**
 * Sets the state errors to the default.
 */
constexpr void cs_state_errors_set_default(state_errors_t *state) {
	state->asInt = STATE_ERRORS_DEFAULT;
}

/**
 * Returns true when a multi switch item is valid.
 */
inline bool cs_multi_switch_item_is_valid(multi_switch_item_t* item, size16_t size) {
	return (size == sizeof(multi_switch_item_t) && item->id != 0);
}

/**
 * Returns true when a multi switch packet is valid.
 */
inline bool cs_multi_switch_packet_is_valid(multi_switch_t* packet, size16_t size) {
	return (size >= 1 + packet->count * sizeof(multi_switch_item_t));
}

inline bool cs_legacy_multi_switch_item_is_valid(cs_legacy_multi_switch_item_t* item, size16_t size) {
	return (size == sizeof(cs_legacy_multi_switch_item_t) && item->id != 0);
}

inline bool cs_legacy_multi_switch_is_valid(const cs_legacy_multi_switch_t* packet, size16_t size) {
	if (size < LEGACY_MULTI_SWITCH_HEADER_SIZE) {
		return false;
	}
	if (packet->type != 0) {
		return false;
	}
	if (packet->count > LEGACY_MULTI_SWITCH_MAX_ITEM_COUNT) {
		return false;
	}
	if (size < LEGACY_MULTI_SWITCH_HEADER_SIZE + packet->count * sizeof(cs_legacy_multi_switch_item_t)) {
		return false;
	}
	return true;
}