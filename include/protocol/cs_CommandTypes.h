/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 3, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

//TODO: make a enum class of these
enum CommandHandlerTypes {
	CTRL_CMD_SWITCH                      = 0,     //! 0x00
	CTRL_CMD_PWM                         = 1,     //! 0x01
	CTRL_CMD_SET_TIME                    = 2,     //! 0x02
	CTRL_CMD_GOTO_DFU                    = 3,     //! 0x03
	CTRL_CMD_RESET                       = 4,     //! 0x04
	CTRL_CMD_FACTORY_RESET               = 5,     //! 0x05
	CTRL_CMD_KEEP_ALIVE_STATE            = 6,     //! 0x06
	CTRL_CMD_KEEP_ALIVE                  = 7,     //! 0x07
	CTRL_CMD_ENABLE_MESH                 = 8,     //! 0x08
	CTRL_CMD_ENABLE_ENCRYPTION           = 9,     //! 0x09
	CTRL_CMD_ENABLE_IBEACON              = 10,    //! 0x0A
	CTRL_CMD_ENABLE_SCANNER              = 12,    //! 0x0C
//	CTRL_CMD_SCAN_DEVICES                = 13,    //! 0x0D // Removed
	CTRL_CMD_USER_FEEDBACK               = 14,    //! 0x0E
	CTRL_CMD_SCHEDULE_ENTRY_SET          = 15,    //! 0x0F
	CTRL_CMD_RELAY                       = 16,    //! 0x10
	CTRL_CMD_REQUEST_SERVICE_DATA        = 18,    //! 0x12 // Deprecated?
	CTRL_CMD_DISCONNECT                  = 19,    //! 0x13
	CTRL_CMD_SET_LED                     = 20,    //! 0x14 // Deprecated?
	CTRL_CMD_NOP                         = 21,    //! 0x15
	CTRL_CMD_INCREASE_TX                 = 22,    //! 0x16
	CTRL_CMD_RESET_ERRORS                = 23,    //! 0x17
	CTRL_CMD_KEEP_ALIVE_REPEAT_LAST      = 24,    //! 0x18
	CTRL_CMD_MULTI_SWITCH                = 25,    //! 0x19
	CTRL_CMD_SCHEDULE_ENTRY_CLEAR        = 26,    //! 0x1A
	CTRL_CMD_KEEP_ALIVE_MESH             = 27,    //! 0x1B
	CTRL_CMD_MESH_COMMAND                = 28,    //! 0x1C
	CTRL_CMD_ALLOW_DIMMING               = 29,    //! 0x1D
	CTRL_CMD_LOCK_SWITCH                 = 30,    //! 0x1E
	CTRL_CMD_SETUP                       = 31,    //! 0x1F
	CTRL_CMD_ENABLE_SWITCHCRAFT          = 32,    //! 0x20
	CTRL_CMD_UART_MSG                    = 33,    //! 0x21
	CTRL_CMD_UART_ENABLE                 = 34,    //! 0x22

	CTRL_CMD_UNKNOWN                     = 0xFF
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

