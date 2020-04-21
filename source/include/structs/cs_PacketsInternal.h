/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 17, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "protocol/cs_Packets.h"
#include "protocol/cs_ErrorCodes.h"
#include <third/nrf/app_config.h>

/**
 * Packets (structs) that are used internally.
 *
 * These should be plain structs, without constructors, or member functions.
 * Instead of a constructor, you can add a function that fills a struct.
 *
 * If the definition becomes large, move it to its own file and include it in this file.
 */

/**
 * Length of a MAC address
 * TODO: check if this length is similar to BLE_GAP_ADDR_LEN
 */
#define MAC_ADDRESS_LEN 6

/**
 * Maximum length advertisement data.
 */
#define ADVERTISEMENT_DATA_MAX_SIZE 31

/**
 * Overhead through encryption
 */
#define ENCRYPTION_OVERHEAD 20

/**
 * Size of MicroApp packet.
 */
#define MICROAPP_PACKET_SIZE (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - ENCRYPTION_OVERHEAD)

/**
 * Size of MicroApp chunk (without header)
 */
#define MICROAPP_CHUNK_SIZE (MICROAPP_PACKET_SIZE - 8)

/**
 * Variable length data encapsulation in terms of length and pointer to data.
 */
struct cs_data_t {
	buffer_ptr_t data = NULL;      /** < Pointer to data. */
	cs_buffer_size_t len = 0;      /** < Length of data. */

	cs_data_t():
		data(),
		len()
	{}
	cs_data_t(buffer_ptr_t buf, cs_buffer_size_t size):
		data(buf),
		len(size)
	{}
};

struct cs_result_t {
	/**
	 * Return code.
	 *
	 * Should be set by the handler.
	 */
	cs_ret_code_t returnCode = ERR_EVENT_UNHANDLED;

	/**
	 * Buffer to put the result data in.
	 *
	 * Can be NULL.
	 */
	cs_data_t buf;

	/**
	 * Length of the data in the buffer.
	 *
	 * Should be set by the handler.
	 */
	cs_buffer_size_t dataSize = 0;

	cs_result_t():
		buf()
	{}
	cs_result_t(cs_data_t buf):
		buf(buf)
	{}
	cs_result_t(cs_ret_code_t returnCode):
		returnCode(returnCode),
		buf()
	{}
};

/**
 * Scanned device.
 */
struct __attribute__((packed)) scanned_device_t {
	int8_t rssi;
	uint8_t address[MAC_ADDRESS_LEN];
	uint8_t addressType; // (ble_gap_addr_t.addr_type) & (ble_gap_addr_t.addr_id_peer << 7).
	uint8_t channel;
	uint8_t dataSize;
//	uint8_t data[ADVERTISEMENT_DATA_MAX_SIZE];
	uint8_t *data;
	// See ble_gap_evt_adv_report_t
	// More possibilities: addressType, connectable, isScanResponse, directed, scannable, extended advertisements, etc.
};


/**
 * A single multi switch command.
 * switchCmd: 0 = off, 100 = fully on.
 * delay: Delay in seconds.
 * source: The source that issued the command.
 */
struct __attribute__((packed)) internal_multi_switch_item_cmd_t {
	uint8_t switchCmd;
	uint16_t delay;
	cmd_source_t source;
};

/**
 * A single multi switch packet, with target id.
 */
struct __attribute__((packed)) internal_multi_switch_item_t {
	stone_id_t id;
	internal_multi_switch_item_cmd_t cmd;
};

inline bool cs_multi_switch_item_is_valid(internal_multi_switch_item_t* item, size16_t size) {
	return (size == sizeof(internal_multi_switch_item_t) && item->id != 0);
}

struct __attribute__((packed)) control_command_packet_t {
	CommandHandlerTypes type;
	buffer_ptr_t data;
	size16_t size;
	EncryptionAccessLevel accessLevel;
	cmd_source_t source;
};

/**
 * Mesh control command packet.
 */
struct __attribute__((__packed__)) mesh_control_command_packet_t {
	mesh_control_command_packet_header_t header;
	stone_id_t* targetIds;
	control_command_packet_t controlCommand;
};

/**
 * How reliable a mesh message should be.
 *
 * For now, the associated number is the number of times the message gets sent.
 */
enum cs_mesh_msg_reliability {
	CS_MESH_RELIABILITY_INVALID = 0,
	CS_MESH_RELIABILITY_LOWEST = 1,
	CS_MESH_RELIABILITY_LOW = 3,
	CS_MESH_RELIABILITY_MEDIUM = 5,
	CS_MESH_RELIABILITY_HIGH = 10
};

