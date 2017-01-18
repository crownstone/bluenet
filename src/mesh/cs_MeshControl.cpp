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
#include <common/cs_Types.h>

// enable for additional debug output
#define PRINT_DEBUG
#define PRINT_MESHCONTROL_VERBOSE

//#define PRINT_VERBOSE_KEEPALIVE
//#define PRINT_VERBOSE_STATE_BROADCAST
//#define PRINT_VERBOSE_STATE_CHANGE
//#define PRINT_VERBOSE_SCAN_RESULT
//#define PRINT_VERBOSE_COMMAND
//#define PRINT_VERBOSE_COMMAND_REPLY

MeshControl::MeshControl() : EventListener(EVT_ALL), _myCrownstoneId(0) {
	EventDispatcher::getInstance().addListener(this);
}

void MeshControl::init() {
	uint32_t err_code;
	err_code = sd_ble_gap_address_get(&_myAddr);
	APP_ERROR_CHECK(err_code);
	Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &_myCrownstoneId);
}

//KEEP_ALIVE_CHANNEL
//STATE_BROADCAST_CHANNEL
//STATE_CHANGE_CHANNEL
//COMMAND_CHANNEL
//COMMAND_REPLY_CHANNEL
//SCAN_RESULT_CHANNEL
//BIG_DATA_CHANNEL

//void MeshControl::process(uint8_t channel, void* p_data, uint16_t length) {
void MeshControl::process(uint8_t channel, void* p_meshMessage, uint16_t messageLength) {
//	LOGd("Process incoming mesh message");

	mesh_message_t* meshMessage = (mesh_message_t*)p_meshMessage;
	uint8_t* p_data = meshMessage->payload;
	uint16_t length = messageLength - PAYLOAD_HEADER_SIZE;

	switch(channel) {
	case KEEP_ALIVE_CHANNEL: {
		LOGi("received keep alive");

		keep_alive_message_t* msg = (keep_alive_message_t*)p_data;
		keep_alive_item_t* p_item = (keep_alive_item_t*)msg->list;
		uint16_t timeout = msg->timeout;

		if (length < KEEP_ALIVE_HEADER_SIZE + msg->size * sizeof(keep_alive_item_t)) {
			LOGe("invalid message, length too small")
			BLEutil::printArray(p_data, length);
			return;
		}

		for (int i = 0; i < msg->size; ++i) {
			if (p_item->id == _myCrownstoneId) {
				handleKeepAlive(p_item, timeout);
				return;
			}
			++p_item;
		}

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_KEEPALIVE)
		LOGi("keep alive, not for us");
		BLEutil::printArray(p_data, length);
#endif

		break;
	}
	case COMMAND_CHANNEL: {
		LOGi("received command");

		command_message_t* msg = (command_message_t*)p_data;

		// only check the "header" part, so message type, and ids, the payload length
		// will be checked in the handleCommand
		if (length < MIN_COMMAND_HEADER_SIZE + msg->numOfIds * sizeof(id_type_t) + SB_HEADER_SIZE) {
			LOGe("invalid message, length too small")
			BLEutil::printArray(p_data, length);
			return;
		}

		uint8_t* payload;
		uint16_t payloadLength;

		if (is_command_for_us(msg, _myCrownstoneId)) {

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_COMMAND)
			LOGi("is for us: ");
			BLEutil::printArray(msg, length);
#endif

			get_payload(msg, length, &payload, payloadLength);

			ERR_CODE errCode;
			errCode = handleCommand(msg->messageType, payload, payloadLength);

			sendStatusReplyMessage(meshMessage->messageCounter, errCode);
		} else {
#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_COMMAND)
			LOGi("not for us: ");
			BLEutil::printArray(msg, length);
#endif
		}

//		uint8_t* ptr;
//		uint16_t payloadLength = length - MIN_COMMAND_HEADER_SIZE;
//		bool handle = false;
//		if (msg->numOfIds == 0) {
//			// broadcast
//			handle = true;
//			ptr = (uint8_t*)&msg->data.ids;
//		} else {
//			id_type_t* p_id;
//			p_id = msg->data.ids;
//			for (int i = 0; i < msg->numOfIds; ++i) {
//				if (*p_id == _myCrownstoneId) {
//					handle = true;
////					break;
//				}
//				++p_id;
//				payloadLength -= sizeof(id_type_t);
//			}
//			ptr = (uint8_t*)p_id;
//		}
//
//		if (handle) {
//			LOGi("received command: ");
//			BLEutil::printArray(msg, length);
//
//			handleCommand(msg->messageType, ptr, payloadLength);
//		} else {
//			LOGi("command not for us: ");
//			BLEutil::printArray(msg, length);
//		}

		break;
	}

	case STATE_BROADCAST_CHANNEL:
	case STATE_CHANGE_CHANNEL: {
		LOGd("received state %s", channel == STATE_CHANGE_CHANNEL ? "change" : "broadcast");

#if defined(PRINT_DEBUG)

#if defined(PRINT_VERBOSE_STATE_BROADCAST) && defined(PRINT_VERBOSE_STATE_CHANGE)
		// ok
#elif defined(PRINT_VERBOSE_STATE_BROADCAST)
		if (channel == STATE_CHANGE_CHANNEL) {
			break;
		}
#elif defined(PRINT_VERBOSE_STATE_CHANGE)
		if (channel == STATE_BROADCAST_CHANNEL) {
			break;
		}
#else
		break;
#endif

		state_message_t* msg = (state_message_t*)p_data;
		BLEutil::printArray(msg, length);

		int16_t idx = -1;
		state_item_t* p_stateItem;
		while (peek_next_state_item(msg, &p_stateItem, idx)) {
			LOGi("idx: %d", idx);
			LOGi("Crownstone Id %d", p_stateItem->id);
			LOGi("  switch state: %d", p_stateItem->switchState);
			LOGi("  power usage: %d", p_stateItem->powerUsage);
			LOGi("  accumulated energy: %d", p_stateItem->accumulatedEnergy);
		}

#endif
		break;
	}
	case COMMAND_REPLY_CHANNEL: {
		LOGi("received command reply");

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_COMMAND_REPLY)
		reply_message_t* msg = (reply_message_t*)p_data;

		BLEutil::printArray(p_data, length);

		LOGi("Received Reply for Message: %d", msg->messageCounter);
		switch(msg->messageType) {
		case STATUS_REPLY: {

			for (int i = 0; i < msg->numOfReplys; ++i) {
				LOGi("  ID %d: %d", msg->list[i].id, msg->list[i].status);
			}
			break;
		}
		}

