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

enum BackgroundAdvFlagBitPos {
	BG_ADV_FLAG_RESERVED = 0,
	BG_ADV_FLAG_IGNORE_FOR_PRESENCE = 1,
	BG_ADV_FLAG_TAP_TO_TOGGLE_ENABLED = 2
};

/**
 * Header of a control packet.
 */
struct __attribute__((__packed__)) control_packet_header_t {
	cs_control_cmd_t commandType;
	cs_buffer_size_t payloadSize;
};

/**
 * Control packet.
 */
template<cs_buffer_size_t N>
struct __attribute__((__packed__)) control_packet_t {
	control_packet_header_t header;
	uint8_t payload[N];
};

/**
 * Header of a result packet.
 */
struct __attribute__((__packed__)) result_packet_header_t {
	cs_control_cmd_t commandType;
	cs_ret_code_t returnCode;
	cs_buffer_size_t payloadSize;
};

/**
 * Result packet.
 */
template<cs_buffer_size_t N>
struct __attribute__((__packed__)) result_packet_t {
	result_packet_header_t header;
	uint8_t payload[N];
};

/**
 * State get/set header packet.
 */
struct __attribute__((__packed__)) state_packet_header_t {
	uint16_t stateType;
	uint16_t stateId;
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
 * Behaviour settings.
 */
struct __attribute__((packed)) behaviour_settings_t {
	struct __attribute__((packed)) {
		bool enabled;
	} flags;
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

struct __attribute__((packed)) sun_time_t {
	uint32_t sunrise = 8*60*60;
	uint32_t sunset = 19*60*60;
};

struct __attribute__((__packed__)) led_message_payload_t {
	uint8_t led;
	bool enable;
};

struct __attribute__((packed)) session_nonce_t {
	uint8_t data[SESSION_NONCE_LENGTH];
};

struct __attribute__((packed)) behaviour_debug_t {
	uint32_t time;    // 0 if unknown.
	uint32_t sunrise; // 0 if unknown
	uint32_t sunset;  // 0 if unknown
	uint8_t overrideState;   // From switch aggregator, 255 when not set.
	uint8_t behaviourState;  // From switch aggregator, 255 when not set.
	uint8_t aggregatedState; // From switch aggregator, 255 when not set.
	uint8_t dimmerPowered; // 1 when powered.
	uint8_t behaviourEnabled; // 1 when enabled.
	uint64_t storedBehaviours; // Bitmask of behaviour indices that are stored.
	uint64_t activeBehaviours; // Bitmask of behaviour indices that are active.
	uint64_t extensionActive;  // Bitmask of behaviours with active end condition.
	uint64_t activeTimeoutPeriod; // Bitmask of behaviours that are in (presence) timeout period.
	uint64_t presence[8]; // Bitmask of presence per profile.
};

struct __attribute__((packed)) register_tracked_device_packet_t {
	uint16_t deviceId;
	uint8_t locationId;
	uint8_t profileId;
	uint8_t rssiOffset;
	uint8_t flags;
	uint8_t deviceToken[3];
	uint16_t timeToLiveMinutes;
};

typedef register_tracked_device_packet_t update_tracked_device_packet_t;


// ========================= functions =========================

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
