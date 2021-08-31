/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 12, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Config.h>
#include <cstdint>
#include <protocol/cs_CmdSource.h>
#include <protocol/cs_CommandTypes.h>
#include <protocol/cs_ErrorCodes.h>
#include <protocol/cs_MicroappPackets.h>
#include <protocol/cs_ServiceDataPackets.h>
#include <protocol/cs_Typedefs.h>
#include <protocol/mesh/cs_MeshModelPackets.h>


#define LEGACY_MULTI_SWITCH_HEADER_SIZE (1+1)
#define LEGACY_MULTI_SWITCH_MAX_ITEM_COUNT 18

#define VALIDATION_KEY_LENGTH   4
#define SESSION_NONCE_LENGTH    5
#define PACKET_NONCE_LENGTH     3

/**
 * Packets (structs) that are used over the air, over uart, or stored in flash.
 *
 * Constructors can be added, as they do not impact the size of the struct.
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
 * Nonce used for encryption.
 */
struct __attribute__((__packed__)) encryption_nonce_t {
	uint8_t packetNonce[PACKET_NONCE_LENGTH];
	uint8_t sessionNonce[SESSION_NONCE_LENGTH];
};

/**
 * Encryption header that's not encrypted.
 */
struct __attribute__((__packed__)) encryption_header_t {
	uint8_t packetNonce[PACKET_NONCE_LENGTH];
	uint8_t accessLevel;
};

/**
 * Encryption header that's encrypted.
 */
struct __attribute__((__packed__)) encryption_header_encrypted_t {
	uint8_t validationKey[VALIDATION_KEY_LENGTH];
};

struct __attribute__((packed)) session_data_t {
	uint8_t protocol;
	uint8_t sessionNonce[SESSION_NONCE_LENGTH];
	uint8_t validationKey[VALIDATION_KEY_LENGTH];
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
	uint8_t protocolVersion;
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
	uint8_t protocolVersion = CS_CONNECTION_PROTOCOL_VERSION;
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


struct __attribute__((__packed__)) setup_data_t {
	stone_id_t     stoneId;
	uint8_t        sphereId;
	uint8_t        adminKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        memberKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        basicKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        serviceDataKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        localizationKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        meshDeviceKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        meshAppKey[ENCRYPTION_KEY_LENGTH];
	uint8_t        meshNetKey[ENCRYPTION_KEY_LENGTH];
	cs_uuid128_t   ibeaconUuid;
	uint16_t       ibeaconMajor;
	uint16_t       ibeaconMinor;
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

/**
 * Flags to determine how to send the mesh message.
 *
 * All possibilities:
 *   1,0,0 Broadcast
 *   1,1,0 Broadcast, ack all provided IDs
 *   1,1,1 Broadcast, ack all known IDs
 *   0,0,0 Send command only to provided IDs
 *   0,1,0 Send command only to provided IDs, with ack
 *   0,0,1 Send command only to known IDs
 *   0,1,1 Send command only to known IDs, with ack
 */
union __attribute__((__packed__)) mesh_control_command_packet_flags_t {
	struct __attribute__((packed)) {
		bool broadcast: 1;
		bool reliable: 1;
		bool useKnownIds: 1;
		bool noHops : 1;
	} flags;
	uint8_t asInt = 1; // Broadcast to all by default.
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
 * Switch command values.
 *
 * We could also write this as struct with 7 bits value,
 * and 1 bit that determines whether the value is switch value (0-100), or a special value (enum).
 */
enum SwitchCommandValue {
	CS_SWITCH_CMD_VAL_OFF = 0,
	// Dimmed from 1 - 99
	CS_SWITCH_CMD_VAL_FULLY_ON = 100,
	CS_SWITCH_CMD_VAL_NONE = 128,      // For printing: the value is set to nothing.
	CS_SWITCH_CMD_VAL_DEBUG_RESET_ALL = 129,
	CS_SWITCH_CMD_VAL_DEBUG_RESET_AGG = 130,
	CS_SWITCH_CMD_VAL_DEBUG_RESET_OVERRIDE = 131,
	CS_SWITCH_CMD_VAL_DEBUG_RESET_AGG_OVERRIDE = 132,

