/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <uart/cs_UartHandler.h>
#include <cstring>
#include <cstdarg>

#if SERIAL_VERBOSITY > SERIAL_BYTE_PROTOCOL_ONLY
static char _logBuffer[128];
#endif

/**
 * A write function with a format specifier.
 */
int cs_write(const char *str, ...) {
#if SERIAL_VERBOSITY > SERIAL_BYTE_PROTOCOL_ONLY
//	if (!_initializedTx) {
//		return 0;
//	}
	va_list ap;
	va_start(ap, str);
	int16_t len = vsnprintf(NULL, 0, str, ap);
	va_end(ap);

	if (len < 0) return len;

	// if strings are small we do not need to allocate by malloc
	if (sizeof(_logBuffer) >= len + 1UL) {
		va_start(ap, str);
		len = vsprintf(_logBuffer, str, ap);
		va_end(ap);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_TEXT, (uint8_t*)_logBuffer, len);
	} else {
		char *p_buf = (char*)malloc(len + 1);
		if (!p_buf) return -1;
		va_start(ap, str);
		len = vsprintf(p_buf, str, ap);
		va_end(ap);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_TEXT, (uint8_t*)p_buf, len);
		free(p_buf);
	}
	return len;
#endif
	return 0;
}



template<>
void cs_add_arg_size(size_t& size, uint8_t& numArgs, char* str) {
	size += sizeof(uart_msg_log_arg_header_t) + strlen(str);
	++numArgs;
}

template<>
void cs_add_arg_size(size_t& size, uint8_t& numArgs, const char* str) {
	size += sizeof(uart_msg_log_arg_header_t) + strlen(str);
	++numArgs;
}

template<>
void cs_write_arg(char* str) {
	const uint8_t* const valPtr = reinterpret_cast<const uint8_t* const>(str);
	cs_write_arg(valPtr, strlen(str));
}

template<>
void cs_write_arg(const char* str) {
	const uint8_t* const valPtr = reinterpret_cast<const uint8_t* const>(str);
	cs_write_arg(valPtr, strlen(str));
}

void cs_write_start(size_t msgSize, uart_msg_log_header_t &header) {
	UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_LOG, msgSize);
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG, reinterpret_cast<uint8_t*>(&header), sizeof(header));
}

void cs_write_arg(const uint8_t* const valPtr, size_t valSize) {
	uart_msg_log_arg_header_t argHeader;
	argHeader.argSize = valSize;
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG, reinterpret_cast<uint8_t*>(&argHeader), sizeof(argHeader));
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG, valPtr, valSize);
}

void cs_write_end() {
	UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_LOG);
}

void cs_write_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const uint8_t* const ptr, size_t size, ElementType elementType, size_t elementSize) {
	uart_msg_log_array_header_t header;
	header.header.fileNameHash = fileNameHash;
	header.header.lineNumber = lineNumber;
	header.header.logLevel = logLevel;
	header.header.flags.newLine = addNewLine;
	header.elementType = elementType;
	header.elementSize = elementSize;
	uint16_t msgSize = sizeof(header) + size;
	UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_LOG_ARRAY, msgSize);
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG_ARRAY, reinterpret_cast<uint8_t*>(&header), sizeof(header));
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG_ARRAY, ptr, size);
	UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_LOG_ARRAY);
}
