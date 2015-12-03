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

//uint32_t lastCounter[3] = {};
//uint32_t incident[3] = {};

/**
 * Get incoming messages and perform certain actions.
 */
void MeshControl::process(uint8_t channel, void* p_data, uint16_t length) {
//	LOGi("Process incoming mesh message");
	switch(channel) {
//	case 3:
	case HUB_CHANNEL: {

//		LOGi("ch %d: received hub message:", channel);
//		BLEutil::printArray((uint8_t*)p_data, length);

		hub_mesh_message_t* msg = (hub_mesh_message_t*)p_data;
		switch(msg->header.messageType) {
		case SCAN_MESSAGE: {
//			_logFirst(INFO, "device ");
//			BLEutil::printInlineArray(msg->header.sourceAddress, BLE_GAP_ADDR_LEN);
//			_log(INFO, " scanned these devices:\r\n");
			LOGi("Device %02X %02X %02X %02X %02X %02X scanned these devices:", msg->header.sourceAddress[5],
					msg->header.sourceAddress[4], msg->header.sourceAddress[3], msg->header.sourceAddress[2],
					msg->header.sourceAddress[1], msg->header.sourceAddress[0]);
			for (int i = 0; i < msg->scanMsg.numDevices; ++i) {
				peripheral_device_t dev = msg->scanMsg.list[i];
				LOGi("%d: [%02X %02X %02X %02X %02X %02X]   rssi: %4d    occ: %3d", i, dev.addr[5],
						dev.addr[4], dev.addr[3], dev.addr[2], dev.addr[1],
						dev.addr[0], dev.rssi, dev.occurrences);
			}
			break;
		}
//		case 102: {
//			if (msg->testMsg.counter != 0 && lastCounter[channel-1] +1 != msg->testMsg.counter) {
//				incident[channel-1] += msg->testMsg.counter - lastCounter[channel-1] - 1;
//				double loss = incident[channel-1] * 100.0 / msg->testMsg.counter;
//				LOGe("ch %d: %d missed, last: %d, current: %d, loss: %d %%", channel, incident[channel-1],
//						lastCounter[channel-1], msg->testMsg.counter, (uint32_t)loss);
//			}
//			lastCounter[channel-1] = msg->testMsg.counter;
////			LOGi(">> count: %d", msg->testMsg.counter);
//			break;
//		}

		}

		// are we the hub? then process the message
		// maybe answer on the data channel to the node that sent it that
		// we received the message ??
		// but basically we don't need to do anything, the
		// hub can just read out the mesh characteristic for the hub channel

		break;
	}
	case DATA_CHANNEL: {
		if (!isValidMessage(p_data, length)) {
			return;
		}

		if (isBroadcast(p_data) || isMessageForUs(p_data)) {
			// [01.12.2015] I think this is not necessary anymore with the new ble mesh version
			// since the receive is not anymore handled in an interrupt handler, but has to be done
			// manually. so we are already doing it in a timer which is executed by the app scheduler.
			// so now we handled it by the scheduler, then put it back in the scheduler queue and again
			// pick it up later
			BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message));
		} else {
			_log(INFO, "Message not for us: ");
			BLEutil::printArray(((device_mesh_message_t*)p_data)->header.targetAddress, BLE_GAP_ADDR_LEN);
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
				EventDispatcher::getInstance().dispatch(EVT_POWER_ON, &pwmValue, sizeof(pwmValue));
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

#if BEACON==1
		LOGi("Received Beacon Message");
//		BLEutil::printArray((uint8_t*)msg, sizeof(mesh_header_t) + sizeof(beacon_mesh_message_t));

		uint16_t major = msg->beaconMsg.major;
		uint16_t minor = msg->beaconMsg.minor;
		ble_uuid128_t& uuid = msg->beaconMsg.uuid;
		int8_t& rssi = msg->beaconMsg.rssi;

		EventDispatcher::getInstance().dispatch(EVT_ADVERTISEMENT_PAUSE);

		bool hasChange = false;
		ps_configuration_t cfg = Settings::getInstance().getConfig();

		uint16_t oldMajor;
		Storage::getUint16((uint32_t&)cfg.beacon.major, oldMajor, BEACON_MAJOR);
		if (major != 0 && major != oldMajor) {
			Settings::getInstance().writeToStorage(CONFIG_IBEACON_MAJOR, (uint8_t*)&major, sizeof(major), false);
			hasChange = true;
		}

		uint16_t oldMinor;
		Storage::getUint16((uint32_t&)cfg.beacon.minor, oldMinor, BEACON_MINOR);
		if (minor != 0 && minor != oldMinor) {
			Settings::getInstance().writeToStorage(CONFIG_IBEACON_MINOR, (uint8_t*)&minor, sizeof(minor), false);
			hasChange = true;
		}

		if (memcmp(&uuid, new uint8_t[16] {}, 16) && memcmp(&uuid, cfg.beacon.uuid.uuid128, 16)) {
			Settings::getInstance().writeToStorage(CONFIG_IBEACON_UUID, (uint8_t*)&uuid, sizeof(uuid), false);
			hasChange = true;
		}

		int8_t oldRssi;
		Storage::getInt8((int32_t&)cfg.beacon.rssi, oldRssi, BEACON_RSSI);
		if (rssi != 0 && rssi != oldRssi) {
			Settings::getInstance().writeToStorage(CONFIG_IBEACON_RSSI, (uint8_t*)&rssi, sizeof(rssi), false);
			hasChange = true;
		}

		if (hasChange) {
			Settings::getInstance().savePersistentStorage();
		}

		EventDispatcher::getInstance().dispatch(EVT_ADVERTISEMENT_RESUME);
#endif

		break;
	}
	}

}