	CS_SWITCH_CMD_VAL_TOGGLE = 253,    // Switch OFF when currently on, switch to SMART_ON when currently off.
	CS_SWITCH_CMD_VAL_BEHAVIOUR = 254, // Switch to the value according to behaviour rules.
	CS_SWITCH_CMD_VAL_SMART_ON = 255   // Switch on, the value will be determined by behaviour rules.
};

/**
 * A single multi switch item.
 *
 * id:        Command is targeted to stone with this ID.
 * switchCmd: SwitchCommandValue
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
	uint32_t sunrise = 8*60*60; // Time in seconds after midnight that the sun rises.
	uint32_t sunset = 19*60*60; // Time in seconds after midnight that the sun sets.
};

/**
 * Packet to change ibeacon config ID.
 *
 * Timestamp: when to set the ID for the first time.
 * Interval: set ID every N seconds after timestamp.
 * Set interval = 0 to execute only once.
 * Set timestamp = 0, and interval = 0 to execute now.
 * A stored entry with timestamp = 0 and interval = 0, will be considered empty.
 */
struct __attribute__((__packed__)) ibeacon_config_id_packet_t {
	uint32_t timestamp = 0;
	uint16_t interval = 0;
};

// See ibeacon_config_id_packet_t
struct __attribute__((__packed__)) set_ibeacon_config_id_packet_t {
	uint8_t ibeaconConfigId = 0;
	ibeacon_config_id_packet_t config;
};

struct __attribute__((__packed__)) led_message_payload_t {
	uint8_t led;
	bool enable;
};

struct __attribute__((__packed__)) hub_data_header_t {
	uint8_t encrypt; // See UartProtocol::Encrypt
	uint8_t reserved; // Reserved for future use (access level?). Must be 0 for now.
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

struct __attribute__((packed)) tracked_device_heartbeat_packet_t {
	uint16_t deviceId;
	uint8_t locationId = 0;
	uint8_t deviceToken[TRACKED_DEVICE_TOKEN_SIZE]; // Should match the token that's registered.
	uint8_t timeToLiveMinutes = 0; // When set, heartbeats will be simulated until timeout.
};

enum class PresenceChange : uint8_t {
	FIRST_SPHERE_ENTER = 0,     // Profile ID and location ID are not set.
	LAST_SPHERE_EXIT = 1,       // Profile ID and location ID are not set.
	PROFILE_SPHERE_ENTER = 2,   // Only profile ID is set.
	PROFILE_SPHERE_EXIT = 3,    // Only profile ID is set.
	PROFILE_LOCATION_ENTER = 4, // Profile ID and location ID are set.
	PROFILE_LOCATION_EXIT = 5,  // Profile ID and location ID are set.
};

struct __attribute((packed)) presence_change_t {
	uint8_t type;
	uint8_t profileId;
	uint8_t locationId;
};

struct __attribute((packed)) presence_t {
	uint64_t presence[8]; // Bitmask of presence per profile.
};



struct __attribute__((__packed__)) mesh_state_part_0_t {
	stone_id_t stoneId;
	cs_mesh_model_msg_state_0_t meshState;
};

struct __attribute__((__packed__)) mesh_state_part_1_t {
	stone_id_t stoneId;
	cs_mesh_model_msg_state_1_t meshState;
};



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

struct __attribute__((packed)) cs_adc_restarts_t {
	uint32_t count = 0; // Number of ADC restarts since boot.
	uint32_t lastTimestamp = 0; // Timestamp of last ADC restart.
};

struct __attribute__((packed)) cs_adc_channel_swaps_t {
	uint32_t count = 0; // Number of detected ADC channel swaps since boot.
	uint32_t lastTimestamp = 0; // Timestamp of last detected ADC channel swap.
};

enum PowerSamplesType {
	POWER_SAMPLES_TYPE_SWITCHCRAFT = 0,
	POWER_SAMPLES_TYPE_SWITCHCRAFT_NON_TRIGGERED = 1,
	POWER_SAMPLES_TYPE_NOW_FILTERED = 2,
	POWER_SAMPLES_TYPE_NOW_UNFILTERED = 3,
	POWER_SAMPLES_TYPE_SOFTFUSE = 4,
	POWER_SAMPLES_TYPE_SWITCH = 5,
};

struct __attribute__((packed)) cs_power_samples_header_t {
	uint8_t type;                 // PowerSamplesType.
	uint8_t index = 0;            // Some types have multiple lists of samples.
	uint16_t count = 0;           // Number of samples.
	uint32_t unixTimestamp;       // Unix timestamp of time the samples have been set.
	uint16_t delayUs;             // Delay of the measurement.
	uint16_t sampleIntervalUs;    // Time between samples.
	uint16_t reserved = 0;
	int16_t offset;               // Calculated offset (mean) of the samples.
	float multiplier;             // Multiply the sample value with this value to get a value in ampere, or volt.
	// Followed by: int16_t samples[count]
};

struct __attribute__((packed)) cs_power_samples_request_t {
	uint8_t type;                 // PowerSamplesType.
	uint8_t index = 0;            // Some types have multiple lists of samples.
};

struct __attribute__((packed)) cs_switch_history_header_t {
	uint8_t count;                // Number of items.
};

struct __attribute__((packed)) cs_switch_history_item_t {
	uint32_t timestamp;           // Timestamp of the switch command.
	uint8_t value;                // Switch command value.
	switch_state_t state;         // Switch state after executing the command.
	cmd_source_t source;          // Source of the command.

