/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_CLogger.h>
#include <drivers/cs_Serial.h>
#include <stdarg.h>

#define C_LOGGER_BUF_SIZE 16

#if SERIAL_VERBOSITY > SERIAL_BYTE_PROTOCOL_ONLY

// Buffer we need to convert a number to a string, as that string is built up from the end.
static char _logBuffer[C_LOGGER_BUF_SIZE];

void cs_clog_write_uint_dec(unsigned int value) {
	size_t i = C_LOGGER_BUF_SIZE;
	do {
		const char digit = (char)(value % 10) + '0';
		_logBuffer[--i] = digit;
		value /= 10;
	} while (value && i);

	for (; i < C_LOGGER_BUF_SIZE; ++i) {
		serial_write(_logBuffer[i]);
	}
}

void cs_clog_write_uint_hex(unsigned int value) {
	size_t i = C_LOGGER_BUF_SIZE;
	do {
		char digit = (char)(value % 16);
		if (digit < 10) {
			digit += '0';
		}
		else {
			digit += 'A' - 10;
		}
		_logBuffer[--i] = digit;
		value /= 16;
	} while (value && i);

	for (; i < C_LOGGER_BUF_SIZE; ++i) {
		serial_write(_logBuffer[i]);
	}
}


void cs_clog(bool addNewLine, const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);

	while (*fmt != 0) {
		if (*fmt != '%') {
			serial_write(*fmt);
			fmt++;
			continue;
		}
		else {
			++fmt;
		}
		if (*fmt == 0) {
			break;
		}
		switch (*fmt) {
			case 'd':
			case 'i': {
				const int value = va_arg(va, int);
				if (value < 0) {
					serial_write('-');
					cs_clog_write_uint_dec(0 - value);
				}
				else {
					cs_clog_write_uint_dec(value);
				}
				break;
			}
			case 'u': {
				const unsigned int value = va_arg(va, unsigned int);
				cs_clog_write_uint_dec(value);
				break;
			}
			case 'p': {
				const unsigned int value = (unsigned long)((uintptr_t)va_arg(va, void*));
				cs_clog_write_uint_hex(value);
				break;
			}
			case 'x':
			case 'X': {
				const unsigned int value = va_arg(va, unsigned int);
				cs_clog_write_uint_hex(value);
				break;
			}
			case 's': {
				char* str = va_arg(va, char*);
				while (*str != 0) {
					serial_write(*str);
					++str;
				}
				break;
			}
			case '%': {
				serial_write('%');
				break;
			}
			default: {
				serial_write(*fmt);
				break;
			}
		}
		++fmt;
	}
	if (addNewLine) {
		serial_write('\r');
		serial_write('\n');
	}
	va_end(va);
}

#endif
