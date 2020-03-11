/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "common/cs_Types.h"

extern "C" {
#include "access.h"
#include "access_reliable.h"
}






class MeshModel {
public:
	MeshModel();
	void init();
	void setOwnAddress(uint16_t address);

	void tick(TYPIFY(EVT_TICK) tickCount);

	/** Internal usage */
	void handleReliableStatus(access_reliable_status_t status);

#if MESH_MODEL_TEST_MSG == 1
	uint32_t _nextSendCounter = 1;
	uint32_t _lastReceivedCounter = 0;
	uint32_t _received = 0;
	uint32_t _dropped = 0;
#elif MESH_MODEL_TEST_MSG == 2
	uint32_t _nextSendCounter = 1;
	uint32_t _acked = 0;
	uint32_t _timedout = 0;
	uint32_t _canceled = 0;
#endif

private:
	access_model_handle_t _accessHandle = ACCESS_HANDLE_INVALID;
	uint16_t _ownAddress = 0;
	access_reliable_t _accessReliableMsg;
	uint8_t _reliableMsg[MAX_MESH_MSG_SIZE]; // Need to keep msg in ram until reply is received.
	access_message_tx_t _accessReplyMsg;
	uint8_t _replyMsg[MESH_HEADER_SIZE + 0];

	cs_ret_code_t _sendReliableMsg(const uint8_t* data, uint16_t len);
	cs_ret_code_t sendReply(const access_message_rx_t * accessMsg);
};
