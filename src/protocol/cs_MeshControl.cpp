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

MeshControl::MeshControl() : EventListener(EVT_ALL) {
	EventDispatcher::getInstance().addListener(this);
}

/**
 * Get incoming messages and perform certain actions.
 */
void MeshControl::process(uint8_t handle, void* p_data, uint16_t length) {
	LOGi("Process incoming mesh message");
	switch(handle) {
	case EVENT_CHANNEL: {
//		if (length != 1) {
//			LOGi("wrong message, length: %d", length);
//			break;
//		}

		mesh_message_t* msg = (mesh_message_t*) p_data;
		LOGi("received event for:");
		BLEutil::printArray(msg->targetAddress, BLE_GAP_ADDR_LEN);
		LOGi("type: %s (%d), from ???", msg->evtMsg.type == EVT_POWER_ON ? "EVT_POWER_ON" : "EVT_POWER_OFF", msg->evtMsg.type);

//		EventMeshPackage* msg = (EventMeshPackage*)p_data;
//		LOGi("received %s (%d) event from ???", msg->evt == EVT_POWER_ON ? "EVT_POWER_ON" : "EVT_POWER_OFF", msg->evt);

		break;
	}
	case PWM_CHANNEL: {
		if (length != 1) {
			LOGi("wrong message, length: %d", length);
			break;
		}

		uint8_t* message = (uint8_t*)p_data;

		uint32_t pwm_value;
		uint8_t channel;
		PWM::getInstance().getValue(channel, pwm_value);
		if (*message == 1 && pwm_value == 0) {
			LOGi("Turn lamp/device on");
			PWM::getInstance().setValue(0, (uint8_t)-1);
		} else if (*message == 0 && pwm_value != 0) {
			LOGi("Turn lamp/device off");
			PWM::getInstance().setValue(0, 0);
		} else {
			LOGi("skip pwm message");
		}
		break;
	}
	}
}

void MeshControl::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case EVT_POWER_ON:
	case EVT_POWER_OFF: {
		assert(length < MAX_EVENT_MESH_MESSAGE_DATA_LENGTH, "event data is too long");
		LOGi("handle power event");

		mesh_message_t msg;
		uint8_t targetAddress[BLE_GAP_ADDR_LEN] = {0x03, 0x24, 0x79, 0x56, 0xD6, 0xC6};
		memcpy(msg.targetAddress, &targetAddress, BLE_GAP_ADDR_LEN);
		msg.evtMsg.type = evt;
		memcpy(msg.evtMsg.data, p_data, length);

		CMesh::getInstance().send(EVENT_CHANNEL, (uint8_t*)&msg, sizeof(msg));

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
