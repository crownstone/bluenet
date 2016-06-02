/*
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

enum MeshChannels {
	HUB_CHANNEL        = 1,
	DATA_CHANNEL       = 2,
};

enum MeshMessageTypes {
	//! data channel messages
	EVENT_MESSAGE      = 0, //! this one might make more sense on the hub channel
							// todo: do we need event messages on the mesh at all?
//	POWER_MESSAGE      = 1, // use command message instead
	BEACON_MESSAGE     = 2,
	COMMAND_MESSAGE    = 3,
	CONFIG_MESSAGE     = 4,

	//! hub channel messages
	SCAN_MESSAGE       = 101,
	POWER_SAMPLES_MESSAGE = 103,
};

//! broadcast address is defined as 00:00:00:00:00:00
#define BROADCAST_ADDRESS {}

#define MAX_MESH_MESSAGE_LEN RBC_MESH_VALUE_MAX_LEN
#define MAX_MESH_MESSAGE_PAYLOAD_LENGTH MAX_MESH_MESSAGE_LEN - BLE_GAP_ADDR_LEN - sizeof(uint16_t)

#define MAX_EVENT_MESH_MESSAGE_DATA_LENGTH MAX_MESH_MESSAGE_PAYLOAD_LENGTH - sizeof(uint16_t)
/** Event mesh message
 */
struct __attribute__((__packed__)) event_mesh_message_t {
	uint16_t event;
//	uint8_t data[MAX_EVENT_MESH_MESSAGE_DATA_LENGTH];
};

/** Beacon mesh message
 */
struct __attribute__((__packed__)) beacon_mesh_message_t {
	uint16_t major;
	uint16_t minor;
	ble_uuid128_t uuid;
	int8_t rssi;
};

#define COMMAND_MM_HEADER_SIZE 4
/** Command mesh message
 */
struct __attribute__((__packed__)) command_mesh_message_t {
	uint8_t command;
	uint8_t reserved; //! reserved for byte alignment
	uint16_t length;
	uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH - COMMAND_MM_HEADER_SIZE];
};

#define CONFIG_MM_HEADER_SIZE 4
/** Config mesh message
 */
struct __attribute__((__packed__)) config_mesh_message_t {
	uint8_t type;
	uint8_t opCode;
	uint16_t length;
	uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH - CONFIG_MM_HEADER_SIZE];
};

/** Device mesh header
 */
struct __attribute__((__packed__)) device_mesh_header_t {
	uint8_t targetAddress[BLE_GAP_ADDR_LEN];
	uint16_t messageType;
};

/** Device mesh message
 */
struct __attribute__((__packed__)) device_mesh_message_t {
	device_mesh_header_t header;
	union {
		uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
		event_mesh_message_t evtMsg;
		beacon_mesh_message_t beaconMsg;
		command_mesh_message_t commandMsg;
		config_mesh_message_t configMsg;
	};
};

#define NR_DEVICES_PER_MESSAGE SR_MAX_NR_DEVICES
//#define NR_DEVICES_PER_MESSAGE 1
//#define NR_DEVICES_PER_MESSAGE 10

/** Scan mesh message
 */
struct __attribute__((__packed__)) scan_mesh_message_t {
	uint8_t numDevices;
	peripheral_device_t list[NR_DEVICES_PER_MESSAGE];
};

#define POWER_SAMPLE_MESH_NUM_SAMPLES 43
//! 91 bytes in total
struct __attribute__((__packed__)) power_samples_mesh_message_t {
	uint32_t timestamp;
//	uint16_t firstSample;
//	int8_t sampleDiff[POWER_SAMPLE_MESH_NUM_SAMPLES-1];
	uint16_t samples[POWER_SAMPLE_MESH_NUM_SAMPLES];
	uint8_t reserved;
//	struct __attribute__((__packed__)) {
//		int8_t dt1 : 4;
//		int8_t dt2 : 4;
//	} timeDiffs[POWER_SAMPLE_MESH_NUM_SAMPLES / 2];
};

/** Hub mesh header
 */
struct __attribute__((__packed__)) hub_mesh_header_t {
	uint8_t sourceAddress[BLE_GAP_ADDR_LEN];
	uint16_t messageType;
};

/** Test mesh message
 */
struct __attribute__((__packed__)) test_mesh_message_t {
	uint32_t counter;
};

/** Hub mesh message
 */
struct __attribute__((__packed__)) hub_mesh_message_t {
	hub_mesh_header_t header;
	union {
		uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
		scan_mesh_message_t scanMsg;
		test_mesh_message_t testMsg;
		power_samples_mesh_message_t powerSamplesMsg;
	};
};

