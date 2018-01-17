/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jun 17, 2016
 * License: LGPLv3+
 */
#pragma once

enum ErrorCodesGeneral {
	ERR_SUCCESS                     = 0x00,
	ERR_VALUE_UNDEFINED             = 0x01,
	ERR_WRONG_PAYLOAD_LENGTH        = 0x02,
	ERR_UNKNOWN_OP_CODE             = 0x03,
	ERR_BUFFER_LOCKED               = 0x04,
	ERR_ACCESS_NOT_ALLOWED          = 0x05,
	ERR_BUFFER_TOO_SMALL            = 0x06,
	ERR_INVALID_CHANNEL             = 0x07,
	ERR_NOT_FOUND                   = 0x08,
};

enum CommandErrorCodes {
	ERR_COMMAND_NOT_FOUND = 0x100,
	ERR_NOT_AVAILABLE,
	ERR_WRONG_PARAMETER,
	ERR_COMMAND_FAILED,
	ERR_NOT_IMPLEMENTED,
};

enum MeshErrorCodes {
	ERR_INVALID_MESSAGE = 0x200,
	ERR_UNKNOWN_MESSAGE_TYPE = 0x201,
};

enum ConfigErrorCodes {
	ERR_READ_CONFIG_FAILED = 0x300,
	ERR_WRITE_CONFIG_DISABLED = 0x301,
	ERR_CONFIG_NOT_FOUND = 0x302,
};

enum StateErrorCodes {
	ERR_STATE_NOT_FOUND = 0x400,
	STATE_WRITE_DISABLED = 0x401,
};

#define SUCCESS(code) code == ERR_SUCCESS
#define FAILURE(code) code != ERR_SUCCESS
