/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <encryption/cs_AES.h>
#include <events/cs_EventListener.h>
#include <protocol/cs_UartProtocol.h>
#include <uart/cs_UartCommandHandler.h>

#define UART_RX_BUFFER_SIZE 192
#define UART_TX_BUFFER_SIZE 300
#define UART_TX_ENCRYPTION_BUFFER_SIZE AES_BLOCK_SIZE
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
	void init(serial_enable_t serialEnabled);

	/**
	 * Write a msg over UART.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] data       Pointer to the msg to be sent.
	 * @param[in] size       Size of the msg.
	 * @param[in] encrypt    How to encrypt the msg.
	 */
	ret_code_t writeMsg(
			UartOpcodeTx opCode,
			uint8_t* data,
			uint16_t size,
			UartProtocol::Encrypt encrypt = UartProtocol::ENCRYPT_ACCORDING_TO_TYPE);

	/**
	 * Convenience method to write a msg over UART without payload data.
	 */
	ret_code_t writeMsg(UartOpcodeTx opCode);

	/**
	 * Write a msg over UART in a streaming manner.
	 * Must be followed by 1 or more writeMsgPart(), followed by 1 writeMsgEnd().
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] size       Size of the msg.
	 * @param[in] encrypt    How to encrypt the msg.
	 */
	ret_code_t writeMsgStart(
			UartOpcodeTx opCode,
			uint16_t size,
			UartProtocol::Encrypt encrypt = UartProtocol::ENCRYPT_ACCORDING_TO_TYPE);

	/**
	 * Write a msg over UART in a streaming manner.
	 * After writing the start of the msg, stream the data with calls to this functions.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] data       Pointer to the data part to be sent.
	 * @param[in] size       Size of this data part.
	 * @param[in] encrypt    How to encrypt the msg.
	 */
	ret_code_t writeMsgPart(
			UartOpcodeTx opCode,
			const uint8_t* const data,
			uint16_t size,
			UartProtocol::Encrypt encrypt = UartProtocol::ENCRYPT_ACCORDING_TO_TYPE);

	/**
	 * Write a msg over UART in a streaming manner.
	 * This finalizes the stream.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] encrypt    How to encrypt the msg.
	 */
	ret_code_t writeMsgEnd(
			UartOpcodeTx opCode, UartProtocol::Encrypt encrypt = UartProtocol::ENCRYPT_ACCORDING_TO_TYPE);

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
	UartHandler()                      = default;

	//! This class is singleton, deny implementation
	UartHandler(UartHandler const&)    = delete;

	//! This class is singleton, deny implementation
	void operator=(UartHandler const&) = delete;

	// Keep up the state
	bool _initialized                  = false;

	UartCommandHandler _commandHandler;

	//////// RX variables ////////

	//! Pointer to the read buffer
	uint8_t* _readBuffer             = nullptr;

	//! Where to read the next byte into the read buffer
	uint16_t _readBufferIdx          = 0;

	//! Keeps up whether we started reading into the read buffer
	bool _startedReading             = false;

	//! Keeps up whether to escape the next read byte
	bool _escapeNextByte             = false;

	/**
	 * Size of the msg to read, including header and tail, excluding start byte.
	 * Once set, the read buffer index is set to 0.
	 */
	uint16_t _sizeToRead             = 0;

	//! Whether reading is busy (if true, can't read anything, until the read buffer was processed)
	bool _readBusy                   = false;

	//////// TX variables ////////

	//! Write buffer. Currently only used as result buffer for control commands.
	uint8_t* _writeBuffer            = nullptr;

	/**
	 * Encryption buffer. Used to encrypt outgoing msgs.
	 *
	 * Only has to be 1 block size large, as we can stream the writes.
	 */
	uint8_t* _encryptionBuffer       = nullptr;

	//! Number of bytes written to the encryption buffer currently.
	uint8_t _encryptionBufferWritten = 0;

	//! Number of blocks encrypted so far.
	uint8_t _encryptionBlocksWritten = 0;

	//! Packet nonce to use for writing current msg.
	encryption_nonce_t _writeNonce;

	//! Keeps up the crc so far.
	uint16_t _crc;

	//! Stone ID, part of the msg header.
	TYPIFY(CONFIG_CROWNSTONE_ID) _stoneId = 0;

	/**
	 * Write the start byte.
	 */
	cs_ret_code_t writeStartByte();

	/**
	 * Write bytes to UART.
	 *
	 * Values get escaped when necessary.
	 *
	 * @param[in] data       Data to write to UART.
	 * @param[in] updateCrc  Whether to update the CRC with thise data.
	 * @return               Result code.
	 */
	cs_ret_code_t writeBytes(cs_data_t data, bool updateCrc);

	/**
	 * Writes wrapper header (including start and size), and initializes CRC.
	 */
	cs_ret_code_t writeWrapperStart(UartMsgType msgType, uint16_t payloadSize);

	/**
	 * Whether to encrypt an outgoing msg.
	 */
	bool mustEncrypt(UartProtocol::Encrypt encrypt, UartOpcodeTx opCode);

	/**
	 * Whether an incoming msg must've been encrypted.
	 */
	bool mustBeEncrypted(UartOpcodeRx opCode);

	/**
	 * Get required buffer size, given the size of a uart msg.
	 *
	 * @param[in] uartMsgSize     Size of the msg to encrypt.
	 * @return                    Size of the buffer, includes size of encryption headers.
	 */
	cs_buffer_size_t getEncryptedBufferSize(cs_buffer_size_t uartMsgSize);

	/**
	 * Write encrypted header, and update CRC.
	 *
	 * @param[in] uartMsgSize     Size of the msg to encrypt.
	 * @return                    Result code.
	 */
	cs_ret_code_t writeEncryptedStart(cs_buffer_size_t uartMsgSize);

	/**
	 * Encrypt given data and write it to UART.
	 *
	 * Also updates the CRC.
	 *
	 * @param[in] data       Data to encrypt and write.
	 * @return               Return code.
	 */
	cs_ret_code_t writeEncryptedPart(cs_data_t data);

	/**
	 * Write last encryption block, and update CRC.
	 */
	cs_ret_code_t writeEncryptedEnd();

	/**
	 * Encrypt 1 block of data from encryption buffer, update CRC, and write to uart.
	 *
	 * @param[in] key        Key to use for encryption.
	 * @return               Return code.
	 */
	cs_ret_code_t writeEncryptedBlock(cs_data_t key);

	/**
	 * Write an error reply: status.
	 */
	void writeErrorReplyStatus();

	/**
	 * Handles read msgs.
	 *
	 * Data starts after size header, and includes wrapper header and tail (CRC).
	 */
	void handleMsg(uint8_t* data, uint16_t size);

	/**
	 * Handles encrypted UART msg.
	 *
	 * Data includes encryption header.
	 */
	void handleEncryptedUartMsg(uint8_t* data, uint16_t size);

	/**
	 * Handles unencrypted UART msg.
	 *
	 * Data includes uart msg header.
	 */
	void handleUartMsg(uint8_t* data, uint16_t size, EncryptionAccessLevel accessLevel);

	/**
	 * Reset the read buffer and status.
	 */
	void resetReadBuf();

	/**
	 * Handle events as EventListener.
	 */
	void handleEvent(event_t& event);
};
