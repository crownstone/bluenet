/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 22, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>

/**
 * Command originates from ..
 */
enum cs_cmd_source {
	CS_CMD_SOURCE_NONE = 0,
	CS_CMD_SOURCE_DEFAULT,             // No good source description, should not be used.
	CS_CMD_SOURCE_INTERNAL,            // Internal control command, should not be used.
	CS_CMD_SOURCE_UART,                // UART.
	CS_CMD_SOURCE_CONNECTION,          // BLE connection.
	CS_CMD_SOURCE_SWITCHCRAFT,         // Switchcraft trigger.
	CS_CMD_SOURCE_DEVICE_TOKEN = 0x300 // A specific device. Add device token to this value.
};


/**
 * flagExternal: true when received via mesh
 * sourceId: see cs_cmd_source
 * count: counter of the device
 */
struct __attribute__((packed)) cmd_source_t {
	bool flagExternal : 1;
	uint8_t reserved  : 5;
	uint16_t sourceId : 10;
	uint8_t count     : 8;
	cmd_source_t(uint16_t source = CS_CMD_SOURCE_DEFAULT):
		flagExternal(false),
		reserved(0),
		sourceId(source),
		count(0)
	{}
};
