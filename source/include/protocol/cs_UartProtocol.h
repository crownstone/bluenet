/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 17, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
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

#include <events/cs_EventListener.h>
#include <protocol/cs_UartMsgTypes.h>
#include <protocol/cs_UartOpcodes.h>
#include <cstdint>

                                           // bit:7654 3210
#define UART_START_BYTE           0x7E // "~"   0111 1110  resets the state
#define UART_ESCAPE_BYTE          0x5C // "\"   0101 1100  next byte gets bits flipped that are in flip mask
#define UART_ESCAPE_FLIP_MASK     0x40 //       0100 0000



struct __attribute__((__packed__)) uart_msg_header_t {
	uint16_t opCode;
	uint16_t size; //! Size of the payload
};

struct __attribute__((__packed__)) uart_msg_tail_t {
	uint16_t crc;
};

#define UART_RX_BUFFER_SIZE            128
#define UART_RX_MAX_PAYLOAD_SIZE       (UART_RX_BUFFER_SIZE - sizeof(uart_msg_header_t) - sizeof(uart_msg_tail_t))

#define UART_TX_MAX_PAYLOAD_SIZE       500


/** Struct storing data for handle msg callback.
 *
 * This struct is the data sent with the callback from the app scheduler.
 * It should not contain large chunks of data, as all data is copied.
 */
struct uart_handle_msg_data_t {
	uint8_t* msg;  //! Pointer to the msg.
	uint16_t msgSize; //! Size of the msg.
};


class UartProtocol : EventListener {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static UartProtocol& getInstance() {
		static UartProtocol instance;
		return instance;
	}

	void init();

	/** Write a binary msg over UART.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] data       Pointer to the msg to be sent.
	 * @param[in] size       Size of the msg.
	 */
	void writeMsg(UartOpcodeTx opCode, uint8_t * data, uint16_t size);

	/** Write a binary msg over UART.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] size       Size of the msg.
	 */
	void writeMsgStart(UartOpcodeTx opCode, uint16_t size);

	/** Write a binary msg over UART.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] data       Pointer to the data part to be sent.
	 * @param[in] size       Size of this data part.
	 */
	void writeMsgPart(UartOpcodeTx opCode, uint8_t * data, uint16_t size);

	/** Write the end of a binary msg over UART.
	 */
	void writeMsgEnd(UartOpcodeTx opCode);

	/** To be called when a byte was read. Can be called from interrupt
	 *
	 * @param[in] val        Value that was read.
	 */
	void onRead(uint8_t val);

	/** Handles read msgs (private function)
	 *
	 */
	void handleMsg(uart_handle_msg_data_t* msgData);

private:
	//! Constructor
	UartProtocol();

	//! This class is singleton, deny implementation
	UartProtocol(UartProtocol const&);

	//! This class is singleton, deny implementation
	void operator=(UartProtocol const &);

	// Keep up the state
	bool _initialized;

	// RX variables
	uint8_t* _readBuffer;     //! Pointer to the read buffer
	uint16_t _readBufferIdx;   //! Where to read the next byte into the read buffer
	bool _startedReading;     //! Keeps up whether we started reading into the read buffer
	bool _escapeNextByte;     //! Keeps up whether to escape the next read byte
	uint16_t _readPacketSize; //! Size of the msg to read, including header and tail.
	bool _readBusy;           //! Whether reading is busy (if true, can't read anything, until the read buffer was processed)

	// TX variables
//	bool _msgStarted;         //! True when a msg has been started to be written.
//	uint16_t _writeSize;      //! Keeps up the size of the payload.
//	uint16_t _writtenBytes;   //! Keeps up how many bytes payload have been written so far.
	uint16_t _crc;            //! Keeps up the crc so far.

	void reset();

	//! Escape a value
	void escape(uint8_t& val);

	//! Unescape a value
	void unEscape(uint8_t& val);

	uint16_t crc16(const uint8_t * data, uint16_t size);
	void crc16(const uint8_t * data, const uint16_t size, uint16_t& crc);

	// Handle events as EventListener.
	void handleEvent(event_t & event);
};
