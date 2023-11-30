#include <structs/cs_PacketsInternal.h>

#include <cstdint>

#pragma once

struct mesh_reply_t {
	//! Type of message.
	cs_mesh_model_msg_type_t type;

	//! Buffer you can write your payload to (don't change the buf.len).
	cs_data_t buf;

	//! Size of your payload.
	uint8_t dataSize;
};

template <cs_mesh_model_msg_type_t E>
struct MeshPacketTraits;

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_TEST> {
	using type = cs_mesh_model_msg_test_t;
};

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_RSSI_PING> {
	using type = rssi_ping_message_t;
};

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_RSSI_DATA> {
	using type = rssi_data_message_t;
};

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_TIME_SYNC> {
	using type = cs_mesh_model_msg_time_sync_t;
};

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_STONE_MAC> {
	using type = cs_mesh_model_msg_stone_mac_t;
};

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_ASSET_FILTER_VERSION> {
	using type = cs_mesh_model_msg_asset_filter_version_t;
};

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_ASSET_INFO_MAC> {
	using type = cs_mesh_model_msg_asset_report_mac_t;
};

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_ASSET_INFO_ID> {
	using type = cs_mesh_model_msg_asset_report_id_t;
};

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI> {
	using type = cs_mesh_model_msg_neighbour_rssi_t;
};

template <>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_SET_BEHAVIOUR_SETTINGS> {
	using type = behaviour_settings_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_NODE_REQUEST> {
	using type = cs_mesh_model_msg_node_request_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_ALTITUDE_REQUEST> {
	using type = cs_mesh_model_msg_altitude_request_t;
};


class MeshMsgEvent {
public:
	//! Message type.
	cs_mesh_model_msg_type_t type;

	//! Message payload.
	cs_data_t msg;

	//! Stone ID of the original sender.
	stone_id_t srcStoneId;

	//! True when the following MAC address is valid.
	bool macAddressValid;

	//! MAC address of the node from which this message was received, which is the source if not relayed.
	uint8_t macAddress[MAC_ADDRESS_LEN];

	//! RSSI of the last hop, thus the RSSI to the source if not relayed.
	int8_t rssi;

	//! Channel of the last hop.
	uint8_t channel;

	//! Whether this message may have been relayed.
	bool isMaybeRelayed;

	//! Whether this message is a reply.
	bool isReply;

	//! When not CTRL_CMD_UNKNOWN and this message is a reply, this is the control command that caused the message to be
	//! sent that this message is a reply to.
	cs_control_cmd_t controlCommand = CTRL_CMD_UNKNOWN;

	/**
	 * If set, a reply is expected. Set the message type, write your payload to the buffer,
	 * and set the data size to the size of the payload.
	 */
	mesh_reply_t* reply             = nullptr;

	//! Mesh model opcode: internal usage only (debug).
	cs_mesh_model_opcode_t opCode;

	//! Current TTL: internal usage only (debug).
	uint8_t ttl;

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
	template <cs_mesh_model_msg_type_t MeshModelType>
	auto& getPacket() {
		return *reinterpret_cast<typename MeshPacketTraits<MeshModelType>::type*>(msg.data);
	}
};
