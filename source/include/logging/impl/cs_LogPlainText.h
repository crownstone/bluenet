/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 1, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

void cs_log_printf(const char* str, ...);
__attribute__((unused)) static bool _logPrefixPlainText = true;

// Adding the file name and line number, adds a lot to the binary size.
#define _FILE (sizeof(__FILE__) > 30 ? __FILE__ + (sizeof(__FILE__) - 30 - 1) : __FILE__)

#undef _log
#define _log(level, addNewLine, fmt, ...)                         \
	if (level <= SERIAL_VERBOSITY) {                              \
		if (_logPrefixPlainText) {                                \
			cs_log_printf("[%-30.30s : %-4d] ", _FILE, __LINE__); \
		}                                                         \
		cs_log_printf(fmt, ##__VA_ARGS__);                        \
		if (addNewLine) {                                         \
			cs_log_printf("\r\n");                                \
		}                                                         \
		_logPrefixPlainText = addNewLine;                         \
	}

#undef _logArray
#define _logArray(level, addNewLine, pointer, size, ...)
