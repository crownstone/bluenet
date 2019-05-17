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
 * timeout: delay in seconds.
 */
struct __attribute__((packed)) internal_multi_switch_item_cmd_t {
	uint8_t switchCmd;
	uint16_t timeout; // timeout in seconds
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
struct __attribute__((__packed__)) adv_background_payload_t {
	uint8_t locationId;
	uint8_t profileId;
	uint8_t rssiOffset;
	uint8_t flags;
};
