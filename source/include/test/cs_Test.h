/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 15, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <drivers/cs_Serial.h>
#include <protocol/cs_UartProtocol.h>

#include <cstdarg>


//     UartProtocol::getInstance().writeMsgStart(UART_OPCODE_TX_POWER_LOG_CURRENT, sizeof(uart_msg_current_t));
//     UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_POWER_LOG_CURRENT,(uint8_t*)&(rtcCount), sizeof(rtcCount));
//     UartProtocol::getInstance().writeMsgEnd(UART_OPCODE_TX_POWER_LOG_CURRENT);
// }

inline int cs_write_test(const char *str, ...) {
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
		UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_FIRMWARESTATE, (uint8_t*)buffer, len);
	} else {
		char *p_buf = (char*)malloc(len + 1);
		if (!p_buf) return -1;
		va_start(ap, str);
		len = vsprintf(p_buf, str, ap);
		va_end(ap);
		UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_FIRMWARESTATE, (uint8_t*)p_buf, len);
		free(p_buf);
	}
	return len;
}

// format:
// ptr to this,prettyfunction,valuename,content

#define TEST_PUSH_EXPR_S(self, expressionnamestr, expr) cs_write_test("%x,%s,%s,%s", self, __PRETTY_FUNCTION__, expressionnamestr, expr)
#define TEST_PUSH_EXPR_D(self, expressionnamestr, expr) cs_write_test("%x,%s,%s,%d", self, __PRETTY_FUNCTION__, expressionnamestr, expr)
#define TEST_PUSH_EXPR_X(self, expressionnamestr, expr) cs_write_test("%x,%s,%s,%x", self, __PRETTY_FUNCTION__, expressionnamestr, expr)

#define TEST_PUSH_D(self, variablename) TEST_PUSH_EXPR_D(self, #variablename, self->variablename)
#define TEST_PUSH_X(self, variablename) TEST_PUSH_EXPR_X(self, #variablename, self->variablename)
#define TEST_PUSH_S(self, variablename) TEST_PUSH_EXPR_S(self, #variablename, self->variablename)