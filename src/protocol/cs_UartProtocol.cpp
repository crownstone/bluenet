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

#include "protocol/cs_UartProtocol.h"
#include "util/cs_Utils.h"
#include "protocol/cs_ErrorCodes.h"
#include "util/cs_BleError.h"
#include "ble/cs_Nordic.h"

#include "structs/cs_StreamBuffer.h"
#include "events/cs_EventDispatcher.h"
#include "events/cs_EventTypes.h"
#include "storage/cs_Settings.h"

// Define both test pin to enable gpio debug.
//#define TEST_PIN   22
//#define TEST_PIN2  23

UartProtocol::UartProtocol():
_readBuffer(NULL),
_readBufferIdx(0),
_startedReading(false),
_escapeNextByte(false),
_readPacketSize(0),
_readBusy(false)
{

}

void handle_msg(void * data, uint16_t size) {
	UartProtocol::getInstance().handleMsg((uart_handle_msg_data_t*)data);
}

void UartProtocol::init() {
	_readBuffer = new uint8_t[UART_RX_BUFFER_SIZE];
#ifdef TEST_PIN
    nrf_gpio_cfg_output(TEST_PIN);
#endif
#ifdef TEST_PIN2
    nrf_gpio_cfg_output(TEST_PIN2);
#endif
}

void UartProtocol::reset() {
	// No logs, this function can be called from interrupt
#ifdef TEST_PIN2
	nrf_gpio_pin_toggle(TEST_PIN2);
#endif
	_readBufferIdx = 0;
	_startedReading = false;
	_escapeNextByte = false;
	_readPacketSize = 0;
}

void UartProtocol::escape(uint8_t& val) {
	val ^= UART_ESCAPE_FLIP_MASK;
}

void UartProtocol::unEscape(uint8_t& val) {
	// No logs, this function can be called from interrupt
	val ^= UART_ESCAPE_FLIP_MASK;
}


uint16_t UartProtocol::crc16(const uint8_t * data, uint16_t size) {
	return crc16_compute(data, size, NULL);
}

void UartProtocol::crc16(const uint8_t * data, const uint16_t size, uint16_t& crc) {
	crc = crc16_compute(data, size, &crc);
}

void UartProtocol::writeMsg(UartOpcodeTx opCode, uint8_t * data, uint16_t size) {
	// when debugging we would like to drop out of certain binary data coming over the console...
	switch(opCode) {
	    case UART_OPCODE_TX_TEXT:
		break;
	    default: 
		return;
	}
	// No logs, this function is called when logging
	writeMsgStart(opCode, size);
	writeMsgPart(opCode, data, size);
	writeMsgEnd(opCode);
}

void UartProtocol::writeMsgStart(UartOpcodeTx opCode, uint16_t size) {
	// when debugging we would like to drop out of certain binary data coming over the console...
	switch(opCode) {
	    case UART_OPCODE_TX_TEXT:
		break;
	    default: 
		return;
	}

	// No logs, this function is called when logging
	if (size > UART_TX_MAX_PAYLOAD_SIZE) {
		return;
	}

	uart_msg_header_t header;
	header.opCode = opCode;
	header.size = size;
	_crc = crc16((uint8_t*)(&header), sizeof(uart_msg_header_t));

	writeStartByte();
	writeBytes((uint8_t*)(&header), sizeof(uart_msg_header_t));
}

void UartProtocol::writeMsgPart(UartOpcodeTx opCode, uint8_t * data, uint16_t size) {
	// when debugging we would like to drop out of certain binary data coming over the console...
	switch(opCode) {
	    case UART_OPCODE_TX_TEXT:
		break;
	    default: 
		return;
	}
	// No logs, this function is called when logging
	crc16(data, size, _crc);
	writeBytes(data, size);
}

