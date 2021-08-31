/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <mesh/cs_MeshDefines.h>
#include <protocol/cs_Typedefs.h>
#include <protocol/cs_CmdSource.h>
#include <protocol/cs_AssetFilterPackets.h>

/**
 * Message opcodes.
 *
 * Must be in the range 0xC0 - 0xFF
 * See access_opcode_t
 */
enum cs_mesh_model_opcode_t {
	CS_MESH_MODEL_OPCODE_MSG = 0xC0,
	CS_MESH_MODEL_OPCODE_UNICAST_RELIABLE_MSG = 0xC1,
	CS_MESH_MODEL_OPCODE_UNICAST_REPLY = 0xC2,
	CS_MESH_MODEL_OPCODE_MULTICAST_RELIABLE_MSG = 0xC3,
	CS_MESH_MODEL_OPCODE_MULTICAST_REPLY = 0xC4,
	CS_MESH_MODEL_OPCODE_MULTICAST_NEIGHBOURS = 0xC5,
};

/**
 * Max message size.
 * When you send packets that are longer than 15 bytes (including opCode of 1-3B, and MIC of 4 or 8B), they will be sent
 * as segmented packets of 12 byte each.
 * See https://devzone.nordicsemi.com/f/nordic-q-a/32854/max-size-of-data-to-send-from-one-node-to-other-ble-mesh
 * The minimum advertising interval that mesh are using now is 20ms, so each advertisement / segment, takes 20ms.
 */
#define MAX_MESH_MSG_SIZE (3 * 12 - 4 - 3)
#define MAX_MESH_MSG_NON_SEGMENTED_SIZE (15 - 4 - 3)

/**
 * Size of the header of each mesh model message.
 * 1B for the message type.
 */
#define MESH_HEADER_SIZE 1

enum cs_mesh_model_msg_type_t {
	CS_MESH_MODEL_TYPE_TEST                      = 0,  // Payload: cs_mesh_model_msg_test_t
	CS_MESH_MODEL_TYPE_ACK                       = 1,  // Payload: none
	CS_MESH_MODEL_TYPE_CMD_TIME                  = 3,  // Payload: cs_mesh_model_msg_time_t
	CS_MESH_MODEL_TYPE_CMD_NOOP                  = 4,  // Payload: none
	CS_MESH_MODEL_TYPE_CMD_MULTI_SWITCH          = 5,  // Payload: cs_mesh_model_msg_multi_switch_item_t
//	CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE_STATE      = 6,  // Payload: keep_alive_state_item_t
//	CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE            = 7,  // Payload: none
	CS_MESH_MODEL_TYPE_STATE_0                   = 8,  // Payload: cs_mesh_model_msg_state_0_t
	CS_MESH_MODEL_TYPE_STATE_1                   = 9,  // Payload: cs_mesh_model_msg_state_1_t
	CS_MESH_MODEL_TYPE_PROFILE_LOCATION          = 10, // Payload: cs_mesh_model_msg_profile_location_t
	CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS    = 11, // Payload: behaviour_settings_t
	CS_MESH_MODEL_TYPE_TRACKED_DEVICE_REGISTER   = 12, // Payload: cs_mesh_model_msg_device_register_t
	CS_MESH_MODEL_TYPE_TRACKED_DEVICE_TOKEN      = 13, // Payload: cs_mesh_model_msg_device_token_t
	CS_MESH_MODEL_TYPE_SYNC_REQUEST              = 14, // Payload: cs_mesh_model_msg_sync_request_t
//	CS_MESH_MODEL_TYPE_SYNC_RESPONSE             = 15, // Payload: cs_mesh_model_msg_sync_response_t
	CS_MESH_MODEL_TYPE_TRACKED_DEVICE_LIST_SIZE  = 16, // Payload: cs_mesh_model_msg_device_list_size_t
	CS_MESH_MODEL_TYPE_STATE_SET                 = 17, // Payload: cs_mesh_model_msg_state_header_ext_t + payload
	CS_MESH_MODEL_TYPE_RESULT                    = 18, // Payload: cs_mesh_model_msg_result_header_t + payload
	CS_MESH_MODEL_TYPE_SET_IBEACON_CONFIG_ID     = 19, // Payload: set_ibeacon_config_id_packet_t
	CS_MESH_MODEL_TYPE_TRACKED_DEVICE_HEARTBEAT  = 20, // Payload: cs_mesh_model_msg_device_heartbeat_t
	CS_MESH_MODEL_TYPE_RSSI_PING                 = 21, // Payload: rssi_ping_message_t                             Only used in MeshTopologyResearch
	CS_MESH_MODEL_TYPE_TIME_SYNC                 = 22, // Payload: cs_mesh_model_msg_time_sync_t
	CS_MESH_MODEL_TYPE_REPORT_ASSET_MAC          = 23, // Payload: report_asset_mac_t                        Only used in NearestCrownstone
	CS_MESH_MODEL_TYPE_RSSI_DATA                 = 24, // Payload: rssi_data_message_t                             Only used in MeshTopologyResearch
	CS_MESH_MODEL_TYPE_STONE_MAC                 = 25, // Payload: cs_mesh_model_msg_stone_mac_t
	CS_MESH_MODEL_TYPE_ASSET_FILTER_VERSION      = 26, // Payload: cs_mesh_model_msg_asset_filter_version_t
	CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC            = 27, // Payload: cs_mesh_model_msg_asset_rssi_mac_t
	CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI            = 28, // Payload: cs_mesh_model_msg_neighbour_rssi_t
	CS_MESH_MODEL_TYPE_CTRL_CMD                  = 29, // Payload: cs_mesh_model_msg_ctrl_cmd_header_ext_t + payload
	CS_MESH_MODEL_TYPE_REPORT_ASSET_ID           = 30, // Payload: report_asset_id_t // REVIEW: why a different message, can just use the same as asset id forward.
	CS_MESH_MODEL_TYPE_ASSET_RSSI_SID            = 31, // Payload: cs_mesh_model_msg_asset_rssi_sid_t

