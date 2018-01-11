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
#include <processing/cs_Switch.h>
#include <mesh/cs_Mesh.h>
#include <common/cs_Types.h>
#include <util/cs_Utils.h>

// enable for additional debug output
//#define PRINT_DEBUG
#define PRINT_MESHCONTROL_VERBOSE

#define PRINT_VERBOSE_KEEPALIVE
//#define PRINT_VERBOSE_STATE
//#define PRINT_VERBOSE_SCAN_RESULT
//#define PRINT_VERBOSE_COMMAND
#define PRINT_VERBOSE_COMMAND_REPLY
#define PRINT_VERBOSE_MULTI_SWITCH

MeshControl::MeshControl() : _myCrownstoneId(0) {
	EventDispatcher::getInstance().addListener(this);
}

void MeshControl::init() {
	Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &_myCrownstoneId);

	LOGd("Keep alive msg: size=%d items=%d", sizeof(keep_alive_message_t), KEEP_ALIVE_SAME_TIMEOUT_MAX_ITEMS);
	LOGd("State msg: size=%d items=%d", sizeof(state_message_t), MAX_STATE_ITEMS);
	LOGd("Scan result msg: size=%d items=%d", sizeof(scan_result_message_t), MAX_SCAN_RESULT_ITEMS);
	LOGd("Multi switch msg: size=%d items=%d", sizeof(multi_switch_message_t), MULTI_SWITCH_LIST_MAX_ITEMS);
}

void MeshControl::process(uint16_t channel, void* p_meshMessage, uint16_t messageLength) {

	mesh_message_t* meshMessage = (mesh_message_t*)p_meshMessage;
	//! p_data is the payload data, length is the payload length.
	uint8_t* p_data = meshMessage->payload;
	uint16_t length = messageLength - PAYLOAD_HEADER_SIZE;

#if defined(PRINT_MESHCONTROL_VERBOSE)
	LOGd("Process incoming mesh message %d", meshMessage->messageCounter);
#endif

	switch(channel) {
	case KEEP_ALIVE_CHANNEL:
		handleKeepAlive((keep_alive_message_t*)p_data, length);
		break;
	case MULTI_SWITCH_CHANNEL:
		handleMultiSwitch((multi_switch_message_t*)p_data, length);
		break;
	case COMMAND_CHANNEL:
		handleCommand((command_message_t*)p_data, length, meshMessage->messageCounter);
		break;
	case STATE_CHANNEL_0:
		handleStateMessage((state_message_t*)p_data, length, 0);
		break;
	case STATE_CHANNEL_1:
		handleStateMessage((state_message_t*)p_data, length, 1);
		break;
	case COMMAND_REPLY_CHANNEL:
		handleCommandReplyMessage((reply_message_t*)p_data, length);
		break;
	case SCAN_RESULT_CHANNEL:
		handleScanResultMessage((scan_result_message_t*)p_data, length);
		break;
	case BIG_DATA_CHANNEL:
		break;
	}
}


ERR_CODE MeshControl::handleCommand(command_message_t* msg, uint16_t length, uint32_t messageCounter) {
#if defined(PRINT_MESHCONTROL_VERBOSE)
	LOGi("handleCommand");
#endif
	//! Only check if the ids array fits, the payload length will be checked in handleCommandForUs()
//	[26-04-2017] I don't think we should be able to handle valid messages which are smaller than sizeof(command_message_t)
//	[01-05-2017] But messages that come in via the mesh characteristic can be smaller.
//	if (length < sizeof(command_message_t) || !is_valid_command_message(msg, length)) {
	if (!is_valid_command_message(msg, length)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
		BLEutil::printArray(msg, length);
//		sendStatusReplyMessage(messageCounter, ERR_WRONG_PAYLOAD_LENGTH); // Can't check bitmask, cause maybe it's not there.
		return ERR_WRONG_PAYLOAD_LENGTH;
	}


	if (is_command_for_us(msg, _myCrownstoneId)) {
#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_COMMAND)
		LOGi("is for us: ");
		BLEutil::printArray(msg, length);
#endif
		uint8_t* payload;
		uint16_t payloadLength;
		get_command_msg_payload(msg, length, &payload, payloadLength);
		handleCommandForUs(msg->messageType, msg->bitmask, messageCounter, payload, payloadLength);
	}
	else {
#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_COMMAND)
		LOGi("not for us: ");
		BLEutil::printArray(msg, length);
#endif
	}
	return ERR_SUCCESS;
}


