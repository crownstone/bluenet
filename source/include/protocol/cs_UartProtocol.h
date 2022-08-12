/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 17, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/cs_UartMsgTypes.h>
#include <protocol/cs_UartOpcodes.h>

#include <cstdint>

// bit:  7654 3210
#define UART_START_BYTE 0x7E        // "~"   0111 1110  resets the state
#define UART_ESCAPE_BYTE 0x5C       // "\"   0101 1100  next byte gets bits flipped that are in flip mask
#define UART_ESCAPE_FLIP_MASK 0x40  //       0100 0000

#define UART_PROTOCOL_MAJOR 1
#define UART_PROTOCOL_MINOR 0

#define UART_PROTOCOL_VALIDATION 0xCAFEBABE

enum class UartMsgType : uint8_t {
	UART_MSG           = 0,    // Payload is: uart_msg_header + data
	ENCRYPTED_UART_MSG = 128,  // Payload is: uart_encrypted_msg_header + encrypted blocks
};

/**
 * Very first header, right after the start byte.
 * Only used to determine how many bytes have to be read, so only has size as field.
 */
struct __attribute__((__packed__)) uart_msg_size_header_t {
	uint16_t size = 0;  // Size of all remaining bytes, including tail.
};

/**
 * Wrapper header, after the initial size field.
 */
struct __attribute__((__packed__)) uart_msg_wrapper_header_t {
	uint8_t protocolMajor = UART_PROTOCOL_MAJOR;
	uint8_t protocolMinor = UART_PROTOCOL_MINOR;
	uint8_t type          = 0;
	// Followed by payload
};

/**
 * Non-encrypted header for encrypted uart msg.
 */
struct __attribute__((__packed__)) uart_encrypted_msg_header_t {
	uint8_t packetNonce[PACKET_NONCE_LENGTH];
	uint8_t keyId;
	// Followed by encrypted data
};

/**
 * Encrypted header for encrypted uart msg.
 */
struct __attribute__((__packed__)) uart_encrypted_data_header_t {
	uint32_t validation;
	uint16_t size;  // Size of uart msg.
					// Followed by uart msg and padding.
};

/**
 * UART message, either non-encrypted, or after decryption.
 */
struct __attribute__((__packed__)) uart_msg_header_t {
	uint16_t type;  // Type of message, see ..
					// Followed by data.
};

/**
 * Final bytes.
 */
struct __attribute__((__packed__)) uart_msg_tail_t {
	uint16_t crc;  // The CRC16 (CRC-16-CCITT) of everything after the size field.
};

namespace UartProtocol {
/**
 * Escape a value: converts a value to the an escaped version.
 */
void escape(uint8_t& val);

/**
 * Unescape a value: converts an escaped value back to the original value.
 */
void unEscape(uint8_t& val);

/**
 * Calculate the CRC of given data.
 */
uint16_t crc16(const uint8_t* data, uint16_t size);

/**
 * Calculate the CRC of given data, with given CRC as start.
 */
void crc16(const uint8_t* data, const uint16_t size, uint16_t& crc);
};  // namespace UartProtocol
