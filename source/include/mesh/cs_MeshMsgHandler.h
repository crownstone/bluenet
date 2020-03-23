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

	/**
	 * Cache part(s) of the last received state, so it can be combined.
	 */
	cs_mesh_model_ext_state_t _lastReceivedState;

	/**
	 * Cache last received switch command, so it can be ignored.
	 */
	cs_mesh_model_msg_multi_switch_item_t _lastReceivedMultiSwitch = {0xFF};

	/**
	 * Cache last received set time command, so it can be ignored.
	 */
	TYPIFY(CMD_SET_TIME) _lastReveivedSetTime = 0;

#if MESH_MODEL_TEST_MSG == 1
	uint32_t _lastReceivedCounter = 0;
	uint32_t _received = 0;
	uint32_t _dropped = 0;
#endif

	/**
	 * Whether a state message is from the same state as last received part.
	 */
	bool isFromSameState(uint16_t srcAddress, stone_id_t id, uint16_t partialTimestamp);

	/**
	 * Check if all parts of a state are received.
	 * Send an event if that's the case.
	 */
	void checkStateReceived(int8_t rssi, uint8_t ttl);
};
