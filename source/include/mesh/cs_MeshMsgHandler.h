/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>
#include <protocol/cs_UartMsgTypes.h>

/**
 * Class that:
 * - Handles received messages from the mesh.
 */
class MeshMsgHandler {
public:
	void init();
	void handleMsg(MeshMsgEvent& msg);

protected:
	cs_ret_code_t handleTest(MeshMsgEvent& msg);
	cs_ret_code_t handleAck(MeshMsgEvent& msg);
	cs_ret_code_t handleCmdTime(MeshMsgEvent& msg);
	cs_ret_code_t handleCmdNoop(MeshMsgEvent& msg);
	cs_ret_code_t handleRssiPing(MeshMsgEvent& msg);
	cs_ret_code_t handleRssiData(MeshMsgEvent& msg);
	cs_ret_code_t handleTimeSync(MeshMsgEvent& msg);
	cs_ret_code_t handleCmdMultiSwitch(MeshMsgEvent& msg);
	cs_ret_code_t handleState0(MeshMsgEvent& msg);
	cs_ret_code_t handleState1(MeshMsgEvent& msg);
	cs_ret_code_t handleProfileLocation(MeshMsgEvent& msg);
	cs_ret_code_t handleTrackedDeviceRegister(MeshMsgEvent& msg);
	cs_ret_code_t handleTrackedDeviceToken(MeshMsgEvent& msg);
	cs_ret_code_t handleTrackedDeviceHeartbeat(MeshMsgEvent& msg);
	cs_ret_code_t handleTrackedDeviceListSize(MeshMsgEvent& msg);
	cs_ret_code_t handleSyncRequest(MeshMsgEvent& msg);
	void handleStateSet(MeshMsgEvent& msg);
	void handleControlCommand(MeshMsgEvent& msg);
	cs_ret_code_t handleResult(MeshMsgEvent& msg);
	cs_ret_code_t handleSetIbeaconConfigId(MeshMsgEvent& msg);

	cs_ret_code_t dispatchEventForMeshMsg(CS_TYPE evtType, MeshMsgEvent& msg);

private:
	TYPIFY(CONFIG_CROWNSTONE_ID) _ownId = 0;

	struct cs_mesh_model_ext_state_t {
		stone_id_t srcId             = 0;
		uint8_t partsReceivedBitmask = 0;
		TYPIFY(EVT_STATE_EXTERNAL_STONE) state;
	};

	/**
	 * Cache part(s) of the last received state, so it can be combined.
	 */
	cs_mesh_model_ext_state_t _lastReceivedState;

	//	/**
	//	 * Cache last received switch command, so it can be ignored.
	//	 */
	//	cs_mesh_model_msg_multi_switch_item_t _lastReceivedMultiSwitch = {0xFF};

	/**
	 * Cache last received set time command, so it can be ignored.
	 */
	TYPIFY(CMD_SET_TIME) _lastReveivedSetTime = 0;

#if MESH_MODEL_TEST_MSG == 1
	uint32_t _lastReceivedCounter = 0;
	uint32_t _received            = 0;
	uint32_t _dropped             = 0;
#endif

	/**
	 * Whether a state message is from the same state as last received part.
	 */
	bool isFromSameState(stone_id_t srcId, stone_id_t id, uint16_t partialTimestamp);

	/**
	 * Check if all parts of a state are received.
	 * Send an event if that's the case.
	 */
	void checkStateReceived();

	/**
	 * Set the reply to a result message with given mesh message type and return code as payload.
	 */
	void replyWithRetCode(cs_mesh_model_msg_type_t type, cs_ret_code_t retCode, mesh_reply_t* reply);

	/**
	 * Send the given result to UART.
	 */
	void sendResultToUart(uart_msg_mesh_result_packet_header_t& resultHeader, const cs_data_t& resultData);
};
