/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

//#include <cstddef>

#define VERSION_V2

#include <ble_gap.h>

extern "C" {
#include <rbc_mesh.h>
}

#include <structs/cs_ScanResult.h>
#include <structs/cs_StreamBuffer.h>
#include <structs/cs_PowerSamples.h>

enum MeshChannels {
	KEEP_ALIVE_CHANNEL       = 1,
	STATE_BROADCAST_CHANNEL  = 2,
	STATE_CHANGE_CHANNEL     = 3,
	COMMAND_CHANNEL          = 4,
	COMMAND_REPLY_CHANNEL    = 5,
	SCAN_RESULT_CHANNEL      = 6,
	BIG_DATA_CHANNEL         = 7,
};

enum MeshCommandTypes {
	CONTROL_MESSAGE          = 0,
	BEACON_MESSAGE           = 1,
	CONFIG_MESSAGE           = 2,
	STATE_MESSAGE            = 3,
};

enum MeshReplyTypes {
	STATUS_REPLY             = 0,
	CONFIG_REPLY             = 1,
	STATE_REPLY              = 2,
};

//! available number of bytes for a mesh message
//#define MAX_MESH_MESSAGE_LEN RBC_MESH_VALUE_MAX_LEN
/** number of bytes used for our mesh message header
 *   - target MAC address: 6B
 *   - message type: 2B
 */
//#define MAX_MESH_MESSAGE_HEADER_LENGTH (BLE_GAP_ADDR_LEN + sizeof(uint16_t))
////! available number of bytes for the mesh message payload. the payload depends on the message type
//#define MAX_MESH_MESSAGE_PAYLOAD_LENGTH (MAX_MESH_MESSAGE_LEN - MAX_MESH_MESSAGE_HEADER_LENGTH)
////! available number of bytes for the data of the message, for command and config messages
//#define MAX_MESH_MESSAGE_DATA_LENGTH (MAX_MESH_MESSAGE_PAYLOAD_LENGTH - SB_HEADER_SIZE)

typedef uint16_t id_type_t;

/********************************************************************
 *
 ********************************************************************/

#define ENCRYPTED_HEADER_SIZE (sizeof(uint32_t) + sizeof(uint32_t))
#define MAX_ENCRYPTED_PAYLOAD_LENGTH (RBC_MESH_VALUE_MAX_LEN - ENCRYPTED_HEADER_SIZE)
#define PAYLOAD_HEADER_SIZE (sizeof(uint32_t))
#define MAX_MESH_MESSAGE_LENGTH (MAX_ENCRYPTED_PAYLOAD_LENGTH - PAYLOAD_HEADER_SIZE)

struct encrypted_mesh_message_t {
	uint32_t messageCounter;
	uint32_t random;
	uint8_t encrypted_payload[MAX_ENCRYPTED_PAYLOAD_LENGTH];
};

struct mesh_message_t {
	uint32_t messageCounter;
	uint8_t payload[MAX_MESH_MESSAGE_LENGTH];
};

/********************************************************************
 * KEEP ALIVE
 ********************************************************************/

struct __attribute__((__packed__)) keep_alive_item_t {
	id_type_t id;
	uint8_t actionSwitchState;
};

#define KEEP_ALIVE_HEADER_SIZE (sizeof(uint16_t) + sizeof(uint8_t))
#define MAX_KEEP_ALIVE_ITEMS ((MAX_MESH_MESSAGE_LENGTH - KEEP_ALIVE_HEADER_SIZE) / sizeof(keep_alive_item_t))

struct __attribute__((__packed__)) keep_alive_message_t {
	uint16_t timeout;
	uint8_t size;
	keep_alive_item_t list[MAX_KEEP_ALIVE_ITEMS];
};

inline bool has_keep_alive_item(keep_alive_message_t* message, id_type_t id, keep_alive_item_t** item) {

	*item = message->list;
	for (int i = 0; i < message->size; ++i) {
		if ((*item)->id == id) {
			return true;
		}
		++(*item);
	}

	return false;
}

