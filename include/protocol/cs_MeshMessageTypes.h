/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstddef>

#include <ble_gap.h>


#define EVENT_MESSAGE 0
#define POWER_MESSAGE 1
#define BEACON_MESSAGE 2




#define MAX_MESH_MESSAGE_LEN 90
#define MAX_MESH_MESSAGE_PAYLOAD_LENGTH MAX_MESH_MESSAGE_LEN - BLE_GAP_ADDR_LEN - sizeof(uint8_t)

#define MAX_EVENT_MESH_MESSAGE_DATA_LENGTH MAX_MESH_MESSAGE_PAYLOAD_LENGTH - sizeof(uint16_t)
struct __attribute__((__packed__)) event_mesh_message_t {
	uint16_t event;
//	uint8_t data[MAX_EVENT_MESH_MESSAGE_DATA_LENGTH];
};

struct __attribute__((__packed__)) power_mesh_message_t {
	uint8_t pwmValue;
};

struct __attribute__((__packed__)) beacon_mesh_message_t {
	uint16_t major;
	uint16_t minor;
	ble_uuid128_t uuid;
	int8_t rssi;
};

struct __attribute__((__packed__)) device_mesh_message_t {
	uint8_t targetAddress[BLE_GAP_ADDR_LEN];
	uint16_t messageType;
	union {
		uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
		event_mesh_message_t evtMsg;
		power_mesh_message_t powerMsg;
		beacon_mesh_message_t beaconMsg;
	};
};

struct __attribute__((__packed__)) hub_mesh_message_t {
	uint8_t payload[MAX_MESH_MESSAGE_LEN];
};


