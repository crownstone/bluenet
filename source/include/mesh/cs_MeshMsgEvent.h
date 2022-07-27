#include <structs/cs_PacketsInternal.h>

#include <cstdint>

#pragma once

struct mesh_reply_t {
	cs_mesh_model_msg_type_t type;     // Type of message.
	cs_data_t buf;                     // Buffer to write payload to.
	uint8_t dataSize;                  // Size of the payload.
};

template<cs_mesh_model_msg_type_t E>
struct MeshPacketTraits;

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_TEST> {
	using type = cs_mesh_model_msg_test_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_RSSI_PING> {
	using type = rssi_ping_message_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_RSSI_DATA> {
	using type = rssi_data_message_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_TIME_SYNC> {
	using type = cs_mesh_model_msg_time_sync_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_STONE_MAC> {
	using type = cs_mesh_model_msg_stone_mac_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_ASSET_FILTER_VERSION> {
	using type = cs_mesh_model_msg_asset_filter_version_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_ASSET_INFO_MAC> {
	using type = cs_mesh_model_msg_asset_report_mac_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_ASSET_INFO_ID> {
	using type = cs_mesh_model_msg_asset_report_id_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI> {
	using type = cs_mesh_model_msg_neighbour_rssi_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS> {
	using type = behaviour_settings_t;
};



class MeshMsgEvent {
public:
	cs_mesh_model_msg_type_t type;       // Message type.
	cs_data_t msg;                       // Message payload.

	stone_id_t srcStoneId;               // Stone ID of the original sender.
	bool macAddressValid;                // True when the following MAC address is valid.
	uint8_t macAddress[MAC_ADDRESS_LEN]; // MAC address of the node from which this message was received, which is the source if not relayed.
	int8_t rssi;                         // RSSI of the last hop, thus the RSSI to the source if not relayed.
	uint8_t channel;                     // Channel of the last hop.

	bool isMaybeRelayed;                 // Whether this message may have been relayed.
	bool isReply;                        // Whether this message is a reply.

	// When not CTRL_CMD_UNKNOWN and this message is a reply, this is the control command that caused the message to be sent that this message is a reply to.
	cs_control_cmd_t controlCommand = CTRL_CMD_UNKNOWN;

	mesh_reply_t* reply = nullptr;       // If set, a reply is expected. Set the message type, write your payload to the buffer,
	                                     // and set the data size to the size of the payload.

	cs_mesh_model_opcode_t opCode;       // For debug prints only.
	uint8_t ttl;                         // For debug prints only.


	/**
	 * Returns the message data of this event as the original
	 * mesh model type, by reference.
	 *
	 * E.g.
	 *
	 * MeshMessageEvent e (.. constructor args ..);
	 *
	 * .. send over event bus ..
	 *
	 * auto sync_packet = e.getPacket<CS_MESH_MODEL_TYPE_TIME_SYNC>();
	 * printf("%d", sync_packet.sender);
	 */
	template<cs_mesh_model_msg_type_t MeshModelType>
	auto& getPacket() {
		return *reinterpret_cast<typename MeshPacketTraits<MeshModelType>::type*>(msg.data);
	}
};
