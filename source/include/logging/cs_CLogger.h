/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/**
 * Logger class for C code.
 *
 * Generally, you want to use: CLOGv, CLOGd, CLOGi, CLOGw, CLOGe, or CLOGf.
 * These can be used like printf(), but without padding, flags, width, or other specifier.
 *
 * The logs will always be written to UART in plain text.
 *
 * Since the log functions have an unknown amount of arguments, these macros require a variadic macro as well.
 * See http://www.wikiwand.com/en/Variadic_macro.
 * The two ## are e.g. a gcc specific extension that removes the , so that the ... arguments can also be left out.
 */

#include <drivers/cs_Serial.h> // For SERIAL_VERBOSITY.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#if !defined HOST_TARGET && (CS_SERIAL_NRF_LOG_ENABLED == 2)
	// Use NRF Logger instead.
	#define CLOGvv NRF_LOG_DEBUG
	#define CLOGv NRF_LOG_DEBUG
	#define CLOGd NRF_LOG_DEBUG
	#define CLOGi NRF_LOG_INFO
	#define CLOGw NRF_LOG_WARNING
	#define CLOGe NRF_LOG_ERROR
	#define CLOGf NRF_LOG_ERROR
	#define _clog(level, addNewLine, fmt, ...) NRF_LOG_DEBUG(fmt, ##__VA_ARGS__)
#else
	#define _clog(level, addNewLine, fmt, ...) \
		if (level <= SERIAL_VERBOSITY) { \
			cs_clog(addNewLine, fmt, ##__VA_ARGS__); \
		}

	#define CLOGvv(fmt, ...) _clog(SERIAL_VERY_VERBOSE, true, fmt, ##__VA_ARGS__)
	#define CLOGv(fmt, ...) _clog(SERIAL_VERBOSE, true, fmt, ##__VA_ARGS__)
	#define CLOGd(fmt, ...) _clog(SERIAL_DEBUG,   true, fmt, ##__VA_ARGS__)
	#define CLOGi(fmt, ...) _clog(SERIAL_INFO,    true, fmt, ##__VA_ARGS__)
	#define CLOGw(fmt, ...) _clog(SERIAL_WARN,    true, fmt, ##__VA_ARGS__)
	#define CLOGe(fmt, ...) _clog(SERIAL_ERROR,   true, fmt, ##__VA_ARGS__)
	#define CLOGf(fmt, ...) _clog(SERIAL_FATAL,   true, fmt, ##__VA_ARGS__)

	void cs_clog(bool addNewLine, const char* fmt, ...);
#endif

#ifdef __cplusplus
}
#endif


#if SERIAL_VERBOSITY < SERIAL_VERY_VERBOSE
	#undef CLOGvv
	#define CLOGvv(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_VERBOSE
	#undef CLOGv
	#define CLOGv(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_DEBUG
	#undef CLOGd
	#define CLOGd(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_INFO
	#undef CLOGi
	#define CLOGi(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_WARN
	#undef CLOGw
	#define CLOGw(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_ERROR
	#undef CLOGe
	#define CLOGe(fmt, ...)
#endif

#if SERIAL_VERBOSITY < SERIAL_FATAL
	#undef CLOGf
	#define CLOGf(fmt, ...)
#endif
