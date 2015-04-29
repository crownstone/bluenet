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

/*
 * Commonly LOG functionality is provided with as first parameter the level of severity of the message. Subsequently
 * the message follows, eventually succeeded by content if the string contains format specifiers. This means that this
 * requires a variadic macro as well, see http://www.wikiwand.com/en/Variadic_macro. The two ## are e.g. a gcc
 * specific extension that removes the , after __LINE__, so log(level, msg) can also be used without arguments.
 */
#define DEBUG                0
#define INFO                 1
#define WARN                 2
#define ERROR                3
#define FATAL                4
#define NONE                 5


#ifndef SERIAL_VERBOSITY
#error "You have to specify SERIAL_VERBOSITY"
#endif

#if SERIAL_VERBOSITY<NONE
	#include "string.h"
	#define _FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

	#define log(level, fmt, ...) \
			   write("[%-30.30s : %-5d] " fmt "\r\n", _FILE, __LINE__, ##__VA_ARGS__)

	#define _log(level, fmt, ...) \
			   write(fmt, ##__VA_ARGS__)
#else
	#define log(level, fmt, ...)

	#define _log(level, fmt, ...)
#endif

#define LOGd(fmt, ...) log(DEBUG, fmt, ##__VA_ARGS__)
#define LOGi(fmt, ...) log(INFO, fmt, ##__VA_ARGS__)
#define LOGw(fmt, ...) log(WARN, fmt, ##__VA_ARGS__)
#define LOGe(fmt, ...) log(ERROR, fmt, ##__VA_ARGS__)
#define LOGf(fmt, ...) log(FATAL, fmt, ##__VA_ARGS__)

#if SERIAL_VERBOSITY>DEBUG
#undef LOGd
#define LOGd(fmt, ...)
#endif

#if SERIAL_VERBOSITY>INFO
#undef LOGi
#define LOGi(fmt, ...)
#endif

#if SERIAL_VERBOSITY>WARN
#undef LOGw
#define LOGw(fmt, ...)
#endif

#if SERIAL_VERBOSITY>ERROR
#undef LOGe
#define LOGe(fmt, ...)
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
void config_uart();

/**
 * Write a string to the serial connection. Make sure you end with `\n` if you want to have new lines in the output.
 * Also flushing the buffer might be done around new lines.
 */
//void write(const char *str);

/**
 * Write a string with printf functionality.
 */
int write(const char *str, ...);

#ifdef __cplusplus
}
#endif
