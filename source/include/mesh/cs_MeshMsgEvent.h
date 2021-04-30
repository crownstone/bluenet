#include <structs/cs_PacketsInternal.h>

#include <cstdint>

#pragma once


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
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_NEAREST_WITNESS_REPORT> {
	using type = nearest_witness_report_t;
};

template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_STONE_MAC> {
	using type = cs_mesh_model_msg_stone_mac_t;
};


class MeshMsgEvent{
public:
	cs_data_t msg;
	cs_mesh_model_msg_type_t type;
	uint16_t srcAddress; // of the original sender
	int8_t rssi; // of the last hop
	uint8_t hops; // 0 hops means the msg was received directly from the original source.
	uint8_t channel;

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
	auto& getPacket(){
		return *reinterpret_cast<
				typename MeshPacketTraits<MeshModelType>::type*>(
						msg.data);
	}
};
