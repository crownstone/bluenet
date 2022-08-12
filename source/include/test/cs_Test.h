/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 15, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <uart/cs_UartHandler.h>

#include <cstdarg>

#ifdef DEBUG
inline int cs_write_test(const char* str, ...) {
	char buffer[128];
	va_list ap;
	va_start(ap, str);
	int16_t len = vsnprintf(NULL, 0, str, ap);
	va_end(ap);

	if (len < 0) return len;

	// if strings are small we do not need to allocate by malloc
	if (sizeof buffer >= len + 1UL) {
		va_start(ap, str);
		len = vsprintf(buffer, str, ap);
		va_end(ap);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_FIRMWARESTATE, (uint8_t*)buffer, len);
	}
	else {
		char* p_buf = (char*)malloc(len + 1);
		if (!p_buf) return -1;
		va_start(ap, str);
		len = vsprintf(p_buf, str, ap);
		va_end(ap);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_FIRMWARESTATE, (uint8_t*)p_buf, len);
		free(p_buf);
	}
	return len;
}
#else
// when debug is not defined, define the centralized method
// as empty symbol so that all calls to it result in no generated code.
#define cs_write_test(...)
#endif

/**
 * Cast optional into integer for easy printing.
 */
template <class U>
inline long int OptionalUnsignedToInt(std::optional<U> opt) {
	return (opt ? static_cast<long int>(opt.value()) : -1);
}

/**
 * format:
 * ptr_to_this,prettyfunction,valuename,content
 */

// folds in pretty_function name
#define TEST_PUSH_DATA(form, self, expressionnamestr, expr) \
	cs_write_test(form, self, __PRETTY_FUNCTION__, expressionnamestr, expr)

/**
 * Expression wrappers for generic expressions in non-static context
 */
#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
#define TEST_PUSH_EXPR_S(self, expressionnamestr, expr) \
	TEST_PUSH_DATA("%x@%s@%s@%s\r\n", self, expressionnamestr, expr)  // string
#define TEST_PUSH_EXPR_D(self, expressionnamestr, expr) \
	TEST_PUSH_DATA("%x@%s@%s@%d\r\n", self, expressionnamestr, expr)  // decimal
#define TEST_PUSH_EXPR_X(self, expressionnamestr, expr) \
	TEST_PUSH_DATA("%x@%s@%s@%x\r\n", self, expressionnamestr, expr)  // hexadecimal
#else
#define TEST_PUSH_EXPR_S(self, expressionnamestr, expr) \
	TEST_PUSH_DATA("%x@%s@%s@%s", self, expressionnamestr, expr)  // string
#define TEST_PUSH_EXPR_D(self, expressionnamestr, expr) \
	TEST_PUSH_DATA("%x@%s@%s@%d", self, expressionnamestr, expr)  // decimal
#define TEST_PUSH_EXPR_X(self, expressionnamestr, expr) \
	TEST_PUSH_DATA("%x@%s@%s@%x", self, expressionnamestr, expr)  // hexadecimal
#endif

// boolean remap to string
#define TEST_PUSH_EXPR_B(self, expressionnamestr, expr) \
	TEST_PUSH_EXPR_S(self, expressionnamestr, (expr ? "True" : "False"))

// std::optional<uint*_t> remap to Decimal
#define TEST_PUSH_EXPR_O(self, expressionnamestr, expr) \
	TEST_PUSH_EXPR_D(self, expressionnamestr, OptionalUnsignedToInt(expr))

/**
 * Utiltiy wrappers for member variables.
 */
#define TEST_PUSH_S(self, variablename) TEST_PUSH_EXPR_S(self, #variablename, self->variablename)
#define TEST_PUSH_D(self, variablename) TEST_PUSH_EXPR_D(self, #variablename, self->variablename)
#define TEST_PUSH_X(self, variablename) TEST_PUSH_EXPR_X(self, #variablename, self->variablename)
#define TEST_PUSH_B(self, variablename) TEST_PUSH_EXPR_B(self, #variablename, self->variablename)
#define TEST_PUSH_O(self, variablename) TEST_PUSH_EXPR_O(self, #variablename, self->variablename)

/**
 * Utility wrappers for static stuff.
 *
 * (Use with care, may result in conflicts when multiple translation units use the same names)
 */
#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
#define TEST_PUSH_STATIC_S(context, expressionnamestr, expr) \
	TEST_PUSH_DATA("%s@%s@%s@%s\r\n", context, expressionnamestr, expr)
#define TEST_PUSH_STATIC_D(context, expressionnamestr, expr) \
	TEST_PUSH_DATA("%s@%s@%s@%d\r\n", context, expressionnamestr, expr)
#define TEST_PUSH_STATIC_X(context, expressionnamestr, expr) \
	TEST_PUSH_DATA("%s@%s@%s@%x\r\n", context, expressionnamestr, expr)
#define TEST_PUSH_STATIC_B(context, expressionnamestr, expr) \
	TEST_PUSH_DATA("%s@%s@%s@%s\r\n", context, expressionnamestr, (expr ? "True" : "False"))
#else
#define TEST_PUSH_STATIC_S(context, expressionnamestr, expr) \
	TEST_PUSH_DATA("%s@%s@%s@%s", context, expressionnamestr, expr)
#define TEST_PUSH_STATIC_D(context, expressionnamestr, expr) \
	TEST_PUSH_DATA("%s@%s@%s@%d", context, expressionnamestr, expr)
#define TEST_PUSH_STATIC_X(context, expressionnamestr, expr) \
	TEST_PUSH_DATA("%s@%s@%s@%x", context, expressionnamestr, expr)
#define TEST_PUSH_STATIC_B(context, expressionnamestr, expr) \
	TEST_PUSH_DATA("%s@%s@%s@%s", context, expressionnamestr, (expr ? "True" : "False"))
#endif
