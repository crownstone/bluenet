/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 20, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>
#include <structs/cs_PacketsInternal.h>

// Commands:

struct cs_central_connect_t {
	device_address_t address;
	uint16_t timeoutMs = 3000;
};

struct cs_central_write_t {
	cs_data_t data;
};

// Events:

struct cs_central_read_result_t {
	cs_ret_code_t retCode;
	cs_data_t data;
};
