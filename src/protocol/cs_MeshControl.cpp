/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "protocol/cs_MeshControl.h"

#include "drivers/cs_PWM.h"

#include <events/cs_EventDispatcher.h>
#include <protocol/cs_Mesh.h>

#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

#include <cfg/cs_Settings.h>

MeshControl::MeshControl() : EventListener(EVT_ALL) {
	EventDispatcher::getInstance().addListener(this);
    sd_ble_gap_address_get(&_myAddr);
}

extern "C" void decode_data_message(void* p_event_data, uint16_t event_size) {

	device_mesh_message_t* msg = (device_mesh_message_t*) p_event_data;
	MeshControl::getInstance().decodeDataMessage(msg);
}


/**
 * Get incoming messages and perform certain actions.
 */
void MeshControl::process(uint8_t handle, void* p_data, uint16_t length) {
	LOGi("Process incoming mesh message");
	switch(handle) {
	case HUB_CHANNEL: {

		break;
	}
	case DATA_CHANNEL: {
		if (!isValidMessage(p_data, length)) {
			return;
		}

		if (isBroadcast(p_data) || isMessageForUs(p_data)) {
			BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message));
		}

		break;
	}
	}
}

void MeshControl::decodeDataMessage(device_mesh_message_t* msg) {

//	BLEutil::printArray((uint8_t*)msg, sizeof(msg));

	switch(msg->header.messageType) {
	case EVENT_MESSAGE: {
//		if (!isValidMessage(p_data, length)) {
//			return;
//		}

//		LOGi("received event for:");
//		BLEutil::printArray(msg->targetAddress, BLE_GAP_ADDR_LEN);
		LOGi("type: %s (%d), from ???", msg->evtMsg.event == EVT_POWER_ON ? "EVT_POWER_ON" : "EVT_POWER_OFF", msg->evtMsg.event);

//		EventMeshPackage* msg = (EventMeshPackage*)p_data;
//		LOGi("received %s (%d) event from ???", msg->evt == EVT_POWER_ON ? "EVT_POWER_ON" : "EVT_POWER_OFF", msg->evt);

		break;
	}
	case POWER_MESSAGE: {
//		if (!isValidMessage(p_data, length)) {
//			return;
//		}

		uint8_t pwmValue = msg->powerMsg.pwmValue;

		uint32_t oldPwmValue = PWM::getInstance().getValue(0);
		if (pwmValue != oldPwmValue) {
			PWM::getInstance().setValue(0, pwmValue);
			if (pwmValue == 0) {
				LOGi("Turn lamp/device off");
				EventDispatcher::getInstance().dispatch(EVT_POWER_OFF);
			} else {
				LOGi("Turn lamp/device on");
				EventDispatcher::getInstance().dispatch(EVT_POWER_ON);
			}
		} else {
			LOGi("skip pwm message");
		}

		break;
	}
	case BEACON_MESSAGE: {
//		if (!isValidMessage(p_data, length)) {
//			return;
//		}

		LOGi("Received Beacon Message");
//		BLEutil::printArray((uint8_t*)msg, sizeof(mesh_header_t) + sizeof(beacon_mesh_message_t));

		uint16_t major = msg->beaconMsg.major;
		uint16_t minor = msg->beaconMsg.minor;
		ble_uuid128_t& uuid = msg->beaconMsg.uuid;
		int8_t& rssi = msg->beaconMsg.rssi;

		if (major != 0) {
			Settings::getInstance().writeToStorage(CONFIG_IBEACON_MAJOR, (uint8_t*)&major, sizeof(major), false);
		}

		if (minor != 0) {
			Settings::getInstance().writeToStorage(CONFIG_IBEACON_MINOR, (uint8_t*)&minor, sizeof(minor), false);
		}

		if (memcmp(&uuid, new uint8_t[16] {}, 16)) {
			Settings::getInstance().writeToStorage(CONFIG_IBEACON_UUID, (uint8_t*)&uuid, sizeof(uuid), false);
		}

		if (rssi != 0) {
			Settings::getInstance().writeToStorage(CONFIG_IBEACON_RSSI, (uint8_t*)&rssi, sizeof(rssi), false);
		}

		Settings::getInstance().savePersistentStorage();

		break;
	}
	}

}

void MeshControl::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case EVT_POWER_ON:
	case EVT_POWER_OFF: {
		assert(length < MAX_EVENT_MESH_MESSAGE_DATA_LENGTH, "event data is too long");

		LOGi("send event %s", evt == EVT_POWER_ON ? "EVT_POWER_ON" : "EVT_POWER_OFF");

		device_mesh_message_t msg;
		uint8_t targetAddress[BLE_GAP_ADDR_LEN] = BROADCAST_ADDRESS;
		memcpy(msg.header.targetAddress, &targetAddress, BLE_GAP_ADDR_LEN);
		msg.evtMsg.event = evt;
//		memset(msg.evtMsg.data, 0, sizeof(msg.evtMsg.data));
//		memcpy(msg.evtMsg.data, p_data, length);

		CMesh::getInstance().send(DATA_CHANNEL, (uint8_t*)&msg, 7 + 2 + length);

		break;
	}
	default:
		break;
	}
}

void MeshControl::send(uint8_t handle, void* p_data, uint8_t length) {

	if (!isValidMessage(p_data, length)) {
		return;
	}

	if (isBroadcast(p_data)) {
		// received broadcast message
		LOGd("received broadcast, send into mesh and handle directly");
		CMesh::getInstance().send(handle, p_data, length);
		BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message));

	} else if (!isMessageForUs(p_data)) {
		// message is not for us, send it into mesh
		LOGd("send it into mesh ...");
		CMesh::getInstance().send(handle, p_data, length);
	} else {
		// message is for us, handle directly
		LOGd("message is for us, handle directly");
		BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message));
	}

}
