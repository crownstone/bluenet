/**
 * Author: Dominik Egger
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <protocol/cs_MeshMessageTypes.h>
#include <storage/cs_Settings.h>

/** Wrapper around meshing functionality.
 *
 */
class MeshControl : public EventListener {

public:
	//! use static variant of singleton, no dynamic memory allocation
	static MeshControl& getInstance() {
		static MeshControl instance;
		return instance;
	}

	/** Send a message with the scanned devices over the mesh
	 *
	 * @p_list pointer to the list of scanned devices
	 * @size number of scanned devices
	 */
	void sendScanMessage(peripheral_device_t* p_list, uint8_t size);

//	void sendPowerSamplesMessage(power_samples_mesh_message_t* samples);

	void sendServiceDataMessage(state_item_t& stateItem, bool event);

	void sendStatusReplyMessage(uint32_t messageCounter, ERR_CODE status);
	void sendConfigReplyMessage(uint32_t messageCounter, config_reply_item_t* configReply);
	void sendStateReplyMessage(uint32_t messageCounter, state_reply_item_t* configReply);

	/** Send a message into the mesh
	 *
	 * @channel the channel number, see <MeshChannels>
	 * @p_data a pointer to the data which should be sent
	 * @length number of bytes of data to with p_data points
	 */
	ERR_CODE send(uint8_t channel, void* p_data, uint8_t length);

	/**
	 * Get incoming messages and perform certain actions.
	 * @channel the channel number, see <MeshChannels>
	 * @p_data a pointer to the data which was received
	 * @length number of bytes received
	 */
	void process(uint8_t channel, void* p_data, uint16_t length);

	void init();

protected:

	/** Handle events dispatched through the EventDispatcher
	 *
	 * @evt the event type, can be one of <GeneralEventType>, <ConfigurationTypes>, <StateVarTypes>
	 * @p_data pointer to the data which was sent with the dispatch, can be NULL
	 * @length number of bytes of data provided
	 */
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	/** Decode a received mesh data message
	 *
	 * @msg pointer to the message data
	 */
	void handleCommand(uint16_t type, uint32_t messageCounter, uint8_t* payload, uint16_t length);

	void handleKeepAlive(keep_alive_item_t* p_item, uint16_t timeout);

	/** Check if a message is valid
	 * Checks the length parameter based on the type of the mesh message
	 *
	 * @p_data pointer to the mesh message
	 * @length number of bytes received
	 */

//	bool isValidMessage(mesh_message_t* msg, uint16_t length) {
//
//		switch (msg->header.messageType) {
//		case CONTROL_MESSAGE:
//		case CONFIG_MESSAGE: {
//			//! command and config message have an array for parameters which don't have to be filled
//			//! so we don't know in advance how long it needs to be exactly. can only give
//			//! a lower and upper bound.
//			uint16_t desiredLength = getMessageSize(msg->header.messageType);
//			bool valid = length >= desiredLength && length <= MAX_MESH_MESSAGE_DATA_LENGTH;
//			if (!valid) {
//				LOGe("invalid message, length: %d <= %d", length, desiredLength);
//			}
//			return valid;
//		}
//		default:{
//			uint16_t desiredLength = getMessageSize(msg->header.messageType);
//			bool valid = length == desiredLength;
//
//			if (!valid) {
//				LOGe("invalid message, length: %d != %d", length, desiredLength);
//			}
//			return valid;
//		}
//		}
//	}

	/** Returns the number of bytes required for a mesh message of the type
	 *
	 * @messageType type of mesh message, see <MeshMessageTypes>
	 */
//	uint16_t getMessageSize(uint16_t messageType) {
//		switch(messageType) {
//		case EVENT_MESSAGE:
//			return sizeof(mesh_header_t) + sizeof(event_mesh_message_t);
//		case BEACON_MESSAGE:
//			return sizeof(mesh_header_t) + sizeof(beacon_mesh_message_t);
//		case CONTROL_MESSAGE:
//		case CONFIG_MESSAGE:
//			return sizeof(mesh_header_t) + SB_HEADER_SIZE;
//		case STATE_MESSAGE:
//			return sizeof(mesh_message_t);
//		default:
//			return 0;
//		}
//	}

////#ifdef VERSION_V2
////	hub_mesh_message_t* createHubMessage(uint16_t messageType) {
////		hub_mesh_message_t* message = new hub_mesh_message_t();
////		memset(message, 0, sizeof(hub_mesh_message_t));
////    	Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &message->header.crownstoneId);
////		message->header.messageType = messageType;
////		return message;
////	}
////#else
//	hub_mesh_message_t* createHubMessage(uint16_t messageType) {
//		hub_mesh_message_t* message = new hub_mesh_message_t();
//		memset(message, 0, sizeof(hub_mesh_message_t));
//		memcpy(&message->header.address, &_myAddr.addr, BLE_GAP_ADDR_LEN);
//		message->header.messageType = messageType;
//		return message;
//	}
////#endif
//
//	std::string getAddress(mesh_message_t* msg) {
//		if (isNewVersion(msg)) {
//			mesh_header_v2_t* header = (mesh_header_v2_t*)&msg->header;
//			char buffer[32];
//			sprintf(buffer, "%d", header->crownstoneId);
//			return std::string(buffer);
//		} else {
//			mesh_header_t* header = &msg->header;
//			char buffer[32];
//			sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", header->address[5],
//						header->address[4], header->address[3], header->address[2],
//						header->address[1], header->address[0]);
//			return std::string(buffer);
//		}
//	}
//
//	// todo for the time being, as long as we don't user reason or userId, we can
//	// continue with both versions to target crownstones. but this needs to be removed
//	// in the final version for only one of the two cases, either crownstoneId or
//	// mac address!
//	bool isNewVersion(mesh_message_t* msg) {
//		mesh_header_v2_t* header = (mesh_header_v2_t*)&msg->header;
//		return (header->reason | header->userId) == 0;
//	}

private:
	MeshControl();

	MeshControl(MeshControl const&); //! singleton, deny implementation
	void operator=(MeshControl const &); //! singleton, deny implementation

    id_type_t _myCrownstoneId;

};
