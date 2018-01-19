/**
 * Author: Crownstone Team
 * Copyright: Crownstone
 * Date: Jan 17, 2018
 * License: LGPLv3+, Apache, or MIT, your choice
 */

/**********************************************************************************************************************
 *
 * The Crownstone is a high-voltage (domestic) switch. It can be used for:
 *   - indoor localization
 *   - building automation
 *
 * It is one of the first, or the first(?), open-source Internet-of-Things devices entering the market.
 *
 * Read more on: https://crownstone.rocks
 *
 * Almost all configuration options should be set in CMakeBuild.config.
 *
 *********************************************************************************************************************/

#pragma once

#include <cstdint>

                                           // bit:7654 3210
#define UART_START_BYTE           0x7E // "~"   0111 1110  resets the state
#define UART_ESCAPE_BYTE          0x5C // "\"   0101 1100  next byte gets bits flipped that are in flip mask
#define UART_ESCAPE_FLIP_MASK     0x40 //       0100 0000

enum UartOpcode {
	UART_OPCODE_CONTROL = 0,
};

struct __attribute__((__packed__)) uart_msg_header_t {
	uint16_t opCode;
	uint16_t size; //! Size of the payload
};

struct __attribute__((__packed__)) uart_msg_tail_t {
	uint16_t crc;
};

#define UART_RX_BUFFER_SIZE            128
#define UART_RX_MAX_PAYLOAD_SIZE       (UART_RX_BUFFER_SIZE - sizeof(uart_msg_header_t) - sizeof(uart_msg_tail_t))

class UartProtocol {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static UartProtocol& getInstance() {
		static UartProtocol instance;
		return instance;
	}

	void init();

	/** To be called when a byte was read. Can be called from interrupt
	 *
	 * @param[in] val        Value that was read.
	 */
	void onRead(uint8_t val);

	void handleMsg(void * data, uint16_t size);

private:
	//! Constructor
	UartProtocol();

	//! This class is singleton, deny implementation
	UartProtocol(UartProtocol const&);

	//! This class is singleton, deny implementation
	void operator=(UartProtocol const &);

	uint8_t* readBuffer;
	uint8_t readBufferIdx;
	bool startedReading;
	bool escapeNextByte;

	//! Size of the msg to read, including header and tail.
	uint16_t readPacketSize;
	bool readBusy;

	void reset();

	//! Escape a value
	void escape(uint8_t& val);

	//! Unescape a value
	void unEscape(uint8_t& val);

	uint16_t crc16(const uint8_t * data, uint16_t size);
	void crc16(const uint8_t * data, const uint16_t size, uint16_t& crc);
};
