/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 10 Oct., 2014
 * License: LGPLv3+
 */

#ifndef CS_SERIAL_H
#define CS_SERIAL_H

/*
 * Commonly LOG functionality is provided with as first paramater the level of severity of the message. Subsequently
 * the message follows, eventually succeeded by content if the string contains format specifiers. This means that this
 * requires a variadic macro as well, see http://www.wikiwand.com/en/Variadic_macro. The two ## are e.g. a gcc
 * specific extension that removes the , after __LINE__, so log(level, msg) can also be used without arguments.
 */
#define DEBUG                0
#define INFO                 1
#define WARN                 2
#define ERROR                3
#define FATAL                4

#define DEBUG_ON

#ifdef DEBUG_ON
#define log(level, fmt, ...) \
       write("[%s:%d] " fmt "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define log(level, fmt, ...) 
#endif

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

#endif
