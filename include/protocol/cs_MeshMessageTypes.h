/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstddef>

#include <ble_gap.h>

#include <third/protocol/rbc_mesh.h>

#include <structs/cs_ScanResult.h>

// device messages
#define EVENT_MESSAGE 0
#define POWER_MESSAGE 1
#define BEACON_MESSAGE 2

// hub messages
#define SCAN_MESSAGE 101

#define BROADCAST_ADDRESS {}

#define MAX_MESH_MESSAGE_LEN RBC_MESH_VALUE_MAX_LEN
#define MAX_MESH_MESSAGE_PAYLOAD_LENGTH MAX_MESH_MESSAGE_LEN - BLE_GAP_ADDR_LEN - sizeof(uint16_t)

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

struct __attribute__((__packed__)) device_mesh_header_t {
	uint8_t targetAddress[BLE_GAP_ADDR_LEN];
	uint16_t messageType;
};

struct __attribute__((__packed__)) device_mesh_message_t {
	device_mesh_header_t header;
	union {
		uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
		event_mesh_message_t evtMsg;
		power_mesh_message_t powerMsg;
		beacon_mesh_message_t beaconMsg;
	};
};

//#define NR_DEVICES_PER_MESSAGE SR_MAX_NR_DEVICES
//#define NR_DEVICES_PER_MESSAGE 1
#define NR_DEVICES_PER_MESSAGE 10
struct __attribute__((__packed__)) scan_mesh_message_t {
	uint8_t numDevices;
	peripheral_device_t list[NR_DEVICES_PER_MESSAGE];
};

struct __attribute__((__packed__)) hub_mesh_header_t {
	uint8_t sourceAddress[BLE_GAP_ADDR_LEN];
	uint16_t messageType;
};

struct __attribute__((__packed__)) test_mesh_message_t {
	uint32_t counter;
};

struct __attribute__((__packed__)) hub_mesh_message_t {
	hub_mesh_header_t header;
	union {
		uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
		scan_mesh_message_t scanMsg;
		test_mesh_message_t testMsg;
	};
};

