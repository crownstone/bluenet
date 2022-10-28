/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 1, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdio>
#include <iostream>

__attribute__((unused)) static bool _logPrefixHost = true;

// use out of range for reset.
constexpr void setPrintfColor(int verbosity) {
	switch (verbosity) {
		case SERIAL_VERY_VERBOSE: std::printf("\033[39m"); break;
		case SERIAL_VERBOSE: std::printf("\033[90m"); break;
		case SERIAL_DEBUG: std::printf("\033[39m"); break;
		case SERIAL_INFO: std::printf("\033[34m"); break;
		case SERIAL_WARN: std::printf("\033[33m"); break;
		case SERIAL_ERROR: std::printf("\033[31m"); break;
		case SERIAL_FATAL: std::printf("\033[31m"); break;
		default: std::printf("\033[39m"); break;
	}
}

#define _FILE (sizeof(__FILE__) > 30 ? __FILE__ + (sizeof(__FILE__) - 30 - 1) : __FILE__)

#define _log(level, addNewLine, fmt, ...)                       \
	if (level <= SERIAL_VERBOSITY) {                            \
		if (_logPrefixHost) {                                   \
			std::printf("[%-30.30s : %-4d] ", _FILE, __LINE__); \
		}                                                       \
		setPrintfColor(level);                                  \
		std::printf(fmt, ##__VA_ARGS__);                        \
		setPrintfColor(-1);                                     \
		if (addNewLine) {                                       \
			std::printf("\r\n");                                \
		}                                                       \
		_logPrefixHost = addNewLine;                            \
	}

#define _logArray(level, addNewLine, pointer, size, ...)
