/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 17, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <protocol/cs_UartProtocol.h>

void UartProtocol::escape(uint8_t& val) {
	val ^= UART_ESCAPE_FLIP_MASK;
}

void UartProtocol::unEscape(uint8_t& val) {
	// No logs, this function can be called from interrupt
	val ^= UART_ESCAPE_FLIP_MASK;
}

uint16_t UartProtocol::crc16(const uint8_t* data, uint16_t size) {
	return crc16_compute(data, size, NULL);
}

void UartProtocol::crc16(const uint8_t* data, const uint16_t size, uint16_t& crc) {
	crc = crc16_compute(data, size, &crc);
}
