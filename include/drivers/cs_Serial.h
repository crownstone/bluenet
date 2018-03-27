/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 Oct., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <cfg/cs_Debug.h>
#include "stdint.h"

/*
 * Commonly LOG functionality is provided with as first parameter the level of severity of the message. Subsequently
 * the message follows, eventually succeeded by content if the string contains format specifiers. This means that this
 * requires a variadic macro as well, see http://www.wikiwand.com/en/Variadic_macro. The two ## are e.g. a gcc
 * specific extension that removes the , after __LINE__, so log(level, msg) can also be used without arguments.
 */
#define SERIAL_DEBUG                0
#define SERIAL_INFO                 1
#define SERIAL_WARN                 2
#define SERIAL_ERROR                3
#define SERIAL_FATAL                4
#define SERIAL_BYTE_PROTOCOL_ONLY   5
#define SERIAL_READ_ONLY            6
#define SERIAL_NONE                 7

#define SERIAL_CRLF "\r\n"

#ifndef SERIAL_VERBOSITY
#error "You have to specify SERIAL_VERBOSITY"
#define SERIAL_VERBOSITY SERIAL_NONE
#endif

//#define INCLUDE_TIMESTAMPS

#if SERIAL_VERBOSITY<SERIAL_BYTE_PROTOCOL_ONLY
	#include "string.h"
	#define _FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

	#define _log(level, fmt, ...) \
			   if (level >= SERIAL_VERBOSITY) { \
				   write(fmt, ##__VA_ARGS__); \
			   }

#ifdef INCLUDE_TIMESTAMPS

	#define log(level, fmt, ...) \
		_log(level, "[%-20.20s : %-5d](%d) " fmt, _FILE, __LINE__, now(), ##__VA_ARGS__)

	#define logLN(level, fmt, ...) \
		_log(level, "[%-20.20s : %-5d](%d) " fmt SERIAL_CRLF, _FILE, __LINE__, now(), ##__VA_ARGS__)

#else

	#define log(level, fmt, ...) \
		_log(level, "[%-30.30s : %-5d] " fmt, _FILE, __LINE__, ##__VA_ARGS__)

	#define logLN(level, fmt, ...) \
		_log(level, "[%-30.30s : %-5d] " fmt SERIAL_CRLF, _FILE, __LINE__, ##__VA_ARGS__)

#endif

#else
	#define _log(level, fmt, ...)
	#define log(level, fmt, ...)
	#define logLN(level, fmt, ...)
#endif

#define LOGd(fmt, ...) logLN(SERIAL_DEBUG, "\033[37;1m" fmt "\033[0m", ##__VA_ARGS__)
#define LOGi(fmt, ...) logLN(SERIAL_INFO,  "\033[34;1m" fmt "\033[0m", ##__VA_ARGS__)
#define LOGw(fmt, ...) logLN(SERIAL_WARN,  "\033[33;1m" fmt "\033[0m", ##__VA_ARGS__)
#define LOGe(fmt, ...) logLN(SERIAL_ERROR, "\033[35;1m" fmt "\033[0m", ##__VA_ARGS__)
#define LOGf(fmt, ...) logLN(SERIAL_FATAL, "\033[31;1m" fmt "\033[0m", ##__VA_ARGS__)

#if SERIAL_VERBOSITY>SERIAL_DEBUG
#undef LOGd
#define LOGd(fmt, ...)
#endif

#if SERIAL_VERBOSITY>SERIAL_INFO
#undef LOGi
#define LOGi(fmt, ...)
#endif

#if SERIAL_VERBOSITY>SERIAL_WARN
#undef LOGw
#define LOGw(fmt, ...)
#endif

#if SERIAL_VERBOSITY>SERIAL_ERROR
#undef LOGe
#define LOGe(fmt, ...)
#endif

#if SERIAL_VERBOSITY>SERIAL_FATAL
#undef LOGf
#define LOGf(fmt, ...)
#endif

/**
 * General configuration of the serial connection. This sets the pin to be used for UART, the baudrate, the parity
 * bits, etc.
 */
void config_uart(uint8_t pinRx, uint8_t pinTx);

/**
 * Write a string with printf functionality.
 */
int write(const char *str, ...);

/** Write a buffer of data. Values get escaped when necessary.
 *
 * @param[in] data           Pointer to the data to write.
 * @param[in] size           Size of the data.
 */
void writeBytes(uint8_t* data, const uint16_t size);

/** Write the start byte
 */
void writeStartByte();

#ifdef INCLUDE_TIMESTAMPS
int now();
#endif

#ifdef __cplusplus
}
#endif