void MeshControl::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
//	case EVT_POWER_ON:
//	case EVT_POWER_OFF: {
//		assert(length < MAX_EVENT_MESH_MESSAGE_DATA_LENGTH, "event data is too long");
//
//		LOGi("send event %s", evt == EVT_POWER_ON ? "EVT_POWER_ON" : "EVT_POWER_OFF");
//
//		device_mesh_message_t msg;
//		uint8_t targetAddress[BLE_GAP_ADDR_LEN] = BROADCAST_ADDRESS;
//		memcpy(msg.header.targetAddress, &targetAddress, BLE_GAP_ADDR_LEN);
//		msg.evtMsg.event = evt;
////		memset(msg.evtMsg.data, 0, sizeof(msg.evtMsg.data));
////		memcpy(msg.evtMsg.data, p_data, length);
//
//		CMesh::getInstance().send(DATA_CHANNEL, (uint8_t*)&msg, 7 + 2 + length);
//
//		break;
//	}
	default:
		break;
	}
}

void MeshControl::send(uint8_t channel, void* p_data, uint8_t length) {

	switch(channel) {
	case DATA_CHANNEL: {

		if (!isValidMessage(p_data, length)) {
			return;
		}

		if (isBroadcast(p_data)) {
			// received broadcast message
			LOGd("received broadcast, send into mesh and handle directly");
			CMesh::getInstance().send(channel, p_data, length);
			BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message));

		} else if (!isMessageForUs(p_data)) {
			// message is not for us, send it into mesh
			LOGd("send it into mesh ...");
			CMesh::getInstance().send(channel, p_data, length);
		} else {
			// message is for us, handle directly
			LOGd("message is for us, handle directly");
			BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message));
		}

		break;
	}
	case HUB_CHANNEL: {

//		if (are we connected to a hub, or are we the hub??) {
//			then store the message
//			and notify the hub about the message
//			since we are the hub, there is no reason to send it into the mesh, it would
//			  just come back to us through the mesh
//		} else {

		// otherwise, send it into the mesh, so that it is being forwarded
		// to the hub
		CMesh::getInstance().send(channel, p_data, length);

//		}

		break;
	}
	}

}

void MeshControl::sendScanMessage(peripheral_device_t* p_list, uint8_t size) {

	LOGi("sendScanMessage, size: %d", size);

	if (size > 0) {
		hub_mesh_message_t message;
		memset(&message, 0, sizeof(message));
		memcpy(&message.header.sourceAddress, &_myAddr.addr, BLE_GAP_ADDR_LEN);
		message.header.messageType = SCAN_MESSAGE;
		message.scanMsg.numDevices = size;
		memcpy(&message.scanMsg.list, p_list, size * sizeof(peripheral_device_t));

		LOGi("message data:");
		BLEutil::printArray(&message, sizeof(message));
		CMesh::getInstance().send(HUB_CHANNEL, &message, sizeof(message));

	}

}
