/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 20, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <structs/cs_PacketsInternal.h>
#include <structs/cs_ResultPacketAccessor.h>

// Commands

struct cs_central_connect_t {
	stone_id_t stoneId = 0;  // If you set stoneId, that will be used instead of address.
	device_address_t address;
	uint16_t timeoutMs = 3000;
};

struct cs_central_write_t {
	cs_control_cmd_t commandType;
	cs_data_t data;
};

// Events

/**
 * Result of writing a control command.
 *
 * writeRetCode    Whether the write was successful.
 * result          The result packet that was received as reply, only set when write was successful.
 *                 Check if ResultPacketAccessor is initialized before reading out fields.
 *                 Check the result code to see if the command was actually executed.
 */
struct cs_central_write_result_t {
	cs_ret_code_t writeRetCode;
	ResultPacketAccessor<> result;
};
