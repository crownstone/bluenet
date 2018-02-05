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

enum UartOpcodeRx {
	UART_OPCODE_RX_CONTROL =                          1,
	UART_OPCODE_RX_ENABLE_ADVERTISEMENT =             10000, // Enable advertising (payload: bool enable)
	UART_OPCODE_RX_ENABLE_MESH =                      10001, // Enable mesh (payload: bool enable)

//	UART_OPCODE_RX_ADC_CONFIG_GET =                   10100, // Get the adc config
//	UART_OPCODE_RX_ADC_CONFIG_SET =                   10101, // Set an adc channel config (payload: uart_msg_adc_channel_config_t)
//	UART_OPCODE_RX_ADC_CONFIG_SET_RANGE =             10102, // Set range of given channel (payload: channel, range)
	UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_CURRENT =     10103, // Increase the range on the current channel
	UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_CURRENT =     10104, // Decrease the range on the current channel
	UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_VOLTAGE =     10105, // Increase the range on the voltage channel
	UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_VOLTAGE =     10106, // Decrease the range on the voltage channel
	UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_CURRENT =  10108, // Enable differential mode on current channel (payload: bool enable)
	UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_VOLTAGE =  10109, // Enable differential mode on voltage channel (payload: bool enable)
	UART_OPCODE_RX_ADC_CONFIG_VOLTAGE_PIN =           10110, // Change the pin used on voltage channel (payload: enum of pins)

	UART_OPCODE_RX_POWER_LOG_CURRENT =                10200, // Enable writing current samples (payload: bool enable)
	UART_OPCODE_RX_POWER_LOG_VOLTAGE =                10201, // Enable writing voltage samples (payload: bool enable)
	UART_OPCODE_RX_POWER_LOG_FILTERED_CURRENT =       10202, // Enable writing filtered current samples (payload: bool enable)
//	UART_OPCODE_RX_POWER_LOG_FILTERED_VOLTAGE =       10203, // Enable writing filtered voltage samples (payload: bool enable)
	UART_OPCODE_RX_POWER_LOG_POWER =                  10204, // Enable writing calculated power (payload: bool enable)
};

enum UartOpcodeTx {
	UART_OPCODE_TX_ACK = 0,
	UART_OPCODE_TX_MESH_STATE_0 = 100, // For 1st handle, next handle has opcode of 1 larger.

	UART_OPCODE_TX_ADC_CONFIG =                       10100, // Current adc config (payload: adc_config_t)

	UART_OPCODE_TX_POWER_LOG_CURRENT =                10200,
	UART_OPCODE_TX_POWER_LOG_VOLTAGE =                10201,
	UART_OPCODE_TX_POWER_LOG_FILTERED_CURRENT =       10202,
	UART_OPCODE_TX_POWER_LOG_FILTERED_VOLTAGE =       10203,
	UART_OPCODE_TX_POWER_LOG_POWER =                  10204,
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


class UartProtocol {
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
	 * @param[in] data       Pointer to the msg to be sent.
	 * @param[in] size       Size of the msg.
	 */
	void writeMsgStart(UartOpcodeTx opCode, uint16_t size);

	/** Write a binary msg over UART.
	 *
	 * @param[in] opCode     OpCode of the msg.
	 * @param[in] data       Pointer to the msg to be sent.
	 * @param[in] size       Size of the msg.
	 */
	void writeMsgPart(uint8_t * data, uint16_t size);

	/** Write the end of a binary msg over UART.
	 */
	void writeMsgEnd();


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
};