#endif

		break;
	}
	case SCAN_RESULT_CHANNEL: {
		LOGi("Received scan results");

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_SCAN_RESULT)
		scan_result_message_t* msg = (scan_result_message_t*)p_data;

		BLEutil::printArray(p_data, length);

		int lastId = 0;
		for (int i = 0; i < msg->numOfResults; ++i) {
			scan_result_item_t scanResultItem = msg->list[i];
			if (scanResultItem.id != lastId) {
				lastId = scanResultItem.id;
				LOGi("Crownstone ID %d scanned:", scanResultItem.id);
			}

			LOGi("   [%02X %02X %02X %02X %02X %02X]   rssi: %4d", scanResultItem.address[5],
					scanResultItem.address[4], scanResultItem.address[3], scanResultItem.address[2],
					scanResultItem.address[1], scanResultItem.address[0], scanResultItem.rssi);
		}
#endif

		break;
	}
	case BIG_DATA_CHANNEL: {
		break;
	}
	}
}

void MeshControl::handleKeepAlive(keep_alive_item_t* p_item, uint16_t timeout) {

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_KEEPALIVE)
	LOGi("received keep alive over mesh:");
	BLEutil::printArray(p_item, sizeof(keep_alive_item_t));
#endif

	keep_alive_state_message_payload_t keepAlive;

	switch (p_item->actionSwitchState) {
	case 255: {
		keepAlive.action = 0;
		break;
	}
	default: {
		keepAlive.action = 1;
		keepAlive.switchState.switchState = p_item->actionSwitchState;
		break;
	}
	}

	keepAlive.timeout = timeout;

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_KEEPALIVE)
	LOGi("KeepAlive, action: %d, switchState: %d, timeout: %d",
			keepAlive.action, keepAlive.switchState.switchState, keepAlive.timeout);
