/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <events/cs_EventDispatcher.h>
#include <processing/cs_EncryptionHandler.h>
#include <uart/cs_UartCommandHandler.h>
#include <uart/cs_UartConnection.h>
#include <uart/cs_UartHandler.h>
#include <drivers/cs_Serial.h>
#include <util/cs_BleError.h>

#define LOGUartHandlerDebug LOGnone

void handle_msg(void * data, uint16_t size) {
	UartHandler::getInstance().handleMsg((cs_data_t*)data);
}

UartHandler::UartHandler() {

}

void UartHandler::init(serial_enable_t serialEnabled) {
	if (_initialized) {
		return;
	}
	switch (serialEnabled) {
		case SERIAL_ENABLE_NONE:
			return;
		case SERIAL_ENABLE_RX_ONLY:
		case SERIAL_ENABLE_RX_AND_TX:
			// We init both at the same time.
			break;
		default:
			return;
	}
	_initialized = true;
	_readBuffer = new uint8_t[UART_RX_BUFFER_SIZE];
	_writeBuffer = new uint8_t[UART_TX_BUFFER_SIZE];

	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_stoneId, sizeof(_stoneId));

	// TODO: get UART key?

	UartConnection::getInstance().init();

	listen();
}

void UartHandler::writeMsg(UartOpcodeTx opCode, uint8_t * data, uint16_t size) {

#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
	switch(opCode) {
		// when debugging we would like to drop out of certain binary data coming over the console...
		case UART_OPCODE_TX_TEXT:
			// Now only the special chars get escaped, no header and tail.
			writeBytes(data, size);
			return;
		case UART_OPCODE_TX_SERVICE_DATA:
			return;
		case UART_OPCODE_TX_FIRMWARESTATE:
			writeBytes(data, size);
			return;
		default:
//			_log(SERIAL_DEBUG, "writeMsg opCode=%u data=", opCode);
//			BLEutil::printArray(data, size);
			return;
	}
#endif

	writeMsgStart(opCode, size);
	writeMsgPart(opCode, data, size);
	writeMsgEnd(opCode);
}

void UartHandler::writeMsgStart(UartOpcodeTx opCode, uint16_t size) {
//	if (size > UART_TX_MAX_PAYLOAD_SIZE) {
//		return;
//	}

	uart_msg_size_header_t sizeHeader;
	uart_msg_wrapper_header_t wrapperHeader;
//	if (UartProtocol::mustBeEncryptedTx(opCode)) {
//		wrapperHeader.type = static_cast<uint8_t>(UartMsgType::ENCRYPTED_UART_MSG);
//		uint16_t encryptedDataSize = sizeof(uart_encrypted_data_header_t.size) + sizeof(uart_msg_header_t) + size;
//		EncryptionHandler::calculateEncryptionBufferLength(encryptedDataSize, EncryptionType::CTR);
//	}
//	else {
		writeStartByte();

		// Init CRC
		_crc = UartProtocol::crc16(nullptr, 0);

		sizeHeader.size = sizeof(wrapperHeader) + sizeof(uart_msg_header_t) + size + sizeof(uart_msg_tail_t);
		writeBytes((uint8_t*)(&sizeHeader), sizeof(sizeHeader));

		wrapperHeader.type = static_cast<uint8_t>(UartMsgType::UART_MSG);
		UartProtocol::crc16((uint8_t*)(&wrapperHeader), sizeof(wrapperHeader), _crc);
		writeBytes((uint8_t*)(&wrapperHeader), sizeof(wrapperHeader));

		uart_msg_header_t msgHeader;
		msgHeader.deviceId = _stoneId;
		msgHeader.type = opCode;
		UartProtocol::crc16((uint8_t*)(&msgHeader), sizeof(msgHeader), _crc);
		writeBytes((uint8_t*)(&msgHeader), sizeof(msgHeader));
//	}
}

void UartHandler::writeMsgPart(UartOpcodeTx opCode, uint8_t * data, uint16_t size) {

#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
	// when debugging we would like to drop out of certain binary data coming over the console...
	switch(opCode) {
		case UART_OPCODE_TX_TEXT:
			// Now only the special chars get escaped, no header and tail.
			writeBytes(data, size);
			return;
		default:
			return;
	}
#endif

	// No logs, this function is called when logging
	UartProtocol::crc16(data, size, _crc);
	writeBytes(data, size);
}

void UartHandler::writeMsgEnd(UartOpcodeTx opCode) {
	// No logs, this function is called when logging
	uart_msg_tail_t tail;
	tail.crc = _crc;
	writeBytes((uint8_t*)(&tail), sizeof(uart_msg_tail_t));
}








void UartHandler::resetReadBuf() {
	// There are no logs written from this function. It can be called from an interrupt service routine.
	_readBufferIdx = 0;
	_startedReading = false;
	_escapeNextByte = false;
	_sizeToRead = 0;
}