/********************************************************************
 * STATE
 ********************************************************************/

struct __attribute__((__packed__)) state_item_t {
	id_type_t id;
	uint8_t switchState;
	uint32_t powerUsage;
	uint32_t accumulatedEnergy;
};

#define STATE_HEADER_SIZE (sizeof(uint8_t) - sizeof(uint8_t) - sizeof(uint8_t))
#define MAX_STATE_ITEMS ((MAX_MESH_MESSAGE_LENGTH - STATE_HEADER_SIZE) / sizeof(state_item_t))

/*
 * tail points to the index where the next element will be added
 * head points to the index where the first element can be read
 * size indicates the number of elements in the list
 */
struct __attribute__((__packed__)) state_message_t {
	uint8_t head;
	uint8_t tail;
	uint8_t size;
	state_item_t list[MAX_STATE_ITEMS];
};

/*
 * HELPER FUNCTIONS
 */

inline void push_state_item(state_message_t* message, state_item_t* item) {
	if (++message->size > MAX_STATE_ITEMS) {
		message->head = (message->head + 1) % MAX_STATE_ITEMS;
		--message->size;
	}
	memcpy(&message->list[message->tail], item, sizeof(state_item_t));
	message->tail = (message->tail + 1) % MAX_STATE_ITEMS;
}

inline bool peek_next_state_item(state_message_t* message, state_item_t** item, int16_t& index) {
	if (message->size == 0) {
		return false;
	} else if (index == -1) {
		index = message->head;
	} else {
		index = (index + 1) % MAX_STATE_ITEMS;
		if (index == message->tail) return false;
	}
	*item = &message->list[index];
	return true;
}

inline bool pop_state_item(state_message_t* message, state_item_t* item) {
	if (message->size > 0) {
		memcpy(item, &message->list[message->tail], sizeof(state_item_t));
		message->head = (message->head + 1) % MAX_STATE_ITEMS;
		--message->size;
		return true;
	} else {
		return false;
	}
}

/********************************************************************
 * COMMAND
 ********************************************************************/

#define MIN_COMMAND_HEADER_SIZE (sizeof(uint16_t) + sizeof(uint8_t))
#define MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH (MAX_MESH_MESSAGE_LENGTH - MIN_COMMAND_HEADER_SIZE - SB_HEADER_SIZE)

using control_mesh_message_t = stream_t<uint8_t, MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH>;
using config_mesh_message_t = stream_t<uint8_t, MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH>;
using state_mesh_message_t = stream_t<uint8_t, MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH>;

/** Beacon mesh message
 */
struct __attribute__((__packed__)) beacon_mesh_message_t {
	uint16_t major;
	uint16_t minor;
	ble_uuid128_t uuid;
	int8_t txPower;
};

struct __attribute__((__packed__)) command_message_t {
	uint16_t messageType;
	uint8_t numOfIds;
	union {
		struct {
			id_type_t ids[];
			union {
				uint8_t payload[];
				beacon_mesh_message_t beaconMsg;
				control_mesh_message_t commandMsg;
				config_mesh_message_t configMsg;
			};
		} data;
		uint8_t raw[MAX_COMMAND_MESSAGE_PAYLOAD_LENGTH]; // dummy item, makes the reply_message_t a fixed size (for allocation)
	};
};

inline bool is_broadcast_command(command_message_t* message) {
	return message->numOfIds == 0;
}

inline bool is_command_for_us(command_message_t* message, id_type_t id) {

	if (is_broadcast_command(message)) {
		return true;
	} else {
		id_type_t* p_id;

		p_id = message->data.ids;
		for (int i = 0; i < message->numOfIds; ++i) {
			if (*p_id == id) {
				return true;
			}
		}

		return false;
	}
}

inline void get_payload(command_message_t* message, uint16_t messageLength, uint8_t** payload, uint16_t& payloadLength) {
	payloadLength =  messageLength - MIN_COMMAND_HEADER_SIZE + message->numOfIds * sizeof(id_type_t);
	*payload = (uint8_t*)message + MIN_COMMAND_HEADER_SIZE + message->numOfIds * sizeof(id_type_t);
}