void UartProtocol::writeMsgEnd(UartOpcodeTx opCode) {
	// when debugging we would like to drop out of certain binary data coming over the console...
	switch(opCode) {
	    case UART_OPCODE_TX_TEXT:
		break;
	    default: 
		return;
	}
	// No logs, this function is called when logging
	uart_msg_tail_t tail;
	tail.crc = _crc;
	writeBytes((uint8_t*)(&tail), sizeof(uart_msg_tail_t));
}


void UartProtocol::onRead(uint8_t val) {
	// No logs, this function can be called from interrupt
	// CRC error? Reset.
	// Start char? Reset.
	// Bad escaped value? Reset.
	// Bad length? Reset. Over-run length of buffer? Reset.
	// Haven't seen a start char in too long? Reset anyway.

	// Can't read anything while processing the previous msg.
	if (_readBusy) {
#ifdef TEST_PIN2
		nrf_gpio_pin_toggle(TEST_PIN2);
#endif
		return;
	}

#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	// An escape shouldn't be followed by a special byte
	if (_escapeNextByte) {
		switch (val) {
		case UART_START_BYTE:
		case UART_ESCAPE_BYTE:
			reset();
			return;
		}
	}

	if (val == UART_START_BYTE) {
		reset();
		_startedReading = true;
		return;
	}

	if (!_startedReading) {
		return;
	}


	if (val == UART_ESCAPE_BYTE) {
		_escapeNextByte = true;
		return;
	}

	if (_escapeNextByte) {
		unEscape(val);
		_escapeNextByte = false;
	}

//#ifdef TEST_PIN
//	nrf_gpio_pin_toggle(TEST_PIN);
//#endif

	_readBuffer[_readBufferIdx++] = val;

	if (_readBufferIdx == sizeof(uart_msg_header_t)) {
		// First check received size, then add header and tail size.
		// Otherwise an overflow would lead to passing the size check.
		_readPacketSize = ((uart_msg_header_t*)_readBuffer)->size;
		if (_readPacketSize > UART_RX_MAX_PAYLOAD_SIZE) {
			reset();
			return;
		}
		_readPacketSize += sizeof(uart_msg_header_t) + sizeof(uart_msg_tail_t);
	}

	if (_readBufferIdx >= sizeof(uart_msg_header_t)) {
		// Header was read.
		if (_readBufferIdx >= _readPacketSize) {
			// Block reading until the buffer has been handled.
			_readBusy = true;
			// Decouple callback from interrupt handler, and put it on app scheduler instead
			uart_handle_msg_data_t msgData;
			msgData.msg = _readBuffer;
			msgData.msgSize = _readBufferIdx;
			uint32_t errorCode = app_sched_event_put(&msgData, sizeof(uart_handle_msg_data_t), handle_msg);
			APP_ERROR_CHECK(errorCode);
		}
	}
}


