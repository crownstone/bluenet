/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 7 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "events/cs_EventListener.h"
#include "common/cs_Types.h"

extern "C" {
#include "access.h"
}

class MeshModel : EventListener {
public:
	void init();
	cs_ret_code_t sendMsg(uint8_t* data, uint16_t len);

	/**
	 * Internal usage
	 */
	void handleMsg(const access_message_rx_t * accessMsg);

private:
	access_model_handle_t _accessHandle;
};
