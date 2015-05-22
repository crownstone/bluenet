/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "protocol/cs_MeshControl.h"

#include "drivers/cs_PWM.h"
#include "drivers/cs_Serial.h"

#include <events/cs_EventDispatcher.h>
#include <protocol/cs_Mesh.h>
#include <protocol/cs_MeshMessageTypes.h>

#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

#include <cfg/cs_Settings.h>

MeshControl::MeshControl() : EventListener(EVT_ALL) {
	EventDispatcher::getInstance().addListener(this);
}

extern "C" void decode_data_message(void* p_event_data, uint16_t event_size) {
	MeshControl::getInstance().decodeDataMessage(p_event_data, event_size);
}


/**
 * Get incoming messages and perform certain actions.
 */
void MeshControl::process(uint8_t handle, void* p_data, uint16_t length) {
	LOGi("Process incoming mesh message");
	switch(handle) {
	case EVENT_CHANNEL: {
		if (length < sizeof(device_mesh_message_t)) {
			LOGi("wrong message received, length: %d, sizeof: %d", length, sizeof(device_mesh_message_t));
			break;
		}

		device_mesh_message_t* msg = (device_mesh_message_t*) p_data;

		if (msg->messageType != EVENT_MESSAGE) {
			LOGi("wrong message received");
			break;
		}

//		LOGi("received event for:");
//		BLEutil::printArray(msg->targetAddress, BLE_GAP_ADDR_LEN);
		LOGi("type: %s (%d), from ???", msg->evtMsg.event == EVT_POWER_ON ? "EVT_POWER_ON" : "EVT_POWER_OFF", msg->evtMsg.event);
		_log(INFO, "\r\n");

//		EventMeshPackage* msg = (EventMeshPackage*)p_data;
//		LOGi("received %s (%d) event from ???", msg->evt == EVT_POWER_ON ? "EVT_POWER_ON" : "EVT_POWER_OFF", msg->evt);

		break;
	}
	case DATA_CHANNEL: {

		BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message));
//		decodeDataMessage(p_data, length);
		break;
	}
	}
}

void MeshControl::decodeDataMessage(void* p_data, uint16_t length) {

	device_mesh_message_t* msg = (device_mesh_message_t*) p_data;
//	BLEutil::printArray((uint8_t*)msg, sizeof(msg));

	switch(msg->messageType) {
	case POWER_MESSAGE: {
//		if (length != 1) {
//			LOGi("wrong message, length: %d", length);
//			break;
//		}

		uint8_t pwmValue = msg->powerMsg.pwmValue;
		uint32_t oldPwmValue;
		uint8_t pwmChannel;

		PWM::getInstance().getValue(pwmChannel, oldPwmValue);
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

		_log(INFO, "\r\n");

		break;
	}
	case BEACON_MESSAGE: {
//		if (length != 1) {
//			LOGi("wrong message, length: %d", length);
//			break;
//		}

		LOGi("Received Beacon Message");

		BLEutil::printArray((uint8_t*)p_data, length);

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
		uint8_t targetAddress[BLE_GAP_ADDR_LEN] = {0x03, 0x24, 0x79, 0x56, 0xD6, 0xC6};
		memcpy(msg.targetAddress, &targetAddress, BLE_GAP_ADDR_LEN);
		msg.evtMsg.event = evt;
//		memset(msg.evtMsg.data, 0, sizeof(msg.evtMsg.data));
//		memcpy(msg.evtMsg.data, p_data, length);

		CMesh::getInstance().send(EVENT_CHANNEL, (uint8_t*)&msg, 7 + 2 + length);

		_log(INFO, "\r\n");

//		EventMeshPackage msg;
//		msg.evt = evt;
//		msg.p_data = (uint8_t*)p_data;
//		CMesh::getInstance().send(EVENT_CHANNEL, (uint8_t*)&msg, length + 1);
		break;
	}
	default:
		break;
	}
}



void MeshControl::sendPwmValue(uint8_t* address, uint8_t value) {

	device_mesh_message_t msg = {};
//	uint8_t targetAddress[BLE_GAP_ADDR_LEN] = {};
//	memcpy(msg.targetAddress, &targetAddress, BLE_GAP_ADDR_LEN);

	memset(msg.payload, 0, sizeof(msg.payload));
	msg.powerMsg.pwmValue = value;

	CMesh::getInstance().send(DATA_CHANNEL, (uint8_t*)&msg, sizeof(msg));

}

void MeshControl::sendIBeaconMessage(uint8_t* address, uint16_t major, uint16_t minor, ble_uuid128_t uuid, int8_t rssi) {

	device_mesh_message_t msg = {};
//	uint8_t targetAddress[BLE_GAP_ADDR_LEN] = {};
//	memcpy(msg.targetAddress, &targetAddress, BLE_GAP_ADDR_LEN);

	msg.beaconMsg.major = major;
	msg.beaconMsg.minor = minor;
	memcpy(&msg.beaconMsg.uuid, &uuid, sizeof(uuid));
	msg.beaconMsg.rssi = rssi;

	CMesh::getInstance().send(DATA_CHANNEL, (uint8_t*)&msg, sizeof(msg));

}