/**
 * How urgent a message is.
 *
 * Lower urgency means that it's okay if the message is not sent immediately.
 */
enum cs_mesh_msg_urgency {
	CS_MESH_URGENCY_LOW,
	CS_MESH_URGENCY_HIGH
};

/**
 * Struct to communicate a mesh message.
 * type            Type of message.
 * payload         The payload.
 * size            Size of the payload.
 * reliability     How reliable the message should be.
 * urgency         How quick the message should be sent.
 */
struct cs_mesh_msg_t {
	cs_mesh_model_msg_type_t type;
	uint8_t* payload;
	size16_t size = 0;
	cs_mesh_msg_reliability reliability = CS_MESH_RELIABILITY_LOW;
	cs_mesh_msg_urgency urgency = CS_MESH_URGENCY_LOW;
};

/**
 * Struct to communicate received state of other stones.
 * rssi            RSSI to this stone, or 0 if not received the state directly from that stone.
 * serviceData     State of the stone.
 */
struct state_external_stone_t {
	int8_t rssi;
	service_data_encrypted_t data;
};

/**
 * Unparsed background advertisement.
 */
struct __attribute__((__packed__)) adv_background_t {
	uint8_t protocol;
	uint8_t sphereId;
	uint8_t* data;
	uint8_t dataSize;
	uint8_t* macAddress;
	int8_t rssi;
};

/**
 * Parsed background advertisement.
 */
struct __attribute__((__packed__)) adv_background_parsed_t {
	uint8_t* macAddress;
	int8_t  adjustedRssi;
	uint8_t locationId;
	uint8_t profileId;
	uint8_t flags;
};

struct __attribute__((__packed__)) adv_background_parsed_v1_t {
	uint8_t* macAddress;
	int8_t rssi;
	uint8_t deviceToken[TRACKED_DEVICE_TOKEN_SIZE];
};

struct profile_location_t {
	uint8_t profileId;
	uint8_t locationId;
	bool fromMesh = false;
};

struct __attribute__((packed)) internal_register_tracked_device_packet_t {
	register_tracked_device_packet_t data;
	uint8_t accessLevel = NOT_SET;
};

typedef internal_register_tracked_device_packet_t internal_update_tracked_device_packet_t;

/*
 * Struct for MicroApp code.
 *
 *  - The protocol byte is reserved for future changes.
 *  - The app_id is an identifier for the app (in theory we can support multiple apps).
 *  - The index refers to the chunk index.
 *  - The count refers to the total number of chunks.
 *  - The data size of total program
 *  - The checksum is a checksum for this chunk.
 *
 * Two remarks:
 *  - The checksum of the last packet is the size for the COMPLETE application.
 *  - The data of the last packet has to be filled with 0xFF. It will be written like that to flash. Moreover, its
 *    checksum will be over the entire buffer, including the 0xFF values.
 *  - When the index field is 0xFF, the information in the package is on enabling/disabling the microapp.
 */
struct __attribute__((packed)) microapp_upload_packet_t {
	uint8_t protocol;
	uint8_t app_id;
	uint8_t index;
	uint8_t count;
	uint16_t size;
	uint16_t checksum;
	uint8_t data[MICROAPP_CHUNK_SIZE];
};

/*
 * Struct for MicroApp meta packet (index equal to 0xFF)
 *
 *   - The opcode indicates enabling/disabling the app (or other future actions).
 *   - The param0 parameter contains e.g. an offset when opcode is "enable app".
 *   - The param1 parameter contains e.g. a checksum.
 */
struct __attribute__((packed)) microapp_upload_meta_packet_t {
	uint8_t protocol;
	uint8_t app_id;
	uint8_t index;
	uint8_t opcode;
	uint16_t param0;
	uint16_t param1;
	uint8_t data[MICROAPP_CHUNK_SIZE];
};

/**
 * Notification from the MicroApp module.
 *
 *   - The protocol byte is reserved for future struct changes.
 *   - The app_id identifies the app (for now we use only one app).
 *   - The index refers to the chunk index being written.
 *   - The repeat value is an index that goes down (notifications will be written with decreasing repeat value).
 *   - The error value.
 *
 * After getting the first notification it is already fine to start sending a new packet. Do check the index in
 * the notification though (to not accidentally treat it as an ACK for the current chunk).
 */
struct __attribute__((packed)) microapp_notification_packet_t {
	uint8_t protocol;
	uint8_t app_id;
	uint8_t index;
	uint8_t repeat;
	uint16_t error;
};

