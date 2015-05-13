/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 6, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstddef>

#include <ble_gap.h>

///////////////////////////////////////////

//struct mesh_message_base_t;

//struct __attribute__((__packed__)) mesh_message_t {
////	uint8_t targetAddress[BLE_GAP_ADDR_LEN];
//	uint8_t* targetAddress;
//	mesh_message_base_t payload;
//};
//
//struct __attribute__((__packed__)) event_mesh_message_t : mesh_message_base_t {
//	EventType type;
//	uint8_t* p_data;
//};

/////////////////////////////////////////

#define MAX_MESH_MESSAGE_LEN 25
#define MAX_MESH_MESSAGE_PAYLOAD_LENGTH MAX_MESH_MESSAGE_LEN - BLE_GAP_ADDR_LEN

#define MAX_EVENT_MESH_MESSAGE_DATA_LENGTH MAX_MESH_MESSAGE_PAYLOAD_LENGTH - sizeof(uint16_t)
struct __attribute__((__packed__)) event_mesh_message_t {
	uint16_t type;
	uint8_t data[MAX_EVENT_MESH_MESSAGE_DATA_LENGTH];
};


struct __attribute__((__packed__)) mesh_message_t {
	uint8_t targetAddress[BLE_GAP_ADDR_LEN];
	union {
		uint8_t payload[MAX_MESH_MESSAGE_PAYLOAD_LENGTH];
		event_mesh_message_t evtMsg;
	};
};

/////////////////////////////////////////

//class MeshMessageBase {
//
//private:
//	uint8_t _targetAddress[BLE_GAP_ADDR_LEN];
//	uint8_t* _payload;
////	uint16_t _payloadLength;
//
//public:
//	MeshMessageBase() :
//		_payload(NULL)
////		_,payloadLength(0)
//{};
//
//	inline uint8_t* getTargetAddress() { return _targetAddress; }
//
//	inline uint8_t* getPayload() { return _payload; }
//
////	inline uint16_t getPayloadLength() { return _payloadLength; }
//
//};
//
//class EventMeshMessage : public MeshMessageBase {
//
//private:
//	EventType _type;
//	uint8_t* p_data;
//
//public:
//	EventMeshMessage() {};
//
//	inline EventType getType() {
//		return _type;
//	}
//
//
//
//};