/********************************************************************
 * REPLY
 ********************************************************************/

#define REPLY_HEADER_SIZE (sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint8_t))

//struct __attribute__((__packed__)) reply_item_t {
//	id_type_t id;
//	union {
//		uint8_t data[];
//		uint8_t status;
//	};
//};

#define MAX_REPLY_LIST_SIZE (MAX_MESH_MESSAGE_LENGTH - REPLY_HEADER_SIZE)

struct __attribute__((__packed__)) status_reply_item_t {
	id_type_t id;
	uint16_t status;
};

#define MAX_STATUS_REPLY_ITEMS ((MAX_MESH_MESSAGE_LENGTH - REPLY_HEADER_SIZE) / sizeof(status_reply_item_t))


#define MAX_CONFIG_REPLY_DATA_LENGTH (MAX_MESH_MESSAGE_LENGTH - REPLY_HEADER_SIZE - sizeof(id_type_t) - SB_HEADER_SIZE)
#define MAX_CONFIG_REPLY_ITEMS 1 // the safe is to request config one by one, but depending on the length of the config

struct __attribute__((__packed__)) config_reply_item_t {
	id_type_t id;
	stream_t<uint8_t, MAX_CONFIG_REPLY_DATA_LENGTH> data;
};

#define MAX_STATE_REPLY_DATA_LENGTH (MAX_MESH_MESSAGE_LENGTH - REPLY_HEADER_SIZE - sizeof(id_type_t) - SB_HEADER_SIZE)
#define MAX_STATE_REPLY_ITEMS 1 // the safe is to request state one by one, but depending on the length of the state
                                // data that is requested, several states might fit in one mesh message

struct __attribute__((__packed__)) state_reply_item_t {
	id_type_t id;
	stream_t<uint8_t, MAX_STATE_REPLY_DATA_LENGTH> data;
};

struct __attribute__((__packed__)) reply_message_t {
	uint16_t messageType;
	uint32_t messageCounter;
	uint8_t numOfReplys;
	union {
//		reply_item_t list[];
		status_reply_item_t statusList[MAX_STATUS_REPLY_ITEMS];
		config_reply_item_t configList[1];
		state_reply_item_t stateList[1];
		uint8_t rawList[MAX_REPLY_LIST_SIZE]; // dummy item, makes the reply_message_t a fixed size (for allocation)
	};
};

inline bool push_status_reply_item(reply_message_t* message, status_reply_item_t* item) {
	if (message->numOfReplys == MAX_STATUS_REPLY_ITEMS) {
		return false;
	} else {
		memcpy(&message->statusList[message->numOfReplys++], item, sizeof(status_reply_item_t));
		return true;
	}
}

/********************************************************************
 * SCAN RESULT
 ********************************************************************/

struct __attribute__((__packed__)) scan_result_item_t {
	id_type_t id;
	uint8_t address[BLE_GAP_ADDR_LEN];
	int8_t rssi;
};

#define SCAN_RESULT_HEADER_SIZE (sizeof(uint8_t))
#define MAX_SCAN_RESULT_ITEMS ((MAX_MESH_MESSAGE_LENGTH - SCAN_RESULT_HEADER_SIZE) / sizeof(scan_result_item_t))

struct __attribute__((__packed__)) scan_result_message_t {
	uint8_t numOfResults;
	scan_result_item_t list[MAX_SCAN_RESULT_ITEMS];
};

//struct __attribute__((__packed__)) scan_result_item_t {
//	uint8_t address[BLE_GAP_ADDR_LEN];
//	int8_t rssi;
//};
//
//#define SCAN_RESULT_HEADER_SIZE (sizeof(id_type_t) + sizeof(uint8_t))
//#define MAX_SCAN_RESULT_ITEMS ((MAX_PAYLOAD_LENGTH - SCAN_RESULT_HEADER_SIZE) / sizeof(scan_result_item_t))
//
//struct __attribute__((__packed__)) scan_result_message_t {
//	id_type_t id;
//	uint8_t numOfResults;
//	scan_result_item_t list[];
//};