ERR_CODE MeshControl::handleKeepAlive(keep_alive_message_t* msg, uint16_t length) {
#if defined(PRINT_MESHCONTROL_VERBOSE)
		LOGi("handleKeepAlive");
#endif
//	[26-04-2017] I don't think we should be able to receive/handle valid messages which are smaller than sizeof(keep_alive_message_t)
//	[01-05-2017] But messages that come in via the mesh characteristic can be smaller.
//	if (length < sizeof(keep_alive_message_t) || !is_valid_keep_alive_msg(msg, length)) {
	if (!is_valid_keep_alive_msg(msg, length)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
		BLEutil::printArray(msg, length);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	keep_alive_cmd_t keepAliveCmd;
	if (has_keep_alive_item(msg, _myCrownstoneId, keepAliveCmd)) {

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_KEEPALIVE)
		LOGi("received keep alive over mesh:");
		BLEutil::printArray(&keepAliveCmd, sizeof(keep_alive_cmd_t));
#endif

		keep_alive_state_message_payload_t keepAlive;

		switch (keepAliveCmd.actionSwitchState) {
		case 255: {
			keepAlive.action = NO_CHANGE;
			keepAlive.switchState.switchState = 0; //! Not necessary
			break;
		}
		default: {
			keepAlive.action = CHANGE;
			keepAlive.switchState.switchState = keepAliveCmd.actionSwitchState;
			break;
		}
		}

		keepAlive.timeout = keepAliveCmd.timeout;

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_KEEPALIVE)
		LOGi("KeepAlive, action: %d, switchState: %d, timeout: %d",
				keepAlive.action, keepAlive.switchState.switchState, keepAlive.timeout);
#endif

		EventDispatcher::getInstance().dispatch(EVT_KEEP_ALIVE, &keepAlive, sizeof(keepAlive));
	}
	else {
#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_KEEPALIVE)
		LOGi("keep alive, not for us");
		BLEutil::printArray(msg, length);
#endif
	}

	return ERR_SUCCESS;
}

ERR_CODE MeshControl::handleMultiSwitch(multi_switch_message_t* msg, uint16_t length) {
#if defined(PRINT_VERBOSE_MULTI_SWITCH)
	LOGi("handleMultiSwitch");
	BLEutil::printArray(msg, length);
#endif

//	[26-04-2017] I don't think we should be able to receive/handle valid messages which are smaller than sizeof(multi_switch_message_t)
//	[01-05-2017] But messages that come in via the mesh characteristic can be smaller.
//	if (length < sizeof(multi_switch_message_t) || !is_valid_multi_switch_message(msg, length)) {
	if (!is_valid_multi_switch_message(msg, length)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
		BLEutil::printArray(msg, length);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	multi_switch_cmd_t multiSwitchCmd;
	if (has_multi_switch_item(msg, _myCrownstoneId, multiSwitchCmd)) {

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_MULTI_SWITCH)
		LOGi("received multi switch over mesh:");
		BLEutil::printArray(&multiSwitchCmd, sizeof(multi_switch_cmd_t));
#endif

		Switch::getInstance().handleMultiSwitch(&multiSwitchCmd);

	}
	else {

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_MULTI_SWITCH)
		LOGi("multi switch, not for us");
		BLEutil::printArray(msg, length);
#endif
	}

	return ERR_SUCCESS;

}

