/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>

/**
 * Class that:
 * - Handles received messages from the mesh.
 */
class MeshMsgHandler {
public:
	void init();
	void handleMsg(const MeshUtil::cs_mesh_received_msg_t& msg);

protected:
	void handleTest(                 uint8_t* payload, size16_t payloadSize);
	void handleAck(                  uint8_t* payload, size16_t payloadSize);
	void handleStateTime(            uint8_t* payload, size16_t payloadSize);
	void handleCmdTime(              uint8_t* payload, size16_t payloadSize);
	void handleCmdNoop(              uint8_t* payload, size16_t payloadSize);
	void handleCmdMultiSwitch(       uint8_t* payload, size16_t payloadSize);
	void handleState0(               uint8_t* payload, size16_t payloadSize, uint16_t srcAddress, int8_t rssi, uint8_t hops);
	void handleState1(               uint8_t* payload, size16_t payloadSize, uint16_t srcAddress, int8_t rssi, uint8_t hops);
	void handleProfileLocation(      uint8_t* payload, size16_t payloadSize);
	void handleSetBehaviourSettings( uint8_t* payload, size16_t payloadSize);
	void handleTrackedDeviceRegister(uint8_t* payload, size16_t payloadSize);
	void handleTrackedDeviceToken(   uint8_t* payload, size16_t payloadSize);
	void handleTrackedDeviceListSize(uint8_t* payload, size16_t payloadSize);
	void handleSyncRequest(          uint8_t* payload, size16_t payloadSize);

private:
	TYPIFY(CONFIG_CROWNSTONE_ID) _ownId = 0;

	struct cs_mesh_model_ext_state_t {
		uint16_t address = 0;
		uint8_t partsReceivedBitmask = 0;
		TYPIFY(EVT_STATE_EXTERNAL_STONE) state;
	};
	cs_mesh_model_ext_state_t _lastReceivedState;

	cs_mesh_model_msg_multi_switch_item_t _lastReceivedMultiSwitch = {0xFF};
	TYPIFY(CMD_SET_TIME) _lastReveivedSetTime = 0;

	bool isFromSameState(uint16_t srcAddress, stone_id_t id, uint16_t partialTimestamp);
	void checkStateReceived(int8_t rssi, uint8_t ttl);
};
