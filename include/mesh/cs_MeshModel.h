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
}

class MeshModel {
public:
	void init();
	cs_ret_code_t sendMsg(const uint8_t* data, uint16_t len, uint8_t repeats=1);

	access_model_handle_t getAccessModelHandle();

	/**
	 * Internal usage
	 */
	void handleMsg(const access_message_rx_t * accessMsg);

private:
	access_model_handle_t _accessHandle;

	int8_t getRssi(const nrf_mesh_rx_metadata_t* metaData);
	void printMeshAddress(const char* prefix, const nrf_mesh_address_t* addr);
};
