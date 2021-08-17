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
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_REPORT_ASSET_MAC> {
	using type = report_asset_mac_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_REPORT_ASSET_ID> {
	using type = report_asset_id_t;
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
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC> {
	using type = cs_mesh_model_msg_asset_rssi_mac_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_ASSET_RSSI_SID> {
	using type = cs_mesh_model_msg_asset_rssi_sid_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_NEIGHBOUR_RSSI> {
	using type = cs_mesh_model_msg_neighbour_rssi_t;
};

class MeshMsgEvent {
public:
	cs_mesh_model_msg_type_t type;     // Type of message
	cs_data_t msg;                     // Message payload
	bool macAddressValid;              // True when the following MAC address is valid.
	uint8_t macAddress[MAC_ADDRESS_LEN]; // MAC address of the relaying node, which is the src in case of 0 hops.
	uint16_t srcAddress;               // Address of the original sender
	uint8_t hops;                      // 0 hops means the msg was received directly from the original source.
	int8_t rssi;                       // RSSI of the last hop
	uint8_t channel;                   // Channel of the last hop
	mesh_reply_t* reply = nullptr;     // If set, a reply is expected. Set the message type, write your payload to the buffer,
	                                   // and set the data size to the size of the payload.

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