	cs_switch_history_item_t(uint32_t timestamp, uint8_t switchValue, switch_state_t switchState, const cmd_source_t& source):
		timestamp(timestamp),
		value(switchValue),
		state(switchState),
		source(source)
	{}
};

struct __attribute__((packed)) cs_gpregret_result_t {
	uint8_t index;                // Which GPREGRET
	uint32_t value;               // The value of this GPREGRET
};

struct __attribute__((packed)) cs_ram_stats_t {
	uint32_t minStackEnd = 0xFFFFFFFF;
	uint32_t maxHeapEnd = 0;
	uint32_t minFree = 0;
	uint32_t numSbrkFails = 0;
};

struct __attribute__((packed)) cs_twi_init_t {
	uint8_t scl;
	uint8_t sda;
	uint8_t freq;
};

struct __attribute__((packed)) cs_twi_write_t {
	uint8_t address;
	uint8_t length;
	uint8_t* buf;
	bool stop;
};

struct __attribute__((packed)) cs_twi_read_t {
	uint8_t address;
	uint8_t length;
	uint8_t* buf;
	bool stop;
};

struct __attribute__((packed)) cs_gpio_init_t {
	uint8_t pin_index;
	uint8_t direction;
	uint8_t pull;
	uint8_t polarity;
	uintptr_t callback;
};

struct __attribute__((packed)) cs_gpio_write_t {
	uint8_t pin_index;
	uint8_t length;
	uint8_t *buf;
};

struct __attribute__((packed)) cs_gpio_read_t {
	uint8_t pin_index;
	uint8_t length;
	uint8_t *buf;
};

struct __attribute__((packed)) cs_gpio_update_t {
	uint8_t pin_index;
	uint8_t length;
	uint8_t *buf;
};

const uint8_t CS_CHARACTERISTIC_NOTIFICATION_PART_LAST = 255;

/**
 * A packet that represents RSSI data about an asset received by a particular Crownstone. There is no timestamp in it.
 * Timestamps can be added on the hub.
 */
struct __attribute__((packed)) cs_asset_rssi_data_mac_t {
	uint8_t address[MAC_ADDRESS_LEN];
	uint8_t stoneId;
	int8_t rssi;
	uint8_t channel;
};

struct __attribute__((packed)) cs_asset_rssi_data_sid_t {
	short_asset_id_t assetId;
	uint8_t stoneId;
	int8_t rssi;
	uint8_t channel;
};

/**
 * A packet that informs about the nearest Crownstone with respect to an asset. The asset is represented by an
 * identifier of three bytes. This allows this message to be sent as unsegmented message through the mesh.
 */
struct __attribute__((packed)) cs_nearest_stone_update_t {
	short_asset_id_t assetId;
	uint8_t stoneId;
	int8_t rssi;
	uint8_t channel;
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
