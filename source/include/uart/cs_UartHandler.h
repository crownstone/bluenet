/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <protocol/cs_UartProtocol.h>
#include <uart/cs_UartCommandHandler.h>


#define UART_RX_BUFFER_SIZE            128
#define UART_TX_BUFFER_SIZE            300
//#define UART_TX_MAX_PAYLOAD_SIZE       500



/**
 * Class that implements the binary UART protocol.
 * - Wraps messages.
 * - Parses incoming UART messages.
 * - Handles internal events, to write UART messages.
 * - Encrypts outgoing UART messages.
 * - Decrypts incoming UART messages.
 */
class UartHandler : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation) of this class.
	static UartHandler& getInstance() {
		static UartHandler instance;
		return instance;
	}

	/**
	 * Initialize the class.
	 *
	 * - Allocates memory.
	 * - Reads settings from State.
	 * - Starts listening for events
	 */
	void init();

	/**
	 * Write a msg over UART.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] data       Pointer to the msg to be sent.
	 * @param[in] size       Size of the msg.
	 */
	void writeMsg(UartOpcodeTx opCode, uint8_t * data, uint16_t size);

	/**
	 * Write a msg over UART in a streaming manner.
	 * Must be followed by 1 or more writeMsgPart(), followed by 1 writeMsgEnd().
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] size       Size of the msg.
	 */
	void writeMsgStart(UartOpcodeTx opCode, uint16_t size);

	/**
	 * Write a msg over UART in a streaming manner.
	 * After writing the start of the msg, stream the data with calls to this functions.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] data       Pointer to the data part to be sent.
	 * @param[in] size       Size of this data part.
	 */
	void writeMsgPart(UartOpcodeTx opCode, uint8_t * data, uint16_t size);

	/**
	 * Write a msg over UART in a streaming manner.
	 * This finalizes the stream.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 */
	void writeMsgEnd(UartOpcodeTx opCode);

	/**
	 * To be called when a byte was read. Can be called from interrupt.
	 *
	 * @param[in] val        Value that was read.
	 */
	void onRead(uint8_t val);

	/**
	 * Handles read msgs (private function)
	 *
	 * Message data starts after START byte, and includes the tail (CRC).
	 * Wrapper header size has been checked already.
	 */
	void handleMsg(cs_data_t* msgData);

private:
	//! Constructor
	UartHandler();

	//! This class is singleton, deny implementation
	UartHandler(UartHandler const&) = delete;

	//! This class is singleton, deny implementation
	void operator=(UartHandler const &) = delete;

	// Keep up the state
	bool _initialized = false;

	UartCommandHandler _commandHandler;


	//////// RX variables ////////

	//! Pointer to the read buffer
	uint8_t* _readBuffer = nullptr;

	//! Where to read the next byte into the read buffer
	uint16_t _readBufferIdx = 0;

	//! Keeps up whether we started reading into the read buffer
	bool _startedReading = false;

	//! Keeps up whether to escape the next read byte
	bool _escapeNextByte = false;

	/**
	 * Size of the msg to read, including header and tail, excluding start byte.
	 * Once set, the read buffer index is set to 0.
	 */
	uint16_t _sizeToRead = 0;

	//! Whether reading is busy (if true, can't read anything, until the read buffer was processed)
	bool _readBusy = false;


	//////// TX variables ////////

	//! Write buffer. Currently only used as result buffer for control commands.
	uint8_t* _writeBuffer = nullptr;

	//! Keeps up the crc so far.
	uint16_t _crc;

	//! Stone ID, part of the msg header.
	TYPIFY(CONFIG_CROWNSTONE_ID) _stoneId;

	/**
	 * Handles read msgs.
	 *
	 * Message data starts after START byte, and includes the tail (CRC).
	 * Wrapper header size has been checked already.
	 */
	void handleMsg(uint8_t* data, uint16_t size);

	/**
	 * Reset the read buffer and status.
	 */
	void resetReadBuf();

	/**
	 * Handle events as EventListener.
	 */
	void handleEvent(event_t & event);
};