ERR_CODE MeshControl::handleStateMessage(state_message_t* msg, uint16_t length, uint16_t stateChan) {
#ifdef PRINT_DEBUG
	LOGd("handleStateMessage");
#endif
	if (!is_valid_state_msg(msg, length)) {
		LOGe("Invalid message:");
		BLEutil::printArray(msg, length);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	switch (stateChan) {
	case 0:
		EventDispatcher::getInstance().dispatch(EVT_EXTERNAL_STATE_MSG_CHAN_0, msg, sizeof(state_message_t));
		break;
	case 1:
		EventDispatcher::getInstance().dispatch(EVT_EXTERNAL_STATE_MSG_CHAN_1, msg, sizeof(state_message_t));
		break;
	}

	if (msg->timestamp != 0) {
		// Dispatch an event with the received timestamp
		EventDispatcher::getInstance().dispatch(EVT_MESH_TIME, &(msg->timestamp), sizeof(msg->timestamp));
	}

//#ifdef PRINT_DEBUG
#ifdef PRINT_VERBOSE_STATE
	BLEutil::printArray(msg, length);

	int16_t idx = -1;
	state_item_t* stateItem;
	while (peek_next_state_item(msg, &stateItem, idx)) {
		switch (stateItem->type) {
		case MESH_STATE_ITEM_TYPE_STATE:
		case MESH_STATE_ITEM_TYPE_EVENT_STATE:
			LOGi("idx=%i type=%u id=%u switch=%u flags=%u temp=%i PF=%i P=%i E=%i time=%u", idx, stateItem->type, stateItem->state.id, stateItem->state.switchState, stateItem->state.flags, stateItem->state.temperature, stateItem->state.powerFactor, stateItem->state.powerUsageReal, stateItem->state.energyUsed, stateItem->state.partialTimestamp);
			break;
		case MESH_STATE_ITEM_TYPE_ERROR:
			LOGi("idx=%i type=%u id=%u error=%u time=%u flags=%u temp=%i time=%u", idx, stateItem->type, stateItem->error.id, stateItem->error.errors, stateItem->error.timestamp, stateItem->error.flags, stateItem->error.temperature, stateItem->error.partialTimestamp);
			break;
		default:
			LOGi("idx=%i type=%u", idx, stateItem->type);
		}
	}
#endif
	return ERR_SUCCESS;
}


ERR_CODE MeshControl::handleCommandReplyMessage(reply_message_t* msg, uint16_t length) {
	LOGi("handleCommandReplyMessage");
	BLEutil::printArray(msg, length);

	if (!is_valid_reply_msg(msg, length)) {
		LOGe("Invalid message:");
#ifdef PRINT_DEBUG
		BLEutil::printArray(msg, length);
#endif
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	switch(msg->messageType) {
	case STATUS_REPLY: {
		LOGi("Received Status Reply for Message: %d", msg->messageCounter);

#if defined(PRINT_DEBUG) and defined(PRINT_VERBOSE_COMMAND_REPLY)
		for (int i = 0; i < msg->itemCount; ++i) {
			LOGi("  ID %d: %d", msg->statusList[i].id, msg->statusList[i].status);
		}
#endif
		break;
	}
	case CONFIG_REPLY: {
		LOGi("Received Config Reply for Message: %d", msg->messageCounter);

#if defined(PRINT_DEBUG) and defined(PRINT_VERBOSE_COMMAND_REPLY)
		for (int i = 0; i < msg->itemCount; ++i) {
			config_reply_item_t* item = &msg->configList[i];
			log(SERIAL_INFO, "  ID %d: Type: %d, Data: 0x", item->id, item->data.type);
			BLEutil::printArray(item->data.payload, item->data.length);
		}
#endif
		break;
	}
	case STATE_REPLY: {
		LOGi("Received State Reply for Message: %d", msg->messageCounter);

#if defined(PRINT_DEBUG) and defined(PRINT_VERBOSE_COMMAND_REPLY)
		for (int i = 0; i < msg->itemCount; ++i) {
			state_reply_item_t* item = &msg->stateList[i];
			log(SERIAL_INFO, "  ID %d: Type: %d, Data: 0x", item->id, item->data.type);
			BLEutil::printArray(item->data.payload, item->data.length);
		}
#endif
		break;
	}
	default: {
		LOGe("Unknown reply msg type: %d", msg->messageType);
		return ERR_UNKNOWN_OP_CODE;
	}
	}
	return ERR_SUCCESS;
}

ERR_CODE MeshControl::handleScanResultMessage(scan_result_message_t* msg, uint16_t length) {
	LOGi("Received scan results");

	if (!is_valid_scan_result_message(msg, length)) {
		LOGe("Invalid message:");
#ifdef PRINT_DEBUG
		BLEutil::printArray(msg, length);
#endif
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

#if defined(PRINT_DEBUG) and defined(PRINT_VERBOSE_SCAN_RESULT)

	BLEutil::printArray(msg, length);

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
	return ERR_SUCCESS;
}


void MeshControl::handleCommandForUs(uint8_t commandType, uint8_t bitmask, uint32_t messageCounter, uint8_t* commandPayload, uint16_t commandPayloadLength) {

	ERR_CODE statusResult;

	//! TODO: use StreamBuffer class to wrap around the stream buffers.
	switch(commandType) {
	case CONFIG_MESSAGE: {
		config_mesh_message_t* configMsg = (config_mesh_message_t*)commandPayload;
		if (!handleConfigCommand(configMsg, commandPayloadLength, messageCounter, statusResult)) {
			return; // Don't send reply message.
		}
		break;
	}
	case STATE_MESSAGE: {
		state_mesh_message_t* stateMsg = (state_mesh_message_t*)commandPayload;
		if (!handleStateCommand(stateMsg, commandPayloadLength, messageCounter, statusResult)) {
			return;
		}
		break;
	}
	case CONTROL_MESSAGE: {
		control_mesh_message_t* controlMsg = (control_mesh_message_t*)commandPayload;
		if (!handleControlCommand(controlMsg, commandPayloadLength, messageCounter, statusResult)) {
			return;
		}
		break;
	}
	case BEACON_MESSAGE: {
		beacon_mesh_message_t* beaconMsg = (beacon_mesh_message_t*)commandPayload;
		if (!handleBeaconConfigCommand(beaconMsg, commandPayloadLength, messageCounter, statusResult)) {
			return;
		}
		break;
	}
	default: {
		LOGi("Unknown message type %d. Don't know how to decode...", commandType);
		statusResult = ERR_UNKNOWN_MESSAGE_TYPE;
		break;
	}

	}

	if (BLEutil::isBitSet(bitmask, REPLY_REQUEST)) {
		sendStatusReplyMessage(messageCounter, statusResult);
	}

}


bool MeshControl::handleConfigCommand(config_mesh_message_t* configMsg, uint16_t configMsgLength, uint32_t messageCounter, ERR_CODE& statusResult) {
	//! Before reading the header, check if the header fits in the msg
	if (configMsgLength < SB_HEADER_SIZE) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, configMsgLength);
		BLEutil::printArray(configMsg, configMsgLength);
		statusResult = ERR_WRONG_PAYLOAD_LENGTH;
		return true;
	}
	uint8_t configType = configMsg->type;
	uint16_t configPayloadlength = configMsg->length;
	uint8_t* configPayload = configMsg->payload;

	//! Check if the length given in the stream buffer actually fits in the message.
	//! This is actually only needed for the WRITE_VALUE case
	if (configMsgLength < SB_HEADER_SIZE + configPayloadlength) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, configMsgLength);
		BLEutil::printArray(configMsg, configMsgLength);
		statusResult = ERR_WRONG_PAYLOAD_LENGTH;
		return true;
	}

	switch (configMsg->opCode) {
	case READ_VALUE: {
		config_reply_item_t configReply = {};
		configReply.id = _myCrownstoneId;
		configReply.data.opCode = READ_VALUE;
		configReply.data.type = configType;
		configReply.data.length = Settings::getInstance().getSettingsItemSize(configType);

		if (configReply.data.length > sizeof(configReply.data.payload)) {
			statusResult = ERR_BUFFER_TOO_SMALL;
		} else {
			statusResult = Settings::getInstance().get(configType, configReply.data.payload);
			if (statusResult == ERR_SUCCESS) {

				sendConfigReplyMessage(messageCounter, &configReply);
				//! Call return to skip sending status reply (since we send the config reply)
				return false;
			}
		}
		return true;
	}
	case WRITE_VALUE: {
		Settings::getInstance().writeToStorage(configType, configPayload, configPayloadlength);
		statusResult = ERR_SUCCESS;
		return true;
	}
	default: {
		statusResult = ERR_WRONG_PARAMETER;
		return true;
	}
	}
}

