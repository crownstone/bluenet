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

#define _FILE (sizeof(__FILE__) > 30 ? __FILE__ + (sizeof(__FILE__) - 30 - 1) : __FILE__)

#define _log(level, addNewLine, fmt, ...)                       \
	if (level <= SERIAL_VERBOSITY) {                            \
		if (_logPrefixHost) {                                   \
			std::printf("[%-30.30s : %-4d] ", _FILE, __LINE__); \
		}                                                       \
		std::printf(fmt, ##__VA_ARGS__);                        \
		if (addNewLine) {                                       \
			std::printf("\r\n");                                \
		}                                                       \
		_logPrefixHost = addNewLine;                            \
	}

#define _logArray(level, addNewLine, pointer, size, ...)
