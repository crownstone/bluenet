/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 3, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "structs/cs_ScheduleEntries.h"

enum CommandHandlerTypes {
	CMD_SWITCH                      = 0,     //! 0x00
	CMD_PWM                         = 1,     //! 0x01
	CMD_SET_TIME                    = 2,     //! 0x02
	CMD_GOTO_DFU                    = 3,     //! 0x03
	CMD_RESET                       = 4,     //! 0x04
	CMD_FACTORY_RESET               = 5,     //! 0x05
	CMD_KEEP_ALIVE_STATE            = 6,     //! 0x06
	CMD_KEEP_ALIVE                  = 7,     //! 0x07
	CMD_ENABLE_MESH                 = 8,     //! 0x08
	CMD_ENABLE_ENCRYPTION           = 9,     //! 0x09
	CMD_ENABLE_IBEACON              = 10,    //! 0x0A
	CMD_ENABLE_CONT_POWER_MEASURE   = 11,    //! 0x0B
	CMD_ENABLE_SCANNER              = 12,    //! 0x0C
	CMD_SCAN_DEVICES                = 13,    //! 0x0D
	CMD_USER_FEEDBACK               = 14,    //! 0x0E
	CMD_SCHEDULE_ENTRY_SET          = 15,    //! 0x0F
	CMD_RELAY                       = 16,    //! 0x10
	CMD_VALIDATE_SETUP              = 17,    //! 0x11
	CMD_REQUEST_SERVICE_DATA        = 18,    //! 0x12
	CMD_DISCONNECT                  = 19,    //! 0x13
	CMD_SET_LED                     = 20,    //! 0x14
	CMD_NOP                         = 21,    //! 0x15
	CMD_INCREASE_TX                 = 22,    //! 0x16
	CMD_RESET_ERRORS                = 23,    //! 0x17
	CMD_KEEP_ALIVE_REPEAT_LAST      = 24,    //! 0x18
	CMD_MULTI_SWITCH                = 25,    //! 0x19
	CMD_SCHEDULE_ENTRY_CLEAR        = 26,    //! 0x1A
	CMD_KEEP_ALIVE_MESH             = 27,    //! 0x1B
	CMD_MESH_COMMAND                = 28,    //! 0x1C
	CMD_ALLOW_DIMMING               = 29,    //! 0x1D
	CMD_LOCK_SWITCH                 = 30,    //! 0x1E
	CMD_SETUP                       = 31,    //! 0x1F
	CMD_ENABLE_SWITCHCRAFT          = 32,    //! 0x20
	CMD_UART_MSG                    = 33,    //! 0x21

	CMD_UNKNOWN                     = 0xFF
};

struct __attribute__((__packed__)) switch_message_payload_t {
	uint8_t switchState;
};

struct __attribute__((__packed__)) opcode_message_payload_t {
	uint8_t opCode;
};

struct __attribute__((__packed__)) enable_message_payload_t {
	bool enable;
};

struct __attribute__((__packed__)) enable_scanner_message_payload_t {
	bool enable;
	uint16_t delay;
};

struct __attribute__((__packed__)) factory_reset_message_payload_t {
	uint32_t resetCode;
};

enum KeepAliveActionTypes {
	NO_CHANGE = 0,
	CHANGE    = 1
};

struct __attribute__((__packed__)) keep_alive_state_message_payload_t {
	uint8_t action;
	switch_message_payload_t switchState;
	uint16_t timeout; // timeout in seconds
};

struct __attribute__((__packed__)) led_message_payload_t {
	uint8_t led;
	bool enable;
};

struct __attribute__((__packed__)) schedule_command_t {
	uint8_t id;
	schedule_entry_t entry;
};