	CS_MESH_MODEL_TYPE_UNKNOWN                   = 255
};

struct __attribute__((__packed__)) cs_mesh_model_msg_test_t {
	uint32_t counter;
#if MESH_MODEL_TEST_MSG == 2
//	uint8_t dummy[3]; // non segmented
//	uint8_t dummy[12]; // 2 segments
	uint8_t dummy[24]; // 3 segments
#else
	uint8_t dummy[3]; // non segmented
#endif
};

struct __attribute__((__packed__)) cs_mesh_model_msg_time_t {
	uint32_t timestamp;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_profile_location_t {
	uint8_t profile;
	uint8_t location;
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
	uint16_t delay = 0; // Deprecated, set to 0 or backwards compatibility.
	cmd_source_with_counter_t source;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_device_register_t {
	device_id_t deviceId;
	uint8_t locationId;
	uint8_t profileId;
	int8_t rssiOffset;
	uint8_t flags;
	uint8_t accessLevel;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_device_token_t {
	device_id_t deviceId;
	uint8_t deviceToken[TRACKED_DEVICE_TOKEN_SIZE];
	uint16_t ttlMinutes;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_device_heartbeat_t {
	device_id_t deviceId;
	uint8_t locationId;
	uint8_t ttlMinutes;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_device_list_size_t {
	uint8_t listSize;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_sync_request_t {
	/**
	 * ID of crownstone that requests the data
	 *
	 * Not really necessary, as the source address is also the crownstone id.
	 */
	stone_id_t id;
	union __attribute__((__packed__)) {
		struct __attribute__((packed)) {
			bool time           : 1;
			bool trackedDevices : 1;
		} bits;
		uint32_t bitmask;
	};
};

struct __attribute__((__packed__)) cs_mesh_model_msg_state_header_t {
	uint8_t type;                 // Shortened version of CS_TYPE
	uint8_t id : 6;               // Shortened version of state id.
	uint8_t persistenceMode : 2;  // Shortened version of peristenceMode.
};

struct __attribute__((__packed__)) cs_mesh_model_msg_state_header_ext_t {
	cs_mesh_model_msg_state_header_t header;
	uint8_t accessLevel : 3;      // Shortened version of access level.
	uint8_t sourceId : 5;         // Shortened version of source.
};

struct __attribute__((__packed__)) cs_mesh_model_msg_ctrl_cmd_header_t {
	uint16_t cmdType;             // CommandHandlerTypes
};

struct __attribute__((__packed__)) cs_mesh_model_msg_ctrl_cmd_header_ext_t {
	cs_mesh_model_msg_ctrl_cmd_header_t header;
	uint8_t accessLevel : 3;      // Shortened version of access level.
	uint8_t sourceId : 5;         // Shortened version of source.
};

struct __attribute__((__packed__)) cs_mesh_model_msg_result_header_t {
	uint8_t msgType; // Mesh msg type of which this is the result.
	uint8_t retCode;
};

/**
 * Packed version of time_sync_message_t.
 */
struct __attribute__((__packed__)) cs_mesh_model_msg_time_sync_t {
	uint32_t posix_s;        // Seconds since epoch.
	uint16_t posix_ms : 10;  // Milliseconds passed since posix_s.
	uint8_t version : 6;     // Synchronization version,
	bool overrideRoot : 1;   // Whether this time overrides the root time.
	uint8_t reserved : 7;    // @arend maybe use these bits to increase version size.
};


struct __attribute__((__packed__)) compressed_rssi_data_t {
	uint8_t channel : 2; // 0 = unknown, 1 = channel 37, 2 = channel 38, 3 = channel 39
	uint8_t rssiHalved : 6; // half of the absolute value of the original rssi.
};

struct __attribute__((__packed__)) report_asset_mac_t {
	uint8_t trackableDeviceMac[MAC_ADDRESS_LEN];
	compressed_rssi_data_t rssi;
};



struct __attribute__((__packed__)) report_asset_id_t {
	short_asset_id_t id;
	uint8_t reserved[3];
	compressed_rssi_data_t compressedRssi;
};

/**
 * Sent from a crownstone when it has too little rssi information from
 * its neighbors.
 */
struct __attribute__((__packed__)) rssi_ping_message_t {

};

/**
 * The data in this packet contains information about a
 * bluetooth channel between this crownstone and the one
 * with id sender_id.
 */
struct __attribute__((__packed__)) rssi_data_t {
	/**
	 * variance of the given channel, rounded to intervals:
	 * 0: [ 0  - 2^2)
	 * 1: [ 2^2  - 4^2)
	 * 2: [ 4^2  - 6^2)
	 * 3: [ 6^2 -  8^2)
	 * 4: [ 8^2 - 10^2)
	 * 5: [10^2 - 15^2)
	 * 6: [15^2 - 20^2)
	 * 7: 20^2 and over
	 */
	uint16_t variance : 3;

	/**
	 * absolute value of the average rssi
	 */
	uint16_t rssi : 7;

	/**
	 * a samplecount == 0x111111, indicates the channel had
	 * at least 2^6-1 == 63 samples.
	 */
	uint16_t sampleCount : 6;
};

struct __attribute__((__packed__)) rssi_data_message_t {
	stone_id_t sender_id;
	rssi_data_t channel37;
	rssi_data_t channel38;
	rssi_data_t channel39;
};


struct __attribute__((__packed__)) cs_mesh_model_msg_neighbour_rssi_t {
	uint8_t type = 0; // Type of report, always 0 for now.
	stone_id_t neighbourId;
	int8_t rssiChannel37;
	int8_t rssiChannel38;
	int8_t rssiChannel39;
	uint8_t lastSeenSecondsAgo;
	uint8_t counter; // Message counter, to identify package loss at the receiving node.

	// Possible future type:
	//   uint8_t type : 4;
	//   uint8_t rssiChannel37 : 6;
	//   uint8_t rssiChannel38 : 6;
	//   uint8_t rssiChannel39 : 6;
	//   uint8_t avgRssiChannel37 : 6;
	//   uint8_t avgRssiChannel38 : 6;
	//   uint8_t avgRssiChannel39 : 6;
	//   stone_id_t neighbourId;
	//   uint8_t lastSeenSecondsAgo;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_stone_mac_t {
	uint8_t type; // 0 = request, 1 = reply.
	uint8_t connectionProtocol = CS_CONNECTION_PROTOCOL_VERSION;
	uint8_t reserved[5]; // Allows for larger payloads for future types. Set to 0 for now.
};

struct __attribute__((__packed__)) cs_mesh_model_msg_asset_filter_version_t {
	asset_filter_cmd_protocol_t protocol;
	uint16_t masterVersion;
	uint32_t masterCrc;
};

struct __attribute__((__packed__)) cs_mesh_model_msg_asset_rssi_mac_t {
	compressed_rssi_data_t rssiData;
	uint8_t mac[MAC_ADDRESS_LEN];
};


struct __attribute__((__packed__)) cs_mesh_model_msg_asset_rssi_sid_t {
	compressed_rssi_data_t rssiData;
	short_asset_id_t assetId;
	uint8_t reserved[3];
};
