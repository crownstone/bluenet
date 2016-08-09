/**
 * Author: Dominik Egger
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <mesh/cs_MeshControl.h>

#include <storage/cs_Settings.h>
#include <drivers/cs_RNG.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_CommandHandler.h>
#include <mesh/cs_Mesh.h>

// enable for additional debug output
//#define PRINT_DEBUG
//#define PRINT_MESHCONTROL_VERBOSE

MeshControl::MeshControl() : EventListener(EVT_ALL) {
	EventDispatcher::getInstance().addListener(this);
    sd_ble_gap_address_get(&_myAddr);
//    Timer::getInstance().createSingleShot(_resetTimerId, (app_timer_timeout_handler_t)MeshControl::reset);
//    Timer::getInstance().start(_resetTimerId, MS_TO_TICKS(20000), NULL);
}


/*
// [30.05.16] not needed anymore since the softdevice events are now handled directly through the scheduler
//   we don't need to decouple it ourselves anymore, but can directly handle them
extern "C" void decode_data_message(void* p_event_data, uint16_t event_size) {
	device_mesh_message_t* msg = (device_mesh_message_t*) p_event_data;
	MeshControl::getInstance().decodeDataMessage(msg->header.messageType, msg->payload);
}
*/

void MeshControl::process(uint8_t channel, void* p_data, uint16_t length) {
//	LOGd("Process incoming mesh message");

//	Timer::getInstance().stop(_resetTimerId);
//	Timer::getInstance().start(_resetTimerId, MS_TO_TICKS(20000), NULL);

	switch(channel) {
	case HUB_CHANNEL: {

		//! are we the hub? then process the message
		//! maybe answer on the data channel to the node that sent it that
		//! we received the message ??
		//! but basically we don't need to do anything, the
		//! hub can just read out the mesh characteristic for the hub channel

//		LOGd("ch %d: received hub message:", channel);
//		BLEutil::printArray((uint8_t*)p_data, length);

		hub_mesh_message_t* msg = (hub_mesh_message_t*)p_data;
//		LOGd("message type: %d", msg->header.messageType);
		switch(msg->header.messageType) {
		case SCAN_MESSAGE: {

#ifdef PRINT_MESHCONTROL_VERBOSE
			LOGf("Crownstone %s scanned these devices:", getAddress((mesh_message_t*)p_data).c_str());
#endif
			if (msg->scanMsg.numDevices > NR_DEVICES_PER_MESSAGE) {
				LOGe("Invalid number of devices!");
			}
			else {
#ifdef PRINT_DEBUG
				for (int i = 0; i < msg->scanMsg.numDevices; ++i) {
					peripheral_device_t dev = msg->scanMsg.list[i];
//					if ((dev.addr[5] == 0xED && dev.addr[4] == 0x01 && dev.addr[3] == 0x53 && dev.addr[2] == 0xB8 && dev.addr[1] == 0x6F && dev.addr[0] == 0xCC) ||
//						(dev.addr[5] == 0xC1 && dev.addr[4] == 0x1F && dev.addr[3] == 0xDC && dev.addr[2] == 0xF9 && dev.addr[1] == 0xB3 && dev.addr[0] == 0xFC)) {
						LOGi("%d: [%02X %02X %02X %02X %02X %02X]   rssi: %4d    occ: %3d", i, dev.addr[5],
								dev.addr[4], dev.addr[3], dev.addr[2], dev.addr[1],
								dev.addr[0], dev.rssi, dev.occurrences);
//					}
				}
#endif
			}

			break;
		}
		case SERVICE_DATA_MESSAGE: {
#ifdef PRINT_MESHCONTROL_VERBOSE
			LOGd("Received service data from crownstone %s", getAddress((mesh_message_t*)p_data).c_str());
#endif

#ifdef PRINT_DEBUG
			service_data_mesh_message_t& sd = msg->serviceDataMsg;
			LOGd("> crownstone id: %d", sd.crownstoneId);
			LOGd("> switch state: %d", sd.switchState);
			LOGd("> event bitmask: %s", BLEutil::toBinaryString(sd.eventBitmask).c_str());
			LOGd("> power usage: %d", sd.powerUsage);
			LOGd("> accumulated energy: %d", sd.accumulatedEnergy);
			LOGd("> temperature: %d", sd.temperature);
#endif

			break;
		}
//		case 102: {
//			if (firstTimeStamp == 0) {
//				firstTimeStamp = RTC::getCount();
//				firstCounter[channel-1] = msg->testMsg.counter;
//			}
//			if (lastCounter[channel-1] != 0 && msg->testMsg.counter != 0 && lastCounter[channel-1] +1 != msg->testMsg.counter) {
//				incident[channel-1] += msg->testMsg.counter - lastCounter[channel-1] - 1;
//				double loss = incident[channel-1] * 100.0 / (msg->testMsg.counter - firstCounter[channel-1]);
//				uint32_t dt = RTC::ticksToMs(RTC::difference(RTC::getCount(), firstTimeStamp));
//				double msgsPerSecond = 0;
//				if (dt != 0) {
//					msgsPerSecond = 1000.0 * (msg->testMsg.counter - firstCounter[channel-1]) / dt;
//				}
////				LOGe("ch %d: %d missed, last: %d, current: %d, loss: %d %%", channel, incident[channel-1],
////						lastCounter[channel-1], msg->testMsg.counter, (uint32_t)loss);
//				LOGe("ch %d: %d missed, current: %d, loss: %d %%, msgs/s: %d", channel, incident[channel-1],
//						msg->testMsg.counter, (uint32_t)loss, (uint32_t)msgsPerSecond);
//			}
//			lastCounter[channel-1] = msg->testMsg.counter;
//			LOGi(">> count: %d", msg->testMsg.counter);
//			break;
//		}

		}

		break;
	}
	case DATA_CHANNEL: {
		mesh_message_t* msg = (mesh_message_t*)p_data;

		if (!isValidMessage(msg, length)) {
			return;
		}

		if (isBroadcast(msg) || isMessageForUs(msg)) {
			//! [01.12.2015] I think this is not necessary anymore with the new ble mesh version
			//! since the receive is not anymore handled in an interrupt handler, but has to be done
			//! manually. so we are already doing it in a timer which is executed by the app scheduler.
			//! so now we handle it by the scheduler, then put it back in the scheduler queue and again
			//! pick it up later
//			BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message));

			decodeDataMessage(msg->header.messageType, msg->payload);
		} else {
#ifdef PRINT_MESHCONTROL_VERBOSE
			LOGi("Message not for us: %s", getAddress(msg).c_str());
#endif
		}

		break;
	}
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20: {
		mesh_message_t* msg = (mesh_message_t*) p_data;
#ifdef PRINT_MESHCONTROL_VERBOSE
		LOGd("power samples: h=%u src id=%s", channel, getAddress(msg).c_str());
#endif
		break;
	}
	}
}


