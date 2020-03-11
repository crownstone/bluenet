/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "mesh/cs_MeshModel.h"
#include "protocol/mesh/cs_MeshModelPackets.h"
#include "protocol/mesh/cs_MeshModelPacketHelper.h"
#include "protocol/cs_CommandTypes.h"
#include "cfg/cs_Config.h"
#include "common/cs_Types.h"
#include "drivers/cs_Serial.h"
#include "events/cs_EventDispatcher.h"
#include "storage/cs_State.h"
#include "time/cs_SystemTime.h"
#include "util/cs_Utils.h"
#include "util/cs_BleError.h"
#include <cstddef> // For NULL

extern "C" {
#include "access.h"
#include "access_config.h"
#include "access_reliable.h"
#include "nrf_mesh.h"
#include "log.h"
}

#if TICK_INTERVAL_MS > MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS
#error "TICK_INTERVAL_MS must not be larger than MESH_MODEL_QUEUE_PROCESS_INTERVAL_MS"
#endif

#define LOGMeshModelInfo    LOGi
#define LOGMeshModelDebug   LOGd
#define LOGMeshModelVerbose LOGnone



static void cs_mesh_model_reply_status_handler(access_model_handle_t model_handle, void * p_args, access_reliable_status_t status) {
	MeshModel* meshModel = (MeshModel*)p_args;
	meshModel->handleReliableStatus(status);
}

static const access_opcode_handler_t cs_mesh_model_opcode_handlers[] = {
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_MSG, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_RELIABLE_MSG, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler},
		{ACCESS_OPCODE_VENDOR(CS_MESH_MODEL_OPCODE_REPLY, CROWNSTONE_COMPANY_ID), cs_mesh_model_msg_handler},
};

MeshModel::MeshModel() {}


void MeshModel::init() {
	uint32_t retVal;
	access_model_add_params_t accessParams;
	accessParams.model_id.company_id = CROWNSTONE_COMPANY_ID;
	accessParams.model_id.model_id = 0;
	accessParams.element_index = 0;
	accessParams.p_opcode_handlers = cs_mesh_model_opcode_handlers;
	accessParams.opcode_count = (sizeof(cs_mesh_model_opcode_handlers) / sizeof((cs_mesh_model_opcode_handlers)[0]));
	accessParams.p_args = this;
	accessParams.publish_timeout_cb = NULL;
	retVal = access_model_add(&accessParams, &_accessHandle);
	APP_ERROR_CHECK(retVal);
	retVal = access_model_subscription_list_alloc(_accessHandle);
	APP_ERROR_CHECK(retVal);
}

void MeshModel::setOwnAddress(uint16_t address) {
	_ownAddress = address;
}



cs_ret_code_t MeshModel::sendReply(const access_message_rx_t * accessMsg) {
//	size16_t msgSize = MESH_HEADER_SIZE;
//	uint8_t msg[msgSize];
	size16_t msgSize = sizeof(_replyMsg);
	uint8_t* msg = _replyMsg;
	MeshUtil::setMeshMessage(CS_MESH_MODEL_TYPE_ACK, NULL, 0, msg, msgSize);

//	access_message_tx_t replyMsg;
	_accessReplyMsg.opcode.company_id = CROWNSTONE_COMPANY_ID;
	_accessReplyMsg.opcode.opcode = CS_MESH_MODEL_OPCODE_REPLY;
	_accessReplyMsg.p_buffer = msg;
	_accessReplyMsg.length = msgSize;
	_accessReplyMsg.force_segmented = false;
	_accessReplyMsg.transmic_size = NRF_MESH_TRANSMIC_SIZE_SMALL;
	_accessReplyMsg.access_token = nrf_mesh_unique_token_get();

	uint32_t retVal = access_model_reply(_accessHandle, accessMsg, &_accessReplyMsg);
	LOGMeshModelVerbose("send reply %u", retVal);
	return retVal;
}








