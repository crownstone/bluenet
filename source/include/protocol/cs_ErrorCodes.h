/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 17, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

enum ErrorCodesGeneral {
	ERR_SUCCESS                     = 0x00,
	ERR_WAIT_FOR_SUCCESS            = 0x01, // So far so good, but wait for actual success.
	ERR_SUCCESS_NO_CHANGE           = 0x02, // Success: nothing changed.

	ERR_BUFFER_UNASSIGNED           = 0x10,
	ERR_BUFFER_LOCKED               = 0x11,
	ERR_BUFFER_TOO_SMALL            = 0x12,
	ERR_NOT_ALIGNED                 = 0x13,

	ERR_WRONG_PAYLOAD_LENGTH        = 0x20,
	ERR_WRONG_PARAMETER             = 0x21,
	ERR_INVALID_MESSAGE             = 0x22,
	ERR_UNKNOWN_OP_CODE             = 0x23,
	ERR_UNKNOWN_TYPE                = 0x24,
	ERR_NOT_FOUND                   = 0x25,
	ERR_NO_SPACE                    = 0x26,
	ERR_BUSY                        = 0x27,
	ERR_WRONG_STATE                 = 0x28,
	ERR_ALREADY_EXISTS              = 0x29,
	ERR_TIMEOUT                     = 0x2A,
	ERR_CANCELED                    = 0x2B,
	ERR_PROTOCOL_UNSUPPORTED        = 0x2C,
	ERR_MISMATCH                    = 0x2D, // Mismatch of CRC, checksum, etc
	ERR_WRONG_OPERATION             = 0x2E,

	ERR_NO_ACCESS                   = 0x30,
	ERR_UNSAFE                      = 0x31,

	ERR_NOT_AVAILABLE               = 0x40,
	ERR_NOT_IMPLEMENTED             = 0x41,
//	ERR_WRONG_SETTING               = 0x42,
	ERR_NOT_INITIALIZED             = 0x43,
	ERR_NOT_STARTED					= 0x44,
	ERR_NOT_POWERED                 = 0x45,
	ERR_WRONG_MODE                  = 0x46,

	ERR_WRITE_DISABLED              = 0x50,
	ERR_WRITE_NOT_ALLOWED           = 0x51,
	ERR_READ_FAILED                 = 0x52,

	ERR_ADC_INVALID_CHANNEL         = 0x60,

	ERR_EVENT_UNHANDLED	            = 0x70,

	ERR_GATT_ERROR                  = 0x80,

	// Mesh uses 0xFF as max.
	ERR_UNSPECIFIED                 = 0xFFFF
};

#define SUCCESS(code) code == ERR_SUCCESS
#define FAILURE(code) code != ERR_SUCCESS