bool MeshControl::handleStateCommand(state_mesh_message_t* stateMsg, uint16_t stateMsgLength, uint32_t messageCounter, ERR_CODE& statusResult) {
	//! Before reading the header, check if the header fits in the msg
	if (stateMsgLength < SB_HEADER_SIZE) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, stateMsgLength);
		BLEutil::printArray(stateMsg, stateMsgLength);
		statusResult = ERR_WRONG_PAYLOAD_LENGTH;
		return true;
	}
	uint8_t stateType = stateMsg->type;

	//! Only the type and opcode seem to be used, not the stream buffer payload.
	//! So no need to check the length field.

	switch (stateMsg->opCode) {
	case READ_VALUE: {
		state_reply_item_t stateReply = {};
		stateReply.id = _myCrownstoneId;
		stateReply.data.opCode = READ_VALUE;
		stateReply.data.type = stateType;
		stateReply.data.length = State::getInstance().getStateItemSize(stateType);

		if (stateReply.data.length > sizeof(stateReply.data.payload)) {
			statusResult = ERR_BUFFER_TOO_SMALL;
		}
		else {
			statusResult = State::getInstance().get(stateType, stateReply.data.payload, stateReply.data.length);
			if (statusResult == ERR_SUCCESS) {
				sendStateReplyMessage(messageCounter, &stateReply);
				//! Already send a state reply, no need to send a status reply
				return false;
			}
		}
		return true;
	}
	default: {
		statusResult = ERR_WRONG_PARAMETER;
		return true;
	}
	}
	return true;
}

