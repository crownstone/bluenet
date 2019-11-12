/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 17, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "protocol/cs_Packets.h"

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

struct command_result_t {
	cs_ret_code_t returnCode;
	cs_data_t data;

	command_result_t():
		returnCode(),
		data()
	{}

	command_result_t(cs_ret_code_t code):
		returnCode(code),
		data()
	{}
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




/**
 * A single keep alive packet, with target id.
 */
struct __attribute__((packed)) keep_alive_state_item_t {
	stone_id_t id;
	keep_alive_state_item_cmd_t cmd;
};

inline bool cs_keep_alive_state_item_is_valid(keep_alive_state_item_t* item, size16_t size) {
	return (size == sizeof(keep_alive_state_item_t) && item->id != 0 && item->cmd.timeout != 0);
}

struct __attribute__((packed)) control_command_packet_t {
	CommandHandlerTypes type;
	buffer_ptr_t data;
	size16_t size;
	EncryptionAccessLevel accessLevel;
	cmd_source_t source;
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
 * Background advertisement.
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
	uint8_t protocol;
	uint8_t sphereId;
	uint8_t* macAddress;
	int8_t  adjustedRssi;
	uint8_t locationId;
	uint8_t profileId;
	uint8_t flags;
};
