/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 Oct., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Debug.h>
#include <cfg/cs_Git.h>
#include <ble/cs_Nordic.h>
#include <cfg/cs_Strings.h>


struct __attribute__((__packed__)) uart_msg_log_header_t {
	uint32_t fileNameHash;
	uint16_t lineNumber; // Line number (starting at line 1) where the ; of this log is.
	struct __attribute__((packed)) {
		bool prefix : 1;  // Whether this log should be prefixed with a timestamp etc.
		bool newLine : 1; // Whether this log should end with a new line.
	} flags;
	uint8_t numArgs;
	// Followed by <numArgs> args, with uart_msg_log_arg_header_t as header.
};

struct __attribute__((__packed__)) uart_msg_log_arg_header_t {
	uint8_t argSize;
	// Followed by <argSize> bytes.
};

#include <type_traits>
#include "stdint.h"
#include "cstring"

#ifdef HOST_TARGET
#include "stdio.h"
#endif

/*
 * Commonly LOG functionality is provided with as first parameter the level of severity of the message. Subsequently
 * the message follows, eventually succeeded by content if the string contains format specifiers. This means that this
 * requires a variadic macro as well, see http://www.wikiwand.com/en/Variadic_macro. The two ## are e.g. a gcc
 * specific extension that removes the , after __LINE__, so log(level, msg) can also be used without arguments.
 *
 * The log levels follow more or less common conventions with a few exeptions. There are some modes in which we run
 * where even fatal messages will not be written to console. In production we use SERIAL_NONE, SERIAL_READ_ONLY, or
 * SERIAL_BYTE_PROTOCOL_ONLY.
 *
 * TODO: Somehow all namespaces are removed. This leads to conflicts! The function "log" means something if you
 * include <cmath>...
 */
#define SERIAL_NONE                 0
#define SERIAL_READ_ONLY            1
#define SERIAL_BYTE_PROTOCOL_ONLY   2
#define SERIAL_FATAL                3
#define SERIAL_ERROR                4
#define SERIAL_WARN                 5
#define SERIAL_INFO                 6
#define SERIAL_DEBUG                7
#define SERIAL_VERBOSE              8

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


#define LOG_IGNORE(fmt, ...)

// To disable particular logs, but without commenting it.
#define LOGnone LOG_IGNORE


/**
 * Returns the 32 bits DJB2 hash of a the reversed file name, up to the first '/'.
 *
 * TODO: For some reason, if this returns a uint16_t, it increased the size by 3k?
 */
constexpr uint32_t fileNameHash(const char* str, size_t strLen) {
	uint32_t hash = 5381;
	for (size_t i = strLen-1; i >= 0; --i) {
		if (str[i] == '/') {
			return hash;
		}
		hash = ((hash << 5) + hash) + str[i]; // hash * 33 + c
	}
	return hash;
}



#if !defined HOST_TARGET && (CS_SERIAL_NRF_LOG_ENABLED > 0)
	#define LOG_FLUSH NRF_LOG_FLUSH
#else
	#define LOG_NOTHING()
	#define LOG_FLUSH LOG_NOTHING
#endif

#if !defined HOST_TARGET && (CS_SERIAL_NRF_LOG_ENABLED == 2)
	#ifdef __cplusplus
	extern "C" {
	#endif
		//#warning NRF_LOG
		#define LOGv NRF_LOG_DEBUG
		#define LOGd NRF_LOG_DEBUG
		#define LOGi NRF_LOG_INFO
		#define LOGw NRF_LOG_WARNING
		#define LOGe NRF_LOG_ERROR
		#define LOGf NRF_LOG_ERROR
		#define _log(level, fmt, addPrefix, addNewLine, ...)
		#define logLN(level, fmt, ...)
	#ifdef __cplusplus
	}
	#endif
#else

	#if SERIAL_VERBOSITY > SERIAL_BYTE_PROTOCOL_ONLY

		#include "string.h"
		//	#define _FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
		//  #define STRING_BACK(s, l) (sizeof(s) > l ? s + (sizeof(s)-l-1) : s)
		#define _FILE (sizeof(__FILE__) > 30 ? __FILE__ + (sizeof(__FILE__)-30-1) : __FILE__) // sizeof() returns the buffer size, so including the null terminator.

//		#define _log(level, fmt, ...) if (level <= SERIAL_VERBOSITY) { cs_write(fmt, ##__VA_ARGS__); }