void UartHandler::onRead(uint8_t val) {
	// No logs, this function can be called from interrupt
	// CRC error? Reset.
	// Start char? Reset.
	// Bad escaped value? Reset.
	// Bad length? Reset. Over-run length of buffer? Reset.
	// Haven't seen a start char in too long? Reset anyway.

	// Can't read anything while still processing the previous message.
	if (_readBusy) {
		return;
	}

	if (_readBuffer == nullptr) {
		return;
	}

	// An escape shouldn't be followed by a special byte.
	switch (val) {
		case UART_START_BYTE:
		case UART_ESCAPE_BYTE:
			if (_escapeNextByte) {
				resetReadBuf();
				return;
			}
	}

	if (val == UART_START_BYTE) {
		resetReadBuf();
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
		UartProtocol::unEscape(val);
		_escapeNextByte = false;
	}

	_readBuffer[_readBufferIdx++] = val;

	if (_sizeToRead == 0) {
		if (_readBufferIdx == sizeof(uart_msg_size_header_t)) {
			// Check received size
			uart_msg_size_header_t* sizeHeader = reinterpret_cast<uart_msg_size_header_t*>(_readBuffer);
			if (sizeHeader->size == 0 || sizeHeader->size > UART_RX_BUFFER_SIZE) {
				resetReadBuf();
				return;
			}
			// Set size to read and reset read buffer index.
			_sizeToRead = sizeHeader->size;
			_readBufferIdx = 0;
		}
	}
	else if (_readBufferIdx >= _sizeToRead) {
		// Block reading until the buffer has been handled.
		_readBusy = true;
		// Decouple callback from interrupt handler, and put it on app scheduler instead
		cs_data_t msgData;
		msgData.data = _readBuffer;
		msgData.len = _readBufferIdx;
		uint32_t errorCode = app_sched_event_put(&msgData, sizeof(msgData), handle_msg);
		APP_ERROR_CHECK(errorCode);
	}
}


void UartHandler::handleMsg(cs_data_t* msgData) {
	uint8_t* data = msgData->data;
	uint16_t size = msgData->len;

	handleMsg(data, size);

	// When done, ALWAYS reset and set readBusy to false!
	_readBusy = false;
	resetReadBuf();
}

void UartHandler::handleMsg(uint8_t* data, uint16_t size) {
	LOGUartHandlerDebug("Handle msg size=%u", size);

	// Check size
	uint16_t wrapperSize = sizeof(uart_msg_wrapper_header_t) + sizeof(uart_msg_tail_t);
	if (size < wrapperSize) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
//		LOGw("Wrapper won't fit required=%u size=%u", wrapperSize, size);
		return;
	}

	// Get wrapper header and payload data.
	uart_msg_wrapper_header_t* wrapperHeader = reinterpret_cast<uart_msg_wrapper_header_t*>(data);
	uint8_t* payload = data + sizeof(uart_msg_wrapper_header_t);
	uint16_t payloadSize = size - wrapperSize;

	LOGd("handleMsg %u %u %u %u %u %u %u %u", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

	// Check CRC
	uint16_t calculatedCrc = UartProtocol::crc16(data, size - sizeof(uart_msg_tail_t));
	uart_msg_tail_t* tail = reinterpret_cast<uart_msg_tail_t*>(data + size - sizeof(uart_msg_tail_t));
	if (calculatedCrc != tail->crc) {
		LOGw("CRC mismatch: calculated=%u received=%u", calculatedCrc, tail->crc);
		return;
	}

	switch (static_cast<UartMsgType>(wrapperHeader->type)) {
		case UartMsgType::ENCRYPTED_UART_MSG: {
			LOGd("TODO");
			break;
		}
		case UartMsgType::UART_MSG: {
			handleUartMsg(payload, payloadSize);
			break;
		}
	}
}

void UartHandler::handleUartMsg(uint8_t* data, uint16_t size) {
	if (size < sizeof(uart_msg_header_t)) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
//		LOGw("Header won't fit required=%u size=%u", sizeof(uart_msg_header_t), payloadSize);
		return;
	}
	uart_msg_header_t* header = reinterpret_cast<uart_msg_header_t*>(data);
	uint16_t msgDataSize = size - sizeof(uart_msg_header_t);
	uint8_t* msgData = data + sizeof(uart_msg_header_t);

	_commandHandler.handleCommand(
			static_cast<UartOpcodeRx>(header->type),
			cs_data_t(msgData, msgDataSize),
			cmd_source_with_counter_t(cmd_source_t(CS_CMD_SOURCE_TYPE_UART, header->deviceId)),
			EncryptionAccessLevel::ADMIN,
			cs_data_t(_writeBuffer, (_writeBuffer == nullptr) ? 0 : UART_TX_BUFFER_SIZE)
			);
}

void UartHandler::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::CONFIG_UART_ENABLED: {
			TYPIFY(CONFIG_UART_ENABLED) enabled = *(TYPIFY(CONFIG_UART_ENABLED)*)event.data;
			serial_enable((serial_enable_t)enabled);
			break;
		}
		case CS_TYPE::EVT_STATE_EXTERNAL_STONE: {
			TYPIFY(EVT_STATE_EXTERNAL_STONE)* state = (TYPIFY(EVT_STATE_EXTERNAL_STONE)*)event.data;
			writeMsg(UART_OPCODE_TX_MESH_STATE, (uint8_t*)&(state->data), sizeof(state->data));
			break;
		}
		case CS_TYPE::EVT_MESH_EXT_STATE_0: {
			TYPIFY(EVT_MESH_EXT_STATE_0)* state = (TYPIFY(EVT_MESH_EXT_STATE_0)*)event.data;
			writeMsg(UART_OPCODE_TX_MESH_STATE_PART_0, (uint8_t*)state, sizeof(*state));
			break;
		}
		case CS_TYPE::EVT_MESH_EXT_STATE_1: {
			TYPIFY(EVT_MESH_EXT_STATE_1)* state = (TYPIFY(EVT_MESH_EXT_STATE_1)*)event.data;
			writeMsg(UART_OPCODE_TX_MESH_STATE_PART_1, (uint8_t*)state, sizeof(*state));
			break;
		}
		case CS_TYPE::EVT_PRESENCE_CHANGE: {
			TYPIFY(EVT_PRESENCE_CHANGE)* state = (TYPIFY(EVT_PRESENCE_CHANGE)*)event.data;
			writeMsg(UART_OPCODE_TX_PRESENCE_CHANGE, (uint8_t*)state, sizeof(*state));
			break;
		}
		default:
			break;
	}
}
