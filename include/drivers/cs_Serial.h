/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 10 Oct., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
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
#define SERIAL_NONE                 5

#define SERIAL_CRLF "\r\n"

#ifndef SERIAL_VERBOSITY
#error "You have to specify SERIAL_VERBOSITY"
#define SERIAL_VERBOSITY SERIAL_NONE
#endif

//#define INCLUDE_TIMESTAMPS

#if SERIAL_VERBOSITY<SERIAL_NONE
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

/** Available Baud rates for UART. */
typedef enum
{
    UART_BAUD_2K4 = 0,        ///< 2400 baud
    UART_BAUD_4K8,            ///< 4800 baud
    UART_BAUD_9K6,            ///< 9600 baud
    UART_BAUD_14K4,           ///< 14.4 kbaud
    UART_BAUD_19K2,           ///< 19.2 kbaud
    UART_BAUD_28K8,           ///< 28.8 kbaud
    UART_BAUD_38K4,           ///< 38.4 kbaud
    UART_BAUD_57K6,           ///< 57.6 kbaud
    UART_BAUD_76K8,           ///< 76.8 kbaud
    UART_BAUD_115K2,          ///< 115.2 kbaud
    UART_BAUD_230K4,          ///< 230.4 kbaud
    UART_BAUD_250K0,          ///< 250.0 kbaud
    UART_BAUD_500K0,          ///< 500.0 kbaud
    UART_BAUD_1M0,            ///< 1 mbaud
    UART_BAUD_TABLE_MAX_SIZE  ///< Used to specify the size of the baudrate table.
} uart_baudrate_t;

/** @brief The baudrate devisors array, calculated for standard baudrates.
    Number of elements defined by ::HAL_UART_BAUD_TABLE_MAX_SIZE*/
#define UART_BAUDRATE_DEVISORS_ARRAY    { \
    0x0009D000, 0x0013B000, 0x00275000, 0x003BA000, 0x004EA000, 0x0075F000, 0x009D4000, \
    0x00EBE000, 0x013A9000, 0x01D7D000, 0x03AFB000, 0x03FFF000, 0x075F6000, 0x10000000  }

/**
 * General configuration of the serial connection. This sets the pin to be used for UART, the baudrate, the parity
 * bits, etc.
 */
void config_uart(uint8_t pinRx, uint8_t pinTx);

/**
 * Write a string to the serial connection. Make sure you end with `\n` if you want to have new lines in the output.
 * Also flushing the buffer might be done around new lines.
 */
//void write(const char *str);

/**
 * Write a string with printf functionality.
 */
int write(const char *str, ...);

#ifdef INCLUDE_TIMESTAMPS
int now();
#endif

#ifdef __cplusplus
}
#endif