void UartProtocol::handleMsg(uart_handle_msg_data_t* msgData) {
	uint8_t* data = msgData->msg;
	uint16_t size = msgData->msgSize;

//	LOGd("read:");
//	BLEutil::printArray(data, size);

	// Check CRC
	uint16_t calculatedCrc = crc16(data, size - sizeof(uart_msg_tail_t));
	uint16_t receivedCrc = *((uint16_t*)(data + size - sizeof(uart_msg_tail_t)));
	if (calculatedCrc != receivedCrc) {
		LOGw("crc mismatch: %u vs %u", calculatedCrc, receivedCrc);
		_readBusy = false;
		reset();
		return;
	}

	uart_msg_header_t* header = (uart_msg_header_t*)data;
	uint8_t* payload = data + sizeof(uart_msg_header_t);
	LOGd("handle msg %u", header->opCode);

	switch (header->opCode) {
	case UART_OPCODE_RX_CONTROL: {
		stream_header_t* streamHeader = (stream_header_t*)payload;
		if (header->size - sizeof(stream_header_t) < streamHeader->length) {
			LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
			break;
		}
//		uint8_t* streamPayload = (uint8_t*)payload + sizeof(stream_header_t);
//		CommandHandler::getInstance().handleCommand((CommandHandlerTypes)streamHeader->type, streamPayload, streamHeader->length);
//		TODO: implement event dispatch..
		EventDispatcher::getInstance().dispatch(EVT_RX_CONTROL, payload, 1);
		break;
	}
	case UART_OPCODE_RX_ENABLE_ADVERTISEMENT:
		if (header->size < 1) { break; }
		EventDispatcher::getInstance().dispatch(EVT_ENABLE_ADVERTISEMENT, payload, 1);
		break;
	case UART_OPCODE_RX_ENABLE_MESH:
		if (header->size < 1) { break; }
		EventDispatcher::getInstance().dispatch(EVT_ENABLE_MESH, payload, 1);
		break;
	case UART_OPCODE_RX_GET_ID:
		uint16_t crownstoneId;
		Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &crownstoneId);
		writeMsg(UART_OPCODE_TX_OWN_ID, (uint8_t*)&crownstoneId, sizeof(crownstoneId));
		break;
	case UART_OPCODE_RX_GET_MAC:
		uint32_t err_code;
		ble_gap_addr_t address;
		err_code = sd_ble_gap_address_get(&address);
		APP_ERROR_CHECK(err_code);
		writeMsg(UART_OPCODE_TX_OWN_MAC, address.addr, sizeof(ble_gap_addr_t));
		break;
//	case UART_OPCODE_RX_ADC_CONFIG_GET:
//		break;
//	case UART_OPCODE_RX_ADC_CONFIG_SET:
//		break;
//	case UART_OPCODE_RX_ADC_CONFIG_SET_RANGE:
//		break;
	case UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_CURRENT:
		EventDispatcher::getInstance().dispatch(EVT_INC_CURRENT_RANGE);
		break;
	case UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_CURRENT:
		EventDispatcher::getInstance().dispatch(EVT_DEC_CURRENT_RANGE);
		break;
	case UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_VOLTAGE:
		EventDispatcher::getInstance().dispatch(EVT_INC_VOLTAGE_RANGE);
		break;
	case UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_VOLTAGE:
		EventDispatcher::getInstance().dispatch(EVT_DEC_VOLTAGE_RANGE);
		break;
	case UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_CURRENT:
		EventDispatcher::getInstance().dispatch(EVT_ENABLE_ADC_DIFFERENTIAL_CURRENT);
		break;
	case UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_VOLTAGE:
		EventDispatcher::getInstance().dispatch(EVT_ENABLE_ADC_DIFFERENTIAL_VOLTAGE);
		break;
	case UART_OPCODE_RX_ADC_CONFIG_VOLTAGE_PIN:
		EventDispatcher::getInstance().dispatch(EVT_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN);
		break;
	case UART_OPCODE_RX_POWER_LOG_CURRENT:
		if (header->size < 1) { break; }
		EventDispatcher::getInstance().dispatch(EVT_ENABLE_LOG_CURRENT, payload, 1);
		break;
	case UART_OPCODE_RX_POWER_LOG_VOLTAGE:
		if (header->size < 1) { break; }
		EventDispatcher::getInstance().dispatch(EVT_ENABLE_LOG_VOLTAGE, payload, 1);
		break;
	case UART_OPCODE_RX_POWER_LOG_FILTERED_CURRENT:
		if (header->size < 1) { break; }
		EventDispatcher::getInstance().dispatch(EVT_ENABLE_LOG_FILTERED_CURRENT, payload, 1);
		break;
//	case UART_OPCODE_RX_POWER_LOG_FILTERED_VOLTAGE:
//		EventDispatcher::getInstance().dispatch();
//		break;
	case UART_OPCODE_RX_POWER_LOG_POWER:
		if (header->size < 1) { break; }
		EventDispatcher::getInstance().dispatch(EVT_ENABLE_LOG_POWER, payload, 1);
		break;
	}


	// When done, ALWAYS reset and set readBusy to false!
	// Reset invalidates the data, right?
	_readBusy = false;
	reset();
	return;


//	case 82: // R
//		write("radio: %u\r\n", NRF_RADIO->POWER);
//		break;
}
