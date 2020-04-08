/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 12, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include "cfg/cs_Config.h"
#include "protocol/cs_CmdSource.h"
#include "protocol/cs_CommandTypes.h"
#include "protocol/cs_ErrorCodes.h"
#include "protocol/cs_ServiceDataPackets.h"
#include "protocol/cs_Typedefs.h"
#include "protocol/mesh/cs_MeshModelPackets.h"


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
	cs_control_cmd_t commandType = CTRL_CMD_UNKNOWN;
	cs_ret_code_t returnCode = ERR_UNSPECIFIED;
	cs_buffer_size_t payloadSize = 0;
	result_packet_header_t() {}
	result_packet_header_t(cs_control_cmd_t commandType, cs_ret_code_t returnCode):
		commandType(commandType),
		returnCode(returnCode),
		payloadSize(0)
	{}
	result_packet_header_t(cs_control_cmd_t commandType, cs_ret_code_t returnCode, cs_buffer_size_t payloadSize):
		commandType(commandType),
		returnCode(returnCode),
		payloadSize(payloadSize)
	{}
};

/**
 * Result packet.
 */
template<cs_buffer_size_t N>
struct __attribute__((__packed__)) result_packet_t {
	result_packet_header_t header;
	uint8_t payload[N];
};

enum class PersistenceModeGet {
	CURRENT = 0,
	STORED = 1,
	FIRMWARE_DEFAULT = 2,
	UNKNOWN = 255
};

enum class PersistenceModeSet {
	TEMPORARY = 0,
	STORED = 1,
	UNKNOWN = 255
};

/**
 * State get/set header packet.
 */
struct __attribute__((__packed__)) state_packet_header_t {
	uint16_t stateType;
	uint16_t stateId;
	uint8_t persistenceMode; // PersistenceModeSet or PersistenceModeGet
	uint8_t reserved = 0;
};

union __attribute__((__packed__)) mesh_control_command_packet_flags_t {
	struct __attribute__((packed)) {
		bool broadcast: 1;
		bool reliable: 1;
		bool useKnownIds: 1;
	} flags;
	uint8_t asInt;
};

/**
 * Mesh control command header packet.
 */
struct __attribute__((__packed__)) mesh_control_command_packet_header_t {
	uint8_t type;
	mesh_control_command_packet_flags_t flags;
	uint8_t timeoutOrTransmissions; // 0 for default.
	uint8_t idCount; // 0 for broadcast.
	// List of ids.
	// Control packet.
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
union __attribute__((packed)) behaviour_settings_t {
	struct __attribute__((packed)) {
		bool enabled : 1;
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

/**
 * Packet to change ibeacon config ID.
 *
 * Timestamp: when to start executing the change.
 * Interval: change every N seconds after timestamp.
 * Set interval = 0 to execute only once.
 * Set timestamp = 0, and interval = 0 to execute now.
 */
struct __attribute__((__packed__)) ibeacon_config_id_packet_t {
	uint8_t ibeaconConfigId = 0;
	uint32_t timestamp = 0;
	uint16_t interval = 0;
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
	uint8_t locationId = 0;
	uint8_t profileId;
	int8_t rssiOffset = 0;
	union __attribute__((packed)) {
		struct __attribute__((packed)) {
			bool reserved : 1;
			bool ignoreForBehaviour : 1;
			bool tapToToggle : 1;
		} flags;
		uint8_t asInt;
	} flags;
//	uint8_t flags;
	uint8_t deviceToken[TRACKED_DEVICE_TOKEN_SIZE];
	uint16_t timeToLiveMinutes = 0;
};

typedef register_tracked_device_packet_t update_tracked_device_packet_t;

struct cs_mesh_iv_index_t {
	// Same as net_flash_data_iv_index_t
    uint32_t iv_index;
    uint8_t iv_update_in_progress;
};

typedef uint32_t cs_mesh_seq_number_t;

struct __attribute__((packed)) cs_uicr_data_t {
	uint32_t board;

	union __attribute__((packed)) {
		struct __attribute__((packed)) {
			uint8_t productType;
			uint8_t region;
			uint8_t productFamily;
		} fields;
		uint32_t asInt;
	} productRegionFamily;

	union __attribute__((packed)) {
		struct __attribute__((packed)) {
			uint8_t patch;
			uint8_t minor;
			uint8_t major;
		} fields;
		uint32_t asInt;
	} majorMinorPatch;

	union __attribute__((packed)) {
		struct __attribute__((packed)) {
			uint8_t housing;
			uint8_t week; // week number
			uint8_t year; // last 2 digits of the year
		} fields;
		uint32_t asInt;
	} productionDateHousing;
};


struct __attribute__((packed)) cs_microapp_t {
	uint32_t start_addr;
	uint16_t size;
	uint16_t checksum;
	uint8_t validation;
	uint8_t id;
};

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
