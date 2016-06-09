/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jun 3, 2016
 * License: LGPLv3+
 */
#pragma once

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
	CMD_SAMPLE_POWER                = 14,    //! 0x0E
	CMD_USER_FEEDBACK               = 15,    //! 0x0F
	CMD_SCHEDULE_ENTRY              = 16,    //! 0x10
	CMD_TYPES
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


