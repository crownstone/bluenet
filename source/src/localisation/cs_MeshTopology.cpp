/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 30, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_MeshTopology.h>
#include <ble/cs_Nordic.h>

#define LOGMeshTopologyInfo  LOGi
#define LOGMeshTopologyDebug LOGd

cs_ret_code_t MeshTopology::init() {

	_neighbours = new (std::nothrow) neighbour_node_t[MAX_NEIGHBOURS];
	if (_neighbours == nullptr) {
		return ERR_NO_SPACE;
	}
	listen();

#if BUILD_MESH_TOPOLOGY_RESEARCH == 1
	_research.init();
#endif

	return ERR_SUCCESS;
}


cs_ret_code_t MeshTopology::getMacAddress(stone_id_t stoneId) {
	LOGMeshTopologyInfo("getMacAddress %u", stoneId);
	uint8_t index = find(stoneId);
	if (index == INDEX_NOT_FOUND) {
		LOGMeshTopologyInfo("Not a neighbour");
		return ERR_NOT_FOUND;
	}

	cs_mesh_model_msg_stone_mac_t request;
	request.type = 0;

	cs_mesh_msg_t meshMsg;
	meshMsg.type = CS_MESH_MODEL_TYPE_STONE_MAC;
	meshMsg.reliability = CS_MESH_RELIABILITY_MEDIUM;
	meshMsg.urgency = CS_MESH_URGENCY_LOW;
	meshMsg.payload = reinterpret_cast<uint8_t*>(&request);
	meshMsg.size = sizeof(request);

	event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));

	event.dispatch();
	LOGMeshTopologyInfo("Sent mesh msg retCode=%u", event.result.returnCode);
	return ERR_WAIT_FOR_SUCCESS;
}


void MeshTopology::add(stone_id_t id, int8_t rssi, uint8_t channel) {
	uint8_t compressedRssi = compressRssi(rssi);
	uint8_t compressedChannel = compressChannel(channel);
	uint8_t index = find(id);
	if (index == INDEX_NOT_FOUND) {
		_neighbours[_neighbourCount] = neighbour_node_t(id, compressedRssi, compressedChannel);
		_neighbourCount++;
	}
	else {
		_neighbours[index].rssi = compressedRssi;
		_neighbours[index].channel = compressedChannel;
		_neighbours[index].lastSeenSeconds = 0;
	}
}

uint8_t MeshTopology::find(stone_id_t id) {
	for (uint8_t index = 0; index < _neighbourCount; ++index) {
		if (_neighbours[index].id == id) {
			return index;
		}
	}
	return INDEX_NOT_FOUND;
}

void MeshTopology::onStoneMacMsg(stone_id_t id, cs_mesh_model_msg_stone_mac_t& packet) {
	switch (packet.type) {
		case 0: {
			LOGMeshTopologyInfo("Send mac address");
			// Send MAC address
			cs_mesh_model_msg_stone_mac_t reply;
			reply.type = 1;

			ble_gap_addr_t address;
			if (sd_ble_gap_addr_get(&address) != NRF_SUCCESS) {
				return;
			}
			memcpy(reply.mac, address.addr, MAC_ADDRESS_LEN);

			cs_mesh_msg_t replyMsg;
			replyMsg.type = CS_MESH_MODEL_TYPE_STONE_MAC;
			replyMsg.reliability = CS_MESH_RELIABILITY_MEDIUM;
			replyMsg.urgency = CS_MESH_URGENCY_LOW;
			replyMsg.payload = reinterpret_cast<uint8_t*>(&reply);
			replyMsg.size = sizeof(reply);

			event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &replyMsg, sizeof(replyMsg));
			event.dispatch();
			break;
		}
		case 1:{
			LOGMeshTopologyInfo("Received mac address id=%u mac=%02X:%02X:%02X:%02X:%02X:%02X", id, packet.mac[5], packet.mac[4], packet.mac[3], packet.mac[2], packet.mac[1], packet.mac[0]);
			break;
		}
	}
}

void MeshTopology::onMeshMsg(MeshMsgEvent& packet) {
	if (packet.hops != 0) {
		return;
	}
	add(packet.srcAddress, packet.rssi, packet.channel);

	if (packet.type == CS_MESH_MODEL_TYPE_STONE_MAC) {
		cs_mesh_model_msg_stone_mac_t payload = packet.getPacket<CS_MESH_MODEL_TYPE_STONE_MAC>();
		onStoneMacMsg(packet.srcAddress, payload);
	}
}

void MeshTopology::onTickSecond() {
	LOGMeshTopologyDebug("onTick");
	print();
	[[maybe_unused]] bool change = false;
	for (uint8_t i = 0; i < _neighbourCount; /**/ ) {
		_neighbours[i].lastSeenSeconds++;
		if (_neighbours[i].lastSeenSeconds == TIMEOUT_SECONDS) {
			change = true;
			// Remove item, by shifting all items after this item.
			_neighbourCount--;
			for (uint8_t j = i; j < _neighbourCount; ++j) {
				_neighbours[j] = _neighbours[j + 1];
			}
		}
		else {
			i++;
		}
	}
	if (change) {
		LOGMeshTopologyDebug("result");
		print();
	}
}

uint8_t MeshTopology::compressRssi(int8_t rssi) {
	if (rssi > -37) {
		return 63;
	}
	if (rssi < -100) {
		return 0;
	}
	return 100 + rssi;
}

uint8_t MeshTopology::compressChannel(uint8_t channel) {
	switch (channel) {
		case 37: return 1;
		case 38: return 2;
		case 39: return 3;
		default: return 0;
	}
}

void MeshTopology::print() {
	for (uint8_t i = 0; i < _neighbourCount; ++i) {
		LOGMeshTopologyDebug("id=%u rssi=%i channel=%u lastSeen=%u", _neighbours[i].id, _neighbours[i].rssi, _neighbours[i].channel, _neighbours[i].lastSeenSeconds);
	}
}

void MeshTopology::handleEvent(event_t &evt) {
	switch (evt.type) {
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto packet = CS_TYPE_CAST(EVT_RECV_MESH_MSG, evt.data);
			onMeshMsg(*packet);
			break;
		}
		case CS_TYPE::EVT_TICK: {
			auto packet = CS_TYPE_CAST(EVT_TICK, evt.data);
			if (*packet % (1000 / TICK_INTERVAL_MS) == 0) {
				onTickSecond();
			}
			break;
		}
		case CS_TYPE::CMD_MESH_TOPO_GET_MAC: {
			auto packet = CS_TYPE_CAST(CMD_MESH_TOPO_GET_MAC, evt.data);
			evt.result.returnCode = getMacAddress(*packet);
			break;
		}
		default:
			break;
	}
}