bool MeshControl::handleControlCommand(control_mesh_message_t* controlMsg, uint16_t controlMsgLength, uint32_t messageCounter, ERR_CODE& statusResult) {
	//! Before reading the header, check if the header fits in the msg
	if (controlMsgLength < SB_HEADER_SIZE) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, controlMsgLength);
		BLEutil::printArray(controlMsg, controlMsgLength);
		statusResult = ERR_WRONG_PAYLOAD_LENGTH;
		return true;
	}

	CommandHandlerTypes controlType = (CommandHandlerTypes)controlMsg->type;
	uint16_t controlPayloadLength = controlMsg->length;
	uint8_t* controlPayload = controlMsg->payload;

	//! Check if the length given in the stream buffer actually fits in the message.
	//! This is actually only needed for the WRITE_VALUE case
	if (controlMsgLength < SB_HEADER_SIZE + controlPayloadLength) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, controlMsgLength);
		BLEutil::printArray(controlMsg, controlMsgLength);
		statusResult = ERR_WRONG_PAYLOAD_LENGTH;
		return true;
	}

	switch(controlType) {
	case CMD_ENABLE_SCANNER: {
		if (controlPayloadLength < sizeof(enable_scanner_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, controlPayloadLength);
			BLEutil::printArray(controlPayload, controlPayloadLength);
			statusResult = ERR_WRONG_PAYLOAD_LENGTH;
			return true;
		}

		//! need to use a random delay for starting the scanner, otherwise
		//! the devices in the mesh will start scanning at the same time
		//! resulting in lots of conflicts
		RNG rng;
		enable_scanner_message_payload_t* pl = (enable_scanner_message_payload_t*)controlPayload;
		// todo: can't edit the delay on the original payload, otherwise the mesh goes crazy with
		//   conflicting values. so for now we make a local copy
		//! TODO: is this still the case? isn't the data a copy already? or only when encryption is enabled?
		//!       Also: how is this a copy?
		enable_scanner_message_payload_t scannerPayload = *pl;

		uint16_t crownstoneId;
		Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &crownstoneId);
		if (crownstoneId != 0) {
			scannerPayload.delay = crownstoneId * 1000;
		} else {
			RNG rng;
			scannerPayload.delay = rng.getRandom16() / 1; //! Delay in ms (about 0-60 seconds)
		}

		statusResult = CommandHandler::getInstance().handleCommand(controlType, (uint8_t*)&scannerPayload, sizeof(enable_scanner_message_payload_t));
		return true;
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
			delay = rng.getRandom16() / 6; //! Delay in ms (about 0-10 seconds)
		}
		statusResult = CommandHandler::getInstance().handleCommandDelayed(controlType, controlPayload, controlPayloadLength, delay);
		return true;
	}
	default: {
		statusResult = CommandHandler::getInstance().handleCommand(controlType, controlPayload, controlPayloadLength);
		return true;
	}
	}
}

