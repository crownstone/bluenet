/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <uart/cs_UartHandler.h>
#include <cstdarg>
#if CS_SERIAL_NRF_LOG_ENABLED == 0

#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
	#include <cstring>

	#if SERIAL_VERBOSITY > SERIAL_BYTE_PROTOCOL_ONLY
	static char _logBuffer[128];
	#endif

	void cs_log_printf(const char *str, ...) {
	#if SERIAL_VERBOSITY > SERIAL_BYTE_PROTOCOL_ONLY
		va_list ap;
		va_start(ap, str);
		int len = vsnprintf(_logBuffer, sizeof(_logBuffer), str, ap);
		va_end(ap);

		if (len < 0) {
			return;
		}
		for (int i = 0; i < len; ++i) {
			serial_write(_logBuffer[i]);
		}
		return;
	#endif
		return;
	}

#endif // CS_UART_BINARY_PROTOCOL_ENABLED == 0


template<>
void cs_log_add_arg_size(size_t& size, uint8_t& numArgs, char* str) {
	size += sizeof(uart_msg_log_arg_header_t) + strlen(str);
	++numArgs;
}

template<>
void cs_log_add_arg_size(size_t& size, uint8_t& numArgs, const char* str) {
	size += sizeof(uart_msg_log_arg_header_t) + strlen(str);
	++numArgs;
}

template<>
void cs_log_arg(char* str) {
	const uint8_t* const valPtr = reinterpret_cast<const uint8_t* const>(str);
	cs_log_arg(valPtr, strlen(str));
}

template<>
void cs_log_arg(const char* str) {
	const uint8_t* const valPtr = reinterpret_cast<const uint8_t* const>(str);
	cs_log_arg(valPtr, strlen(str));
}

void cs_log_start(size_t msgSize, uart_msg_log_header_t &header) {
	UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_LOG, msgSize);
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG, reinterpret_cast<uint8_t*>(&header), sizeof(header));
}

void cs_log_arg(const uint8_t* const valPtr, size_t valSize) {
	uart_msg_log_arg_header_t argHeader;
	argHeader.argSize = valSize;
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG, reinterpret_cast<uint8_t*>(&argHeader), sizeof(argHeader));
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_LOG, valPtr, valSize);
}

void cs_log_end() {
	UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_LOG);
}

void cs_log_array(uint32_t fileNameHash, uint32_t lineNumber, uint8_t logLevel, bool addNewLine, const uint8_t* const ptr, size_t size, ElementType elementType, size_t elementSize) {
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

#endif // CS_SERIAL_NRF_LOG_ENABLED == 0

