/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <cstdint>
//#include <stdint.h>
//#include <common/cs_Types.h>

#include <ble/cs_Nordic.h>

#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"

#include <events/cs_EventListener.h>
#include <events/cs_EventTypes.h>

#include <protocol/cs_MeshMessageTypes.h>

#include <structs/cs_ScanResult.h>

//struct __attribute__((__packed__)) EventMeshPackage {
//	EventType evt;
//	uint8_t* p_data;
//};

#define HUB_CHANNEL          1
#define DATA_CHANNEL         2

class MeshControl : public EventListener {
private:
	MeshControl();

	MeshControl(MeshControl const&); // singleton, deny implementation
	void operator=(MeshControl const &); // singleton, deny implementation

    ble_gap_addr_t _myAddr;
    app_timer_id_t _resetTimerId;

    bool isMessageForUs(void* p_data) {
    	device_mesh_message_t* msg = (device_mesh_message_t*) p_data;

    	if (memcmp(msg->header.targetAddress, _myAddr.addr, BLE_GAP_ADDR_LEN) == 0) {
    		// target address of package is set to our address
    		return true;
    	} else {
//    		_log(INFO, "message not for us, target: ");
//    		BLEutil::printArray(msg->header.targetAddress, BLE_GAP_ADDR_LEN);
			return false;
    	}
    }

    bool isBroadcast(void* p_data) {
    	device_mesh_message_t* msg = (device_mesh_message_t*) p_data;
//    	uint8_t broadcastAddr[BLE_GAP_ADDR_LEN] = BROADCAST_ADDRESS;
//    	return memcmp(msg->header.targetAddress, broadcastAddr, BLE_GAP_ADDR_LEN) == 0;
    	return memcmp(msg->header.targetAddress, new uint8_t[BLE_GAP_ADDR_LEN] BROADCAST_ADDRESS, BLE_GAP_ADDR_LEN) == 0;
    }

	bool isValidMessage(void* p_data, uint16_t length) {
		device_mesh_message_t* msg = (device_mesh_message_t*) p_data;

		if (msg->header.messageType == COMMAND_MESSAGE) {
			// command message has an array for parameters which doesn't have to be filled
			// so we don't know in advance how long it needs to be exactly. can only give
			// a lower bound.
			return length > getMessageSize(msg->header.messageType);
		} else {
			uint16_t desiredLength = getMessageSize(msg->header.messageType);
			bool valid = length == desiredLength;

			if (!valid) {
				LOGd("invalid message, length: %d != %d", length, desiredLength);
			}
			return valid;
		}
	}

	uint16_t getMessageSize(uint16_t messageType) {
		switch(messageType) {
		case EVENT_MESSAGE:
			return sizeof(device_mesh_header_t) + sizeof(event_mesh_message_t);
		case POWER_MESSAGE:
			return sizeof(device_mesh_header_t) + sizeof(power_mesh_message_t);
		case BEACON_MESSAGE:
			return sizeof(device_mesh_header_t) + sizeof(beacon_mesh_message_t);
		case COMMAND_MESSAGE:
			return sizeof(device_mesh_header_t) + sizeof(uint16_t);
		default:
			return 0;
		}
	}

	static void reset();


public:
	// use static variant of singelton, no dynamic memory allocation
	static MeshControl& getInstance() {
		static MeshControl instance;
		return instance;
	}

	/**
	 * Get incoming messages and perform certain actions.
	 */
	void process(uint8_t channel, void* p_data, uint16_t length);

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

//	void sendPwmValue(uint8_t* address, uint8_t value);
//	void sendIBeaconMessage(uint8_t* address, uint16_t major, uint16_t minor, ble_uuid128_t uuid, int8_t rssi);

	void sendScanMessage(peripheral_device_t* p_list, uint8_t size);

	void decodeDataMessage(device_mesh_message_t* msg);

	void send(uint8_t channel, void* p_data, uint8_t length);

private:


};