bool MeshControl::handleBeaconConfigCommand(beacon_mesh_message_t* beaconMsg, uint16_t beaconMsgLength, uint32_t messageCounter, ERR_CODE& statusResult) {
	if (beaconMsgLength < sizeof(beacon_mesh_message_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, beaconMsgLength);
		BLEutil::printArray(beaconMsg, beaconMsgLength);
		statusResult = ERR_WRONG_PAYLOAD_LENGTH;
		return true;
	}

	uint16_t major = beaconMsg->major;
	uint16_t minor = beaconMsg->minor;
	ble_uuid128_t& uuid = beaconMsg->uuid;
	int8_t& rssi = beaconMsg->txPower;

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

	statusResult = ERR_SUCCESS;
	return true;
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

//! Used by the mesh characteristic to send a message into the mesh
ERR_CODE MeshControl::send(uint16_t channel, void* p_data, uint16_t length) {

	switch(channel) {
	case COMMAND_CHANNEL: {

		command_message_t* message = (command_message_t*)p_data;

		//! Only check if the ids array fits, the payload length will be checked in handleCommandForUs()
		if (!is_valid_command_message(message, length)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
			BLEutil::printArray(p_data, length);
			return ERR_INVALID_MESSAGE;
		}

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_COMMAND)
		{
		stone_id_t* id = message->data.ids;
		if (message->idCount == 1 && *id == _myCrownstoneId) {
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

		bool handleSelf = is_command_for_us(message, _myCrownstoneId);

		//! If the only id in there is for this crownstone, there is no need to send it into the mesh.
		bool sendOverMesh = is_broadcast_command(message);
		if (message->idCount == 1 && handleSelf) {
			sendOverMesh = false;
		}
		else if (message->idCount > 0) {
			sendOverMesh = true;
		}

		uint32_t messageCounter = 0;

		if (sendOverMesh) {
			messageCounter = Mesh::getInstance().send(channel, p_data, length);
		}
		else {
			// TODO: get and inc message counter?????
		}

		if (handleSelf) {
			uint8_t* payload;
			uint16_t payloadLength;

			get_command_msg_payload(message, length, &payload, payloadLength);
			// TODO: this will send a reply message with replyMsg.messageCounter = 0, will that bring any trouble?
			handleCommandForUs(message->messageType, message->bitmask, messageCounter, payload, payloadLength);
		}
		break;
	}
	case KEEP_ALIVE_CHANNEL: {
		keep_alive_message_t* msg = (keep_alive_message_t*)p_data;

		ERR_CODE errCode;
		errCode = handleKeepAlive(msg, length);
		if (errCode != ERR_SUCCESS) {
			return errCode;
		}

		Mesh::getInstance().send(channel, p_data, length);
		break;
	}
	case MULTI_SWITCH_CHANNEL: {

		multi_switch_message_t* msg = (multi_switch_message_t*)p_data;
		ERR_CODE errCode;
		errCode = handleMultiSwitch(msg, length);
		if (errCode != ERR_SUCCESS) {
			return errCode;
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
	uint16_t messageSize = sizeof(message);
	message.messageType = STATUS_REPLY;

	status_reply_item_t replyItem;
	replyItem.id = _myCrownstoneId;
	replyItem.status = status;

	bool success = Mesh::getInstance().getLastMessage(COMMAND_REPLY_CHANNEL, &message, messageSize);
	if (!success || message.messageCounter != messageCounter || message.messageType != STATUS_REPLY || !is_valid_reply_msg(&message)) {
		cear_reply_msg(&message);
		message.messageType = STATUS_REPLY;
		message.messageCounter = messageCounter;
	}

	push_status_reply_item(&message, &replyItem);

#if defined(PRINT_DEBUG) &&  defined(PRINT_VERBOSE_COMMAND_REPLY)
		LOGi("message data:");
		BLEutil::printArray(&message, sizeof(reply_message_t));
#endif

	Mesh::getInstance().send(COMMAND_REPLY_CHANNEL, &message, sizeof(reply_message_t));
}

void MeshControl::sendConfigReplyMessage(uint32_t messageCounter, config_reply_item_t* configReply) {

#if defined(PRINT_MESHCONTROL_VERBOSE) && defined(PRINT_VERBOSE_COMMAND_REPLY)
	LOGd("MESH SEND");
//	LOGi("Send StatusReply for message %d, status: %d", messageCounter, status);
#endif

	reply_message_t message = {};
	message.messageType = CONFIG_REPLY;
	message.messageCounter = messageCounter;
	message.itemCount = 1;
	memcpy(&message.configList[0], configReply, sizeof(config_reply_item_t));

#if defined(PRINT_DEBUG) &&  defined(PRINT_VERBOSE_COMMAND_REPLY)
		LOGi("message data:");
		BLEutil::printArray(&message, sizeof(reply_message_t));
#endif

	Mesh::getInstance().send(COMMAND_REPLY_CHANNEL, &message, sizeof(reply_message_t));
}


void MeshControl::sendStateReplyMessage(uint32_t messageCounter, state_reply_item_t* stateReply) {

#if defined(PRINT_MESHCONTROL_VERBOSE) && defined(PRINT_VERBOSE_COMMAND_REPLY)
	LOGd("MESH SEND");
//	LOGi("Send StatusReply for message %d, status: %d", messageCounter, status);
#endif

	reply_message_t message = {};
	message.messageType = STATE_REPLY;
	message.messageCounter = messageCounter;
	message.itemCount = 1;
	memcpy(&message.stateList[0], stateReply, sizeof(state_reply_item_t));

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
//	uint16_t handle = (message.header.address[0] % (MESH_HANDLE_COUNT-2-1)) + 3;
//	Mesh::getInstance().send(handle, message, sizeof(hub_mesh_message_t));
//	free(message);
//}

void MeshControl::sendServiceDataMessage(state_item_t& stateItem, bool event) {
#ifdef PRINT_VERBOSE_STATE
	LOGi("Send state event=%u", event);
	switch (stateItem.type) {
	case MESH_STATE_ITEM_TYPE_STATE:
	case MESH_STATE_ITEM_TYPE_EVENT_STATE:
		LOGi("  type=%u id=%u switch=%u flags=%u temp=%i PF=%i P=%i E=%i time=%u", stateItem.type, stateItem.state.id, stateItem.state.switchState, stateItem.state.flags, stateItem.state.temperature, stateItem.state.powerFactor, stateItem.state.powerUsageReal, stateItem.state.energyUsed, stateItem.state.partialTimestamp);
		break;
	case MESH_STATE_ITEM_TYPE_ERROR:
		LOGi("  type=%u id=%u error=%u time=%u flags=%u temp=%i time=%u", stateItem.type, stateItem.error.id, stateItem.error.errors, stateItem.error.timestamp, stateItem.error.flags, stateItem.error.temperature, stateItem.error.partialTimestamp);
		break;
	default:
		LOGi("  type=%u", stateItem.type);
	}
#endif

	state_message_t message = {};
	uint16_t messageSize = sizeof(message);
	uint8_t channel = STATE_CHANNEL_0;

	// Use a "random" channel, but always the same channel!
	// Otherwise, the state of this crownstone is on different channels,
	// which leads to not knowing which state is the newest.
	if (_myCrownstoneId % MESH_STATE_HANDLE_COUNT) {
		channel = STATE_CHANNEL_1;
	}

	// Append state to message, regardless of whether the state of this crownstone is already in there.
	// This way, states of crownstones that are no longer in the mesh, will be pushed out of the message.
	bool success = Mesh::getInstance().getLastMessage(channel, &message, messageSize);
	if (!success || !is_valid_state_msg(&message)) {
		clear_state_msg(&message);
	}

	push_state_item(&message, &stateItem);

	// Set the timestamp to the current time
	uint32_t timestamp;
	if (State::getInstance().get(STATE_TIME, timestamp) != ERR_SUCCESS) {
		timestamp = 0;
	}
	message.timestamp = timestamp;


#ifdef PRINT_VERBOSE_STATE
		LOGi("message data:");
		BLEutil::printArray(&message, sizeof(state_message_t));
#endif

	Mesh::getInstance().send(channel, &message, sizeof(state_message_t));
}

ERR_CODE MeshControl::sendKeepAliveMessage(keep_alive_message_t* msg, uint16_t length) {
	ERR_CODE errCode;

	if (!is_valid_keep_alive_msg(msg, length)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
		BLEutil::printArray(msg, length);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	// TODO: only send when this message is (also) for other crownstones.
	if (Mesh::getInstance().send(KEEP_ALIVE_CHANNEL, msg, length) == 0) {
		return ERR_NOT_AVAILABLE;
	}

	// Handle message after sending it, else the message gets delayed too much.
	errCode = handleKeepAlive(msg, length);
	if (errCode != ERR_SUCCESS) {
		return errCode;
	}

	return ERR_SUCCESS;
}

ERR_CODE MeshControl::sendLastKeepAliveMessage() {
	LOGi("repeat last keep alive");

	// TODO: do we have to make a copy here?
	keep_alive_message_t msg = {};
	uint16_t length = sizeof(msg);

	if (!Mesh::getInstance().getLastMessage(KEEP_ALIVE_CHANNEL, &msg, length)) {
		return ERR_NOT_AVAILABLE;
	}

	if (!is_valid_keep_alive_msg(&msg, length)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
		BLEutil::printArray(&msg, length);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	// TODO: only send when this message is (also) for other crownstones.
	if (Mesh::getInstance().send(KEEP_ALIVE_CHANNEL, &msg, length) == 0) {
		return ERR_NOT_AVAILABLE;
	}

	// Handle message after sending it, else the message gets delayed too much.
	ERR_CODE errCode;
	errCode = handleKeepAlive(&msg, length);
	if (errCode != ERR_SUCCESS) {
		return errCode;
	}

	return ERR_SUCCESS;
}

ERR_CODE MeshControl::sendMultiSwitchMessage(multi_switch_message_t* msg, uint16_t length) {
	ERR_CODE errCode;
	if (!is_valid_multi_switch_message(msg, length)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
		BLEutil::printArray(msg, length);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	// TODO: only send when this message is (also) for other crownstones.
	if (Mesh::getInstance().send(MULTI_SWITCH_CHANNEL, msg, length) == 0) {
		return ERR_NOT_AVAILABLE;
	}

	// Handle message after sending it, else the message gets delayed too much.
	errCode = handleMultiSwitch(msg, length);
	if (errCode != ERR_SUCCESS) {
		return errCode;
	}

	return ERR_SUCCESS;
}

ERR_CODE MeshControl::sendCommandMessage(command_message_t* msg, uint16_t length) {

	// Only checks if the ids array fits, the payload length will be checked in handleCommandForUs()
	if (!is_valid_command_message(msg, length)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
		BLEutil::printArray(msg, length);
		return ERR_INVALID_MESSAGE;
	}

#if defined(PRINT_DEBUG) && defined(PRINT_VERBOSE_COMMAND)
	{
	stone_id_t* id = msg->data.ids;
	if (msg->idCount == 1 && *id == _myCrownstoneId) {
		LOGd("Message is only for us");
	}
	else if (is_broadcast_command(msg)) {
		LOGd("Send broadcast and process");
	}
	else if (is_command_for_us(msg, _myCrownstoneId)) {
		LOGd("Multicast message, send into mesh and process");
	}
	else {
		LOGd("Message not for us, send into mesh");
	}
	LOGi("message:");
	BLEutil::printArray((uint8_t*)msg, length);
	}
#endif

	bool handleSelf = is_command_for_us(msg, _myCrownstoneId);

	// If the only id in the message is for this crownstone, there is no need to send it into the mesh.
	bool sendOverMesh = is_broadcast_command(msg);
	if (msg->idCount == 1 && handleSelf) {
		sendOverMesh = false;
	}
	else if (msg->idCount > 0) {
		sendOverMesh = true;
	}

	uint32_t messageCounter = 0;
	if (sendOverMesh) {
		// TODO: what if send() returns 0?
		messageCounter = Mesh::getInstance().send(COMMAND_CHANNEL, msg, length);
	}
	else {
		// TODO: get and inc message counter?????
		// Make sure there is no reply?
		BLEutil::clearBit(msg->bitmask, REPLY_REQUEST);
	}

	if (handleSelf) {
		uint8_t* payload;
		uint16_t payloadLength;

		get_command_msg_payload(msg, length, &payload, payloadLength);
		// TODO: this can send a reply message with replyMsg.messageCounter = 0, will that bring any trouble?
		handleCommandForUs(msg->messageType, msg->bitmask, messageCounter, payload, payloadLength);
	}
	return ERR_SUCCESS;
}

bool MeshControl::getLastStateDataMessage(state_message_t& message, uint16_t size, uint8_t stateChan) {
	// TODO: check if message is valid.
	mesh_handle_t handle;
	switch(stateChan) {
	case 0:
		handle = STATE_CHANNEL_0;
		break;
	case 1:
		handle = STATE_CHANNEL_1;
		break;
	default:
		return false;
	}
	return Mesh::getInstance().getLastMessage(handle, &message, size);
}