//
///** Event mesh message
// */
//struct __attribute__((__packed__)) event_mesh_message_t {
//	uint16_t event;
////	uint8_t data[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
//};
//
//using control_mesh_message_t = stream_t<uint8_t, MAX_MESH_MESSAGE_DATA_LENGTH>;
//using config_mesh_message_t = stream_t<uint8_t, MAX_MESH_MESSAGE_DATA_LENGTH>;
//using state_mesh_message_t = stream_t<uint8_t, MAX_MESH_MESSAGE_DATA_LENGTH>;
//
//
//#define NR_DEVICES_PER_MESSAGE SR_MAX_NR_DEVICES
////#define NR_DEVICES_PER_MESSAGE 1
////#define NR_DEVICES_PER_MESSAGE 10
//
///** Scan mesh message
// */
//struct __attribute__((__packed__)) scan_mesh_message_t {
//	uint8_t numDevices;
//	peripheral_device_t list[NR_DEVICES_PER_MESSAGE];
//};
//
////#define POWER_SAMPLE_MESH_NUM_SAMPLES 43
//////! 91 bytes in total
////struct __attribute__((__packed__)) power_samples_mesh_message_t {
////	uint32_t timestamp;
//////	uint16_t firstSample;
//////	int8_t sampleDiff[POWER_SAMPLE_MESH_NUM_SAMPLES-1];
////	uint16_t samples[POWER_SAMPLE_MESH_NUM_SAMPLES];
////	uint8_t reserved;
//////	struct __attribute__((__packed__)) {
//////		int8_t dt1 : 4;
//////		int8_t dt2 : 4;
//////	} timeDiffs[POWER_SAMPLE_MESH_NUM_SAMPLES / 2];
////};
//typedef power_samples_cont_message_t power_samples_mesh_message_t;
//
///** Test mesh message
// */
//struct __attribute__((__packed__)) test_mesh_message_t {
//	uint32_t counter;
//};
//
//struct __attribute__((__packed__)) service_data_mesh_message_t {
//	uint16_t crownstoneId;
//	uint8_t switchState;
//	uint8_t eventBitmask;
//	int32_t powerUsage;
//	int32_t accumulatedEnergy;
//	int32_t temperature;
//};
//
//
////#ifdef VERSION_V2
//
///** mesh header
// */
//struct __attribute__((__packed__)) mesh_header_v2_t {
//	uint16_t crownstoneId;
//	uint16_t reason;
//	uint16_t userId;
//	uint16_t messageType;
//};
//
////#else
//
///** mesh header
// */
//struct __attribute__((__packed__)) mesh_header_t {
//	uint8_t address[BLE_GAP_ADDR_LEN];
//	uint16_t messageType;
//};
//
////#endif
//
//
//struct __attribute__((__packed__)) mesh_message_t {
//	mesh_header_t header;
//	uint8_t payload[1]; // dynamic size
//};
//
///** Device mesh message
// */
//struct __attribute__((__packed__)) device_mesh_message_t {
//	mesh_header_t header;
//	union {
//		uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
//		event_mesh_message_t evtMsg;
//		beacon_mesh_message_t beaconMsg;
//		control_mesh_message_t commandMsg;
//		config_mesh_message_t configMsg;
//	};
//};
//
////struct __attribute__((__packed__)) hub_mesh_header_t {
////	uint16_t sourceCrownstoneId;
////	uint16_t messageType;
////};
//
//struct __attribute__((__packed__)) hub_mesh_message_t {
//	mesh_header_t header;
//	union {
//		uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
//		scan_mesh_message_t scanMsg;
//		test_mesh_message_t testMsg;
//		power_samples_mesh_message_t powerSamplesMsg;
//		service_data_mesh_message_t serviceDataMsg;
//	};
//};