#endif

	EventDispatcher::getInstance().dispatch(EVT_KEEP_ALIVE, &keepAlive, sizeof(keepAlive));
}

ERR_CODE MeshControl::handleCommand(uint16_t type, uint8_t* payload, uint16_t length) {

	switch(type) {
//	case EVENT_MESSAGE: {
//		break;
//	}
	case CONFIG_MESSAGE: {
		if (length != sizeof(config_mesh_message_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
			BLEutil::printArray(payload, length);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		config_mesh_message_t* msg = (config_mesh_message_t*)payload;
		uint8_t type = msg->type;
		uint16_t length = msg->length;
		uint8_t* payload = msg->payload;
		Settings::getInstance().writeToStorage(type, payload, length);

		return ERR_SUCCESS;
	}
	case CONTROL_MESSAGE: {
		control_mesh_message_t* msg = (control_mesh_message_t*)payload;
		CommandHandlerTypes command = (CommandHandlerTypes)msg->type;
		uint16_t length = msg->length;
		uint8_t* msgPayload = msg->payload;

		switch(command) {
		case CMD_ENABLE_SCANNER: {
			if (length != sizeof(enable_scanner_message_payload_t)) {
				LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
				BLEutil::printArray(msgPayload, length);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

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

			return ERR_SUCCESS;
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

			return ERR_SUCCESS;
		}
		default:
			break;
		}

		return CommandHandler::getInstance().handleCommand(command, msgPayload, length);

	}
	case BEACON_MESSAGE: {
#ifdef PRINT_MESHCONTROL_VERBOSE
		LOGi("Received Beacon Message");
#endif

		if (length != sizeof(beacon_mesh_message_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
			BLEutil::printArray(payload, length);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

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

		return ERR_SUCCESS;
	}
//	case STATE_MESSAGE: {
//		// should have single value
//		LOGi("Decode message: value %i", payload[0]);
//		log(SERIAL_INFO, "Decode message:");
//		BLEutil::printArray((uint8_t*)payload, 1);
//		break;
//	}
	default: {
		LOGi("Unknown message type %d. Don't know how to decode...", type);
		return ERR_UNKNOWN_MESSAGE_TYPE;
	}

	}

}

//! handle event triggered by the EventDispatcher, in case we want to send events
//! into the mesh, e.g. for power on/off
void MeshControl::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch(evt) {
	case CONFIG_CROWNSTONE_ID: {
		_myCrownstoneId = *(uint16_t*)p_data;
		break;
	}
	default:
		break;
	}
}

//! used by the mesh characteristic to send a message into the mesh
ERR_CODE MeshControl::send(uint8_t channel, void* p_data, uint8_t length) {

	switch(channel) {
	case COMMAND_CHANNEL: {

		command_message_t* message = (command_message_t*)p_data;

//		if (!isValidMessage(msg, length)) {
//			return ERR_INVALID_MESSAGE;
//		}

		// only check the "header" part, so message type, ids, and the command header. the payload length
		// will be checked in the handleCommand
		if (length < MIN_COMMAND_HEADER_SIZE + message->numOfIds * sizeof(id_type_t) + SB_HEADER_SIZE) {
			LOGe("invalid message, length too small")
			BLEutil::printArray(p_data, length);
			return ERR_INVALID_MESSAGE;
		}

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_COMMAND)
		{
		id_type_t* id = message->data.ids;
		if (message->numOfIds == 1 && *id == _myCrownstoneId) {
			LOGd("Message is only for us");
		} else if (is_broadcast_command(message)) {
			LOGd("Send broadcast and process");
		} else if (is_command_for_us(message, _myCrownstoneId)) {
			LOGd("Multicast message, send into mesh and process");
		} else {
			LOGd("Message not for us, send into mesh");
		}
		LOGi("message:");
		BLEutil::printArray((uint8_t*)p_data, length);
		}
#endif

		bool handleSelf = false;
		bool sendOverMesh = false;

		id_type_t* id = message->data.ids;
		if (message->numOfIds == 1 && *id == _myCrownstoneId) {

			// message is only for us, no need to send it into the mesh
			handleSelf = true;
			sendOverMesh = false;

		} else {
			// send over mesh
			sendOverMesh = true;
			// and check if we should handle it ourselves too
			handleSelf = is_command_for_us(message, _myCrownstoneId);
		}

		ERR_CODE errCode;
		if (handleSelf) {
			uint8_t* payload;
			uint16_t payloadLength;

			get_payload(message, length, &payload, payloadLength);
			errCode = handleCommand(message->messageType, payload, payloadLength);
		}

		if (sendOverMesh) {
			uint32_t messageCounter = Mesh::getInstance().send(channel, p_data, length);

			if (handleSelf) {
				sendStatusReplyMessage(messageCounter, errCode);
			}
		}

		break;
	}
	case KEEP_ALIVE_CHANNEL: {

		keep_alive_message_t* msg = (keep_alive_message_t*)p_data;
		keep_alive_item_t* p_item = (keep_alive_item_t*)msg->list;
		uint16_t timeout = msg->timeout;

		if (length < KEEP_ALIVE_HEADER_SIZE + msg->size * sizeof(keep_alive_item_t)) {
			LOGe("invalid message, length too small")
			BLEutil::printArray(p_data, length);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		for (int i = 0; i < msg->size; ++i) {
			if (p_item->id == _myCrownstoneId) {
				handleKeepAlive(p_item, timeout);
				break;
			}
			++p_item;
		}

		Mesh::getInstance().send(channel, p_data, length);

		break;
	}
	default: {

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

void MeshControl::sendStatusReplyMessage(uint32_t messageCounter, ERR_CODE status) {

#if defined(PRINT_MESHCONTROL_VERBOSE) && defined(PRINT_VERBOSE_COMMAND_REPLY)
	LOGd("MESH SEND");
	LOGi("Send StatusReply for message %d, status: %d", messageCounter, status);
#endif

	reply_message_t message = {};
	uint16_t messageSize;

	status_reply_item_t replyItem;
	replyItem.id = _myCrownstoneId;
	replyItem.status = status;

	Mesh::getInstance().getLastMessage(COMMAND_REPLY_CHANNEL, &message, messageSize);

	if (message.messageCounter != messageCounter) {
		memset(&message, 0, sizeof(message));
		message.messageCounter = messageCounter;
	}

	push_status_reply_item(&message, &replyItem);

#if defined(PRINT_DEBUG) &&  defined(PRINT_VERBOSE_COMMAND_REPLY)
		LOGi("message data:");
		BLEutil::printArray(&message, sizeof(reply_message_t));
#endif

	Mesh::getInstance().send(COMMAND_REPLY_CHANNEL, &message, sizeof(reply_message_t));
}

//! sends the result of a scan, i.e. a list of scanned devices with rssi values
//! into the mesh on the hub channel so that it can be synced to the cloud
void MeshControl::sendScanMessage(peripheral_device_t* p_list, uint8_t size) {

#if defined(PRINT_MESHCONTROL_VERBOSE) && defined(PRINT_VERBOSE_SCAN_RESULT)
	LOGd("MESH SEND");
	LOGi("Send ScanMessage, size: %d", size);
#endif

	//! if no devices were scanned there is no reason to send a message!
	if (size > 0) {

		scan_result_message_t message;
		message.numOfResults = size;

		scan_result_item_t* p_item;
		p_item = message.list;
		for (int i = 0; i < size; ++i) {
			p_item->id = _myCrownstoneId;
			memcpy(&p_item->address, p_list[i].addr, BLE_GAP_ADDR_LEN);
			p_item->rssi = p_list[i].rssi;
			++p_item;
		}

		int messageSize = SCAN_RESULT_HEADER_SIZE + size * sizeof(scan_result_item_t);

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_SCAN_RESULT)
		LOGi("message data:");
		BLEutil::printArray(&message, messageSize);
#endif

		Mesh::getInstance().send(SCAN_RESULT_CHANNEL, &message, messageSize);
	}

}

//void MeshControl::sendPowerSamplesMessage(power_samples_mesh_message_t* samples) {
//
//#ifdef PRINT_MESHCONTROL_VERBOSE
//	LOGd("Send PowerSamplesMessage");
//#endif
//
//	hub_mesh_message_t* message = createHubMessage(POWER_SAMPLES_MESSAGE);
//	memcpy(&message.powerSamplesMsg, samples, sizeof(power_samples_mesh_message_t));
//	uint16_t handle = (message.header.address[0] % (MESH_NUM_HANDLES-2-1)) + 3;
//	Mesh::getInstance().send(handle, message, sizeof(hub_mesh_message_t));
//	free(message);
//}

void MeshControl::sendServiceDataMessage(state_item_t& stateItem, bool event) {

//#define UPDATE_EXISTING
	bool debug = false;

#if defined(PRINT_DEBUG)
#if defined(PRINT_VERBOSE_STATE_BROADCAST) && defined(PRINT_VERBOSE_STATE_CHANGE)
	debug = true;
#elif defined(PRINT_VERBOSE_STATE_BROADCAST)
	debug = !event;
#elif defined(PRINT_VERBOSE_STATE_CHANGE)
	debug = event;
#endif

	if (debug) {
		LOGd("MESH SEND");
		LOGd("Send state %s", event ? "change" : "broadcast");

		LOGi("Crownstone Id %d", stateItem.id);
		LOGi("  switch state: %d", stateItem.switchState);
		LOGi("  power usage: %d", stateItem.powerUsage);
		LOGi("  accumulated energy: %d", stateItem.accumulatedEnergy);
	}
#endif

	uint16_t messageSize = 0;
	state_message_t message = {};
	uint8_t channel;

	if (event) {
		channel = STATE_CHANGE_CHANNEL;
	} else {
		channel = STATE_BROADCAST_CHANNEL;
	}

#ifdef UPDATE_EXISTING
	int index = 0;
	if (!Mesh::getInstance().getLastMessage(channel, &message, messageSize)) {
		message.tail = 0;
		message.head = 0;
	} else {
		bool found = false;
		for (uint8_t i = 0; i < MAX_STATE_ITEMS; ++i) {
			index = (message.head + i) % MAX_STATE_ITEMS;

			if (message.list[index].id == _myCrownstoneId) {
#if defined(PRINT_DEBUG)
				if (debug) {
					LOGi("found at index %d", index);
				}
#endif
				found = true;
				break;
			}

			if (index == message.tail) break;
		}

		if (!found) {
			message.tail = (message.tail + 1) % MAX_STATE_ITEMS;
			index = message.tail;
#if defined(PRINT_DEBUG)
			if (debug) {
				LOGd("not found, add at %d", message.tail);
			}
#endif
			if (message.tail == message.head) {
				message.head = (message.head + 1) % MAX_STATE_ITEMS;
			}
		}
	}

	memcpy(&message.list[index], &stateItem, sizeof(state_item_t));
#else

	Mesh::getInstance().getLastMessage(channel, &message, messageSize);
	push_state_item(&message, &stateItem);

#endif

#if defined(PRINT_DEBUG)
	if (debug) {
		LOGi("message data:");
		BLEutil::printArray(&message, sizeof(state_message_t));
	}
#endif

	Mesh::getInstance().send(channel, &message, sizeof(state_message_t));
}
