/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

//#include <cstddef>

#include <ble_gap.h>

#include <third/protocol/rbc_mesh.h>

#include <structs/cs_ScanResult.h>
#include <structs/cs_StreamBuffer.h>

enum MeshChannels {
	HUB_CHANNEL     = 1,
	DATA_CHANNEL    = 2,
};

enum MeshMessageTypes {
	//! data channel messages
	CONTROL_MESSAGE       = 0,
	BEACON_MESSAGE        = 1,
	CONFIG_MESSAGE        = 2,
	STATE_MESSAGE         = 3,

	//! hub channel messages
	SCAN_MESSAGE          = 101,
//	UNUSED                = 102,
	POWER_SAMPLES_MESSAGE = 103,
	EVENT_MESSAGE         = 104,    // todo: do we need event messages on the mesh at all?
};

//! broadcast address is defined as 00:00:00:00:00:00
#define BROADCAST_ADDRESS {}

//! available number of bytes for a mesh message
#define MAX_MESH_MESSAGE_LEN RBC_MESH_VALUE_MAX_LEN
/** number of bytes used for our mesh message header
 *   - target MAC address: 6B
 *   - message type: 2B
 */
#define MAX_MESH_MESSAGE_HEADER_LENGTH BLE_GAP_ADDR_LEN - sizeof(uint16_t)
//! available number of bytes for the mesh message payload. the payload depends on the message type
#define MAX_MESH_MESSAGE_PAYLOAD_LENGTH MAX_MESH_MESSAGE_LEN - MAX_MESH_MESSAGE_HEADER_LENGTH
//! available number of bytes for the data of the message, for command and config messages
#define MAX_MESH_MESSAGE_DATA_LENGTH MAX_MESH_MESSAGE_PAYLOAD_LENGTH - SB_HEADER_SIZE

/** Event mesh message
 */
struct __attribute__((__packed__)) event_mesh_message_t {
	uint16_t event;
//	uint8_t data[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
};

/** Beacon mesh message
 */
struct __attribute__((__packed__)) beacon_mesh_message_t {
	uint16_t major;
	uint16_t minor;
	ble_uuid128_t uuid;
	int8_t txPower;
};

using control_mesh_message_t = stream_t<uint8_t, MAX_MESH_MESSAGE_DATA_LENGTH>;
using config_mesh_message_t = stream_t<uint8_t, MAX_MESH_MESSAGE_DATA_LENGTH>;
using state_mesh_message_t = stream_t<uint8_t, MAX_MESH_MESSAGE_DATA_LENGTH>;

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
		control_mesh_message_t commandMsg;
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