void MeshControl::decodeDataMessage(uint16_t type, uint8_t* payload) {

	switch(type) {
	case EVENT_MESSAGE: {
		break;
	}
	case CONFIG_MESSAGE: {
		config_mesh_message_t* msg = (config_mesh_message_t*)payload;
		uint8_t type = msg->type;
		uint16_t length = msg->length;
		uint8_t* payload = msg->payload;
		Settings::getInstance().writeToStorage(type, payload, length);
		break;
	}
	case CONTROL_MESSAGE: {
		control_mesh_message_t* msg = (control_mesh_message_t*)payload;
		CommandHandlerTypes command = (CommandHandlerTypes)msg->type;
		uint16_t length = msg->length;
		uint8_t* msgPayload = msg->payload;

		switch(command) {
		case CMD_ENABLE_SCANNER: {
			//! need to use a random delay for starting the scanner, otherwise
			//! the devices in the mesh will start scanning at the same time
			//! resulting in lots of conflicts
			RNG rng;
			enable_scanner_message_payload_t* pl = (enable_scanner_message_payload_t*)msgPayload;
			// todo: can't edit the delay on the original payload, otherwise the mesh goes crazy with
			//   conflicting values. so for now we make a local copy
			enable_scanner_message_payload_t scannerPayload = *pl;

			uint16_t crownstoneId;
			Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &crownstoneId);
			if (crownstoneId != 0) {
				scannerPayload.delay = crownstoneId * 1000;
			} else {
				RNG rng;
				scannerPayload.delay = rng.getRandom16() / 1; //! Delay in ms (about 0-60 seconds)
			}

			CommandHandler::getInstance().handleCommand(command, (uint8_t*)&scannerPayload, 3);
			return;
		}
		case CMD_REQUEST_SERVICE_DATA: {
			//! need to delay the sending of the service data or all devices will write their
			//! service data to the mesh at the same time. so solution for now, use crownstone
			//! id (if set) as the delay
			uint16_t crownstoneId;
			Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &crownstoneId);
			uint32_t delay;
			if (crownstoneId != 0) {
				delay = crownstoneId * 100;
			} else {
				RNG rng;
				delay = rng.getRandom16() / 6; //! Delay in ms (about 0-60 seconds)
			}
			CommandHandler::getInstance().handleCommandDelayed(command, msgPayload, length, delay);
			return;
		}
		default:
			break;
		}

		CommandHandler::getInstance().handleCommand(command, msgPayload, length);
		break;
	}
	case BEACON_MESSAGE: {

#ifdef PRINT_MESHCONTROL_VERBOSE
		LOGi("Received Beacon Message");
#endif
		beacon_mesh_message_t* msg = (beacon_mesh_message_t*)payload;
		//		BLEutil::printArray((uint8_t*)msg, sizeof(mesh_header_t) + sizeof(beacon_mesh_message_t));

		uint16_t major = msg->major;
		uint16_t minor = msg->minor;
		ble_uuid128_t& uuid = msg->uuid;
		int8_t& rssi = msg->txPower;

		Settings& settings = Settings::getInstance();

		bool hasChange = false;

		uint16_t oldMajor;
		settings.get(CONFIG_IBEACON_MAJOR, &oldMajor);
		if (major != 0 && major != oldMajor) {
			settings.writeToStorage(CONFIG_IBEACON_MAJOR, (uint8_t*)&major, sizeof(major), false);
			hasChange = true;
		}

		uint16_t oldMinor;
		settings.get(CONFIG_IBEACON_MINOR, &oldMinor);
		if (minor != 0 && minor != oldMinor) {
			settings.writeToStorage(CONFIG_IBEACON_MINOR, (uint8_t*)&minor, sizeof(minor), false);
			hasChange = true;
		}

		uint8_t oldUUID[16];
		settings.get(CONFIG_IBEACON_UUID, oldUUID);
		if (memcmp(&uuid, new uint8_t[16] {}, 16) && memcmp(&uuid, oldUUID, 16)) {
			settings.writeToStorage(CONFIG_IBEACON_UUID, (uint8_t*)&uuid, sizeof(uuid), false);
			hasChange = true;
		}

		int8_t oldRssi;
		settings.get(CONFIG_IBEACON_TXPOWER, &oldRssi);
		if (rssi != 0 && rssi != oldRssi) {
			settings.writeToStorage(CONFIG_IBEACON_TXPOWER, (uint8_t*)&rssi, sizeof(rssi), false);
			hasChange = true;
		}

		if (hasChange) {
			//			settings.savePersistentStorage();
			// instead of saving the whole config, only store the whole iBeacon struct
			settings.saveIBeaconPersistent();
		}

		// if iBeacon is enabled, trigger event to update the advertisement with the new iBeacon
		// parameters. This doesn't depend on if it is currently advertising or not but can be done
		// in either state
		if (settings.isSet(CONFIG_IBEACON_ENABLED)) {
			EventDispatcher::getInstance().dispatch(EVT_ADVERTISEMENT_UPDATED);
		}

		break;
	}
	}

}

