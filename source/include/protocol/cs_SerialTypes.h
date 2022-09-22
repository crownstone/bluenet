/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 18, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

#include <stdint.h>

/**
 * The log levels follow more or less common conventions with a few exceptions. There are some modes in which we run
 * where even fatal messages will not be written to console. In production we use SERIAL_NONE, SERIAL_READ_ONLY, or
 * SERIAL_BYTE_PROTOCOL_ONLY.
 */
#define SERIAL_NONE 0
#define SERIAL_READ_ONLY 1
#define SERIAL_BYTE_PROTOCOL_ONLY 2
#define SERIAL_FATAL 3
#define SERIAL_ERROR 4
#define SERIAL_WARN 5
#define SERIAL_INFO 6
#define SERIAL_DEBUG 7
#define SERIAL_VERBOSE 8
#define SERIAL_VERY_VERBOSE 9

#define SERIAL_CRLF "\r\n"

#ifndef SERIAL_VERBOSITY
#error "You have to specify SERIAL_VERBOSITY"
#define SERIAL_VERBOSITY SERIAL_NONE
#endif

typedef enum {
	SERIAL_ENABLE_NONE      = 0,
	SERIAL_ENABLE_RX_ONLY   = 1,
	SERIAL_ENABLE_RX_AND_TX = 3,
} serial_enable_t;

typedef void (*serial_read_callback)(uint8_t val);
