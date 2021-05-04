/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 30, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <events/cs_EventListener.h>
#include <protocol/cs_MeshTopologyPackets.h>
#include <structs/buffer/cs_CircularBuffer.h>

#if BUILD_MESH_TOPOLOGY_RESEARCH == 1
#include <localisation/cs_MeshTopologyResearch.h>
#endif

class MeshTopology: EventListener {
public:
	static constexpr uint8_t MAX_NEIGHBOURS = 50;
	static constexpr uint8_t TIMEOUT_SECONDS = 50;
	static constexpr uint8_t INDEX_NOT_FOUND = 0xFF;

	cs_ret_code_t init();

	cs_ret_code_t getMacAddress(stone_id_t stoneId);

private:
	stone_id_t _myId = 0;
	neighbour_node_t* _neighbours = nullptr;
	uint8_t _neighbourCount = 0;

	void add(stone_id_t id, int8_t rssi, uint8_t channel);
	uint8_t find(stone_id_t id);

	cs_ret_code_t onStoneMacMsg(stone_id_t id, cs_mesh_model_msg_stone_mac_t& packet, mesh_reply_t* result);
	void onMeshMsg(MeshMsgEvent& packet, cs_result_t& result);
	void onTickSecond();

	uint8_t compressRssi(int8_t rssi);
	uint8_t compressChannel(uint8_t channel);

	void print();

public:
	void handleEvent(event_t &evt);

#if BUILD_MESH_TOPOLOGY_RESEARCH == 1
private:
	MeshTopologyResearch _research;
#endif
};


