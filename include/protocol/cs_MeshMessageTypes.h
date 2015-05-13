/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstddef>

#include <ble_gap.h>

#define MAX_MESH_MESSAGE_LEN 190
#define MAX_MESH_MESSAGE_PAYLOAD_LENGTH MAX_MESH_MESSAGE_LEN - BLE_GAP_ADDR_LEN

#define MAX_EVENT_MESH_MESSAGE_DATA_LENGTH MAX_MESH_MESSAGE_PAYLOAD_LENGTH - sizeof(uint16_t)
struct __attribute__((__packed__)) event_mesh_message_t {
	uint16_t type;
	uint8_t data[MAX_EVENT_MESH_MESSAGE_DATA_LENGTH];
};

struct __attribute__((__packed__)) power_mesh_message_t {
	uint8_t pwmValue;
};

struct __attribute__((__packed__)) device_mesh_message_t {
	uint8_t targetAddress[BLE_GAP_ADDR_LEN];
	union {
		uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
		event_mesh_message_t evtMsg;
		power_mesh_message_t powerMsg;
	};
};

struct __attribute__((__packed__)) hub_mesh_message_t {
	uint8_t payload[MAX_MESH_MESSAGE_LEN];
};


