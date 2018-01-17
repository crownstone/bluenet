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

#include "protocol/cs_UartParser.h"
#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"
#include "protocol/cs_ErrorCodes.h"
#include "util/cs_BleError.h"
#include "ble/cs_Nordic.h"

//#include <crc16.h>
//#include <nrf.h>

UartParser::UartParser():
readBuffer(NULL),
readBufferIdx(0),
startedReading(false),
escapeNextByte(false),
readPacketSize(0),
readBusy(false)
{

}

void handle_msg(void * data, uint16_t size) {
	UartParser::getInstance().handleMsg(data, size);
}

void UartParser::init() {
	readBuffer = new uint8_t[UART_RX_BUFFER_SIZE];
}

void UartParser::reset() {
	write("r\r\n");
	BLEutil::printArray(readBuffer, readBufferIdx);
	readBufferIdx = 0;
	startedReading = false;
	escapeNextByte = false;
	readPacketSize = 0;
}

void UartParser::escape(uint8_t& val) {
	val ^= SERIAL_ESCAPE_FLIP_MASK;
}

void UartParser::unEscape(uint8_t& val) {
	val ^= SERIAL_ESCAPE_FLIP_MASK;
}


void UartParser::onRead(uint8_t val) {
	write("b\r\n");

	// CRC error? Reset.
	// Start char? Reset.
	// Bad escaped value? Reset.
	// Bad length? Reset. Over-run length of buffer? Reset.
	// Haven't seen a start char in too long? Reset anyway.

	// Can't read anything while processing the previous msg.
	if (readBusy) {
		write("i\r\n");
		return;
	}

	// An escape shouldn't be followed by a special byte
	if (escapeNextByte) {
		switch (val) {
		case SERIAL_START_BYTE:
		case SERIAL_ESCAPE_BYTE:
			reset();
			return;
		}
	}

	if (val == SERIAL_START_BYTE) {
		reset();
		startedReading = true;
		return;
	}

	if (!startedReading) {
		write("n\r\n");
		return;
	}

	if (val == SERIAL_ESCAPE_BYTE) {
		escapeNextByte = true;
		write("e\r\n");
		return;
	}

	if (escapeNextByte) {
		unEscape(val);
		escapeNextByte = false;
	}

	readBuffer[readBufferIdx++] = val;
	write("b\r\n");

	if (readBufferIdx == sizeof(uart_msg_header_t)) {
		// First check received size, then add header and tail size.
		// Otherwise an overflow would lead to passing the size check.
		readPacketSize = ((uart_msg_header_t*)readBuffer)->size;
		if (readPacketSize > UART_RX_BUFFER_SIZE) {
			reset();
			return;
		}
		readPacketSize += sizeof(uart_msg_header_t) + sizeof(uart_msg_tail_t);
	}

	if (readBufferIdx >= sizeof(uart_msg_header_t)) {
		// Header was read.
		if (readBufferIdx >= readPacketSize) {
			// Check CRC
			uint16_t calculatedCrc = crc16(readBuffer, readPacketSize - sizeof(uart_msg_tail_t));
			uint16_t receivedCrc = *((uint16_t*)(readBuffer + readPacketSize - sizeof(uart_msg_tail_t)));
			if (calculatedCrc != receivedCrc) {
				LOGw("crc: %u vs %u", calculatedCrc, receivedCrc);
				reset();
				return;
			}

			readBusy = true;
			// Decouple callback from interrupt handler, and put it on app scheduler instead
			uint32_t errorCode = app_sched_event_put(readBuffer, readBufferIdx, handle_msg);
			APP_ERROR_CHECK(errorCode);
		}
	}
}

uint16_t UartParser::crc16(const uint8_t * data, uint16_t size) {
	return crc16_compute(data, size, NULL);
}

void UartParser::crc16(const uint8_t * data, const uint16_t size, uint16_t& crc) {
	crc = crc16_compute(data, size, &crc);
}

void UartParser::handleMsg(void * data, uint16_t size) {
	LOGd("read:");
	BLEutil::printArray(data, size);
	readBufferIdx = 0;
	readBusy = false;
	return;


//	uint16_t event = 0;
//	switch (readByte) {
//	case 42: // *
//		event = EVT_INC_VOLTAGE_RANGE;
//		break;
//	case 47: // /
//		event = EVT_DEC_VOLTAGE_RANGE;
//		break;
//	case 43: // +
//		event = EVT_INC_CURRENT_RANGE;
//		break;
//	case 45: // -
//		event = EVT_DEC_CURRENT_RANGE;
//		break;
//	case 97: // a
//		event = EVT_TOGGLE_ADVERTISEMENT;
//		break;
//	case 99: // c
//		event = EVT_TOGGLE_LOG_CURRENT;
//		break;
//	case 68: // D
//		event = EVT_TOGGLE_ADC_DIFFERENTIAL_VOLTAGE;
//		break;
//	case 100: // d
//		event = EVT_TOGGLE_ADC_DIFFERENTIAL_CURRENT;
//		break;
//	case 70: // F
//		write("Paid respect\r\n");
//		break;
//	case 102: // f
//		event = EVT_TOGGLE_LOG_FILTERED_CURRENT;
//		break;
//	case 109: // m
//		event = EVT_TOGGLE_MESH;
//		break;
//	case 112: // p
//		event = EVT_TOGGLE_LOG_POWER;
//		break;
//	case 82: // R
//		write("radio: %u\r\n", NRF_RADIO->POWER);
//		break;
//	case 114: // r
//		event = EVT_CMD_RESET;
//		break;
//	case 86: // V
//		event = EVT_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN;
//		break;
//	case 118: // v
//		event = EVT_TOGGLE_LOG_VOLTAGE;
//		break;
//	}
//	if (event != 0) {
//		EventDispatcher::getInstance().dispatch(event);
//	}
//	readBusy = false;
}
