/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "mesh/cs_MeshModel.h"
#include "protocol/mesh/cs_MeshModelProtocol.h"
#include "cfg/cs_Config.h"
#include <cstddef> // For NULL

extern "C" {
#include "access.h"
#include "nrf_mesh.h"
}

static void cs_mesh_model_msg_handler(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args) {
	MeshModel* meshModel = (MeshModel*)p_args;
	meshModel->handleMsg(p_message);
}

static const access_opcode_handler_t cs_mesh_model_opcode_handlers[] = {
		{{CS_MESH_MODEL_OPCODE_MSG, CROWNSTONE_COMPANY_ID}, cs_mesh_model_msg_handler}
};


void MeshModel::init() {
	access_model_add_params_t accessParams;
	accessParams.model_id.company_id = CROWNSTONE_COMPANY_ID;
	accessParams.model_id.model_id = 0;
	accessParams.element_index = 0;
	accessParams.p_opcode_handlers = cs_mesh_model_opcode_handlers;
	accessParams.opcode_count = 1;
	accessParams.p_args = this;
	accessParams.publish_timeout_cb = NULL;
	access_model_add(&accessParams, &_accessHandle);
}

// TODO: wait for NRF_MESH_EVT_TX_COMPLETE before sending next msg (in case of segmented msg?).
cs_ret_code_t MeshModel::sendMsg(uint8_t* data, uint16_t len) {
	access_message_tx_t accessMsg;
	accessMsg.opcode.company_id = CROWNSTONE_COMPANY_ID;
	accessMsg.opcode.opcode = CS_MESH_MODEL_OPCODE_MSG;
	accessMsg.p_buffer = data;
	accessMsg.length = len;
	accessMsg.force_segmented = false;
	accessMsg.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
	accessMsg.access_token = nrf_mesh_unique_token_get();
	return access_model_publish(_accessHandle, &accessMsg);
}

void MeshModel::handleMsg(const access_message_rx_t * accessMsg) {
//	accessMsg->p_data
}