//		#define logLN(level, fmt, ...) _log(level, "[%-30.30s : %-5d] " fmt SERIAL_CRLF, _FILE, __LINE__, ##__VA_ARGS__)
//		#define logLN(level, fmt, ...) cs_write_args(_FILE, __LINE__, ##__VA_ARGS__); _log(level, "[%-30.30s : %-5d] " fmt SERIAL_CRLF, _FILE, __LINE__, ##__VA_ARGS__)
//		#define logLN(level, fmt, ...) if (level <= SERIAL_VERBOSITY) { cs_write_args(_FILE, __LINE__, ##__VA_ARGS__); }
//		#define logLN(level, fmt, ...) if (level <= SERIAL_VERBOSITY) { cs_write_args(fileNameHash<30>(_FILE), __LINE__, ##__VA_ARGS__); }
//		#define logLN(level, fmt, ...) if (level <= SERIAL_VERBOSITY) { cs_write_args(fileNameHash<sizeof(__FILE__)>(__FILE__), __LINE__, ##__VA_ARGS__); }

		#define _log(level, addPrefix, addNewLine, fmt, ...) \
				if (level <= SERIAL_VERBOSITY) { \
					cs_write_args(fileNameHash(__FILE__, sizeof(__FILE__)), __LINE__, addPrefix, addNewLine, ##__VA_ARGS__); \
				}
		#define logLN(level, fmt, ...) _log(level, true, true, fmt, ##__VA_ARGS__)


	#else
		#define _log(level, addPrefix, addNewLine, fmt, ...)
		#define logLN(level, fmt, ...)
	#endif

	#define LOGv(fmt, ...) logLN(SERIAL_VERBOSE, "\033[37;1m" fmt "\033[0m", ##__VA_ARGS__) // White
	#define LOGd(fmt, ...) logLN(SERIAL_DEBUG,   "\033[37;1m" fmt "\033[0m", ##__VA_ARGS__) // White
	#define LOGi(fmt, ...) logLN(SERIAL_INFO,    "\033[34;1m" fmt "\033[0m", ##__VA_ARGS__) // Blue
	#define LOGw(fmt, ...) logLN(SERIAL_WARN,    "\033[33;1m" fmt "\033[0m", ##__VA_ARGS__) // Yellow
	#define LOGe(fmt, ...) logLN(SERIAL_ERROR,   "\033[35;1m" fmt "\033[0m", ##__VA_ARGS__) // Purple
	#define LOGf(fmt, ...) logLN(SERIAL_FATAL,   "\033[31;1m" fmt "\033[0m", ##__VA_ARGS__) // Red
#endif

#define LOG_MEMORY \
	uint8_t *p = (uint8_t*)malloc(1); \
	void* sp; \
	asm("mov %0, sp" : "=r"(sp) : : ); \
	LOGd("Memory %s() heap=%p, stack=%p", __func__, p, (uint8_t*)sp); \
	free(p);

#if SERIAL_VERBOSITY < SERIAL_VERBOSE
	#undef LOGv
	#define LOGv(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_DEBUG
	#undef LOGd
	#define LOGd(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_INFO
	#undef LOGi
	#define LOGi(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_WARN
	#undef LOGw
	#define LOGw(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_ERROR
	#undef LOGe
	#define LOGe(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_FATAL
	#undef LOGf
	#define LOGf(fmt, ...)
#endif

/**
 * General configuration of the serial connection. This sets the pin to be used for UART, the baudrate, the parity
 * bits, etc.
 * Should only be called once.
 */
void serial_config(uint8_t pinRx, uint8_t pinTx);

/**
 * Init the UART.
 * Make sure it has been configured first.
 */
void serial_init(serial_enable_t enabled);

/**
 * Change what is enabled.
 */
void serial_enable(serial_enable_t enabled);

/**
 * Get the state of the serial.
 */
serial_enable_t serial_get_state();

/**
 * Returns whether serial is ready for TX, and has it enabled.
 */
bool serial_tx_ready();

/**
 * Write a string with printf functionality.
 */
#ifdef HOST_TARGET
	#define cs_write printf
#else
	int cs_write(const char *str, ...);
#endif

/**
 * Write a single byte.
 */
void writeByte(uint8_t val);


void cs_write_start(size_t msgSize);
void cs_write_header(uart_msg_log_header_t& header);
void cs_write_val(const uint8_t* const valPtr, size_t valSize);
void cs_write_end();


template<typename T>
void cs_write_size(size_t& size, uint8_t& numArgs, T val) {
	size += sizeof(uart_msg_log_arg_header_t) + sizeof(T);
	++numArgs;
}

template<>
void cs_write_size(size_t& size, uint8_t& numArgs, char* str);

template<>
void cs_write_size(size_t& size, uint8_t& numArgs, const char* str);


template<typename T>
void cs_write_val(T val) {
	const uint8_t* const valPtr = reinterpret_cast<const uint8_t* const>(&val);
	cs_write_val(valPtr, sizeof(T));
}

template<>
void cs_write_val(char* str);

template<>
void cs_write_val(const char* str);





// Uses the fold expression, an easy way to replace a recursive call.
template<class... Args>
void cs_write_args(uint32_t fileNameHash, uint32_t lineNumber, bool addPrefix, bool addNewLine, const Args&... args) {
	uart_msg_log_header_t header;
	header.fileNameHash = fileNameHash;
	header.lineNumber = lineNumber;
	header.flags.prefix = addPrefix;
	header.flags.newLine = addNewLine;
	header.numArgs = 0;
	size_t totalSize = sizeof(header);
	(cs_write_size(totalSize, header.numArgs, args), ...);
//	cs_write("hash=%u line=%u numArgs=%u size=%u", header.fileNameHash, header.lineNumber, header.numArgs, totalSize);
	cs_write_start(totalSize);
	cs_write_header(header);
	(cs_write_val(args), ...);
	cs_write_end();
}
