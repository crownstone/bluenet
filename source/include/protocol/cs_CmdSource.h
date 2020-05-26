/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 22, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint>

enum cs_cmd_source_type {
	CS_CMD_SOURCE_TYPE_ENUM = 0,       // Source ID is one of cs_cmd_source_id.
	CS_CMD_SOURCE_TYPE_BEHAVIOUR = 1,  // Source ID is the Nth behaviour rule.
	CS_CMD_SOURCE_TYPE_BROADCAST = 3   // Source ID is the device ID.
};

/**
 * Command originates from ..
 */
enum cs_cmd_source_id {
	CS_CMD_SOURCE_NONE            = 0,
//	CS_CMD_SOURCE_SAFETY          = 1,        // Safety precautions, for example overcurrent.
	CS_CMD_SOURCE_INTERNAL        = 2,        // Internal control command, should not be used.
	CS_CMD_SOURCE_UART            = 3,        // UART.
	CS_CMD_SOURCE_CONNECTION      = 4,        // BLE connection.
	CS_CMD_SOURCE_SWITCHCRAFT     = 5,        // Switchcraft trigger.
//  CS_CMD_SOURCE_CONFIG_APPLY    = 6,        // A configuration was changed, resulting in an immediate change. For example, dimming is no longer allowed.
};

/**
 * Struct that tells where a command originated from.
 */
struct __attribute__((packed)) cmd_source_t {
	bool flagExternal  : 1; // True when the command was received via mesh.
	uint8_t reserved   : 4; // Reserved for flags.
	uint8_t type       : 3; // See cs_cmd_source_type.
	uint8_t id         : 8; // Source ID, depends on type.
	cmd_source_t(cs_cmd_source_id sourceId = CS_CMD_SOURCE_NONE):
		flagExternal(false),
		reserved(0),
		type(CS_CMD_SOURCE_TYPE_ENUM),
		id(sourceId)
	{}
};

/**
 * Struct that tells where a command originated from.
 * Including the command counter, which helps to figure out if a command is new or not.
 */
struct __attribute__((packed)) cmd_source_with_counter_t {
	cmd_source_t source;
	uint8_t count; // Command counter, increases each command.
	cmd_source_with_counter_t(cs_cmd_source_id source = CS_CMD_SOURCE_NONE):
		source(source),
		count(0)
	{}
};