//! handle event triggered by the EventDispatcher, in case we want to send events
//! into the mesh, e.g. for power on/off
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

//! used by the mesh characteristic to send a message into the mesh
ERR_CODE MeshControl::send(uint8_t channel, void* p_data, uint8_t length) {

	switch(channel) {
	case DATA_CHANNEL: {

		mesh_message_t* msg = (mesh_message_t*)p_data;
		if (!isValidMessage(msg, length)) {
			return ERR_INVALID_MESSAGE;
		}

		if (isBroadcast(msg)) {
			//! received broadcast message
			LOGd("Received broadcast");
//			log(INFO, "message:");
//			BLEutil::printArray((uint8_t*)p_data, length);
			Mesh::getInstance().send(channel, p_data, length);
			// [30.05.16] as long as we don't call this function in an interrupt, we don't need to
			//   decouple it anymore, because softdevice events are handled already by the scheduler
			//	 BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message));
			device_mesh_message_t* msg = (device_mesh_message_t*)p_data;
			decodeDataMessage(msg->header.messageType, msg->payload);

		} else if (!isMessageForUs(msg)) {
			//! message is not for us, send it into mesh
			LOGd("Message not for us, send into mesh ...");
			Mesh::getInstance().send(channel, p_data, length);
		} else {
			//! message is for us, handle directly, no reason to send it into the mesh!
			LOGd("Message is for us");
			// [30.05.16] as long as we don't handle this in an interrupt, we don't need to
			//   decouple it anymore, because softdevice events are handled already by the scheduler
			//	 BLE_CALL(app_sched_event_put, (p_data, length, decode_data_message)););
			device_mesh_message_t* msg = (device_mesh_message_t*)p_data;
			decodeDataMessage(msg->header.messageType, msg->payload);
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

		//! otherwise, send it into the mesh, so that it is being forwarded
		//! to the hub
		Mesh::getInstance().send(channel, p_data, length);

//		}

		break;
	}
	}

	return ERR_SUCCESS;

}

//! sends the result of a scan, i.e. a list of scanned devices with rssi values
//! into the mesh on the hub channel so that it can be synced to the cloud
void MeshControl::sendScanMessage(peripheral_device_t* p_list, uint8_t size) {

#ifdef PRINT_MESHCONTROL_VERBOSE
	LOGi("Send ScanMessage, size: %d", size);
#endif

	//! if no devices were scanned there is no reason to send a message!
	if (size > 0) {
		hub_mesh_message_t* message = createHubMessage(SCAN_MESSAGE);
		message->scanMsg.numDevices = size;
		memcpy(&message->scanMsg.list, p_list, size * sizeof(peripheral_device_t));

#ifdef PRINT_DEBUG
		LOGi("message data:");
		BLEutil::printArray(message, sizeof(hub_mesh_message_t));
#endif

		Mesh::getInstance().send(HUB_CHANNEL, message, sizeof(hub_mesh_message_t));
		free(message);
	}

}

void MeshControl::sendPowerSamplesMessage(power_samples_mesh_message_t* samples) {

#ifdef PRINT_MESHCONTROL_VERBOSE
	LOGd("Send PowerSamplesMessage");
#endif

	hub_mesh_message_t* message = createHubMessage(POWER_SAMPLES_MESSAGE);
	memcpy(&message->powerSamplesMsg, samples, sizeof(power_samples_mesh_message_t));
	uint16_t handle = (message->header.address[0] % (MESH_NUM_HANDLES-2-1)) + 3;
	Mesh::getInstance().send(handle, message, sizeof(hub_mesh_message_t));
	free(message);
}

void MeshControl::sendServiceDataMessage(service_data_mesh_message_t* serviceData) {

#ifdef PRINT_MESHCONTROL_VERBOSE
	LOGd("Send service data");
#endif

	hub_mesh_message_t* message = createHubMessage(SERVICE_DATA_MESSAGE);
	memcpy(&message->serviceDataMsg, serviceData, sizeof(service_data_mesh_message_t));

#ifdef PRINT_DEBUG
	LOGi("message data:");
	BLEutil::printArray(message, sizeof(hub_mesh_message_t));
#endif

	Mesh::getInstance().send(HUB_CHANNEL, message, sizeof(hub_mesh_message_t));
	free(message);
}
