/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 17, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <common/cs_Types.h>
#include <events/cs_EventDispatcher.h>
#include <protocol/cs_ErrorCodes.h>
#include <protocol/cs_UartProtocol.h>
#include <storage/cs_State.h>
#include <structs/cs_ControlPacketAccessor.h>
#include <util/cs_BleError.h>
#include <util/cs_Utils.h>

UartProtocol::UartProtocol():
	_initialized(false),
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
	if (_initialized) {
		return;
	}
	_initialized = true;
	_readBuffer = new uint8_t[UART_RX_BUFFER_SIZE];
	EventDispatcher::getInstance().addListener(this);
}

void UartProtocol::reset() {
	// There are no logs written from this function. It can be called from an interrupt service routine.
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
#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
	switch(opCode) {
	// when debugging we would like to drop out of certain binary data coming over the console...
	case UART_OPCODE_TX_TEXT:
		// Now only the special chars get escaped, no header and tail.
		writeBytes(data, size);
		return;
	default:
		return;
	}
#endif
	// No logs, this function is called when logging
	writeMsgStart(opCode, size);
	writeMsgPart(opCode, data, size);
	writeMsgEnd(opCode);
}

void UartProtocol::writeMsgStart(UartOpcodeTx opCode, uint16_t size) {
#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
	// when debugging we would like to drop out of certain binary data coming over the console...
	switch(opCode) {
	default:
		return;
	}
#endif
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
	crc16(data, size, _crc);
	writeBytes(data, size);
}

void UartProtocol::writeMsgEnd(UartOpcodeTx opCode) {
#if CS_UART_BINARY_PROTOCOL_ENABLED == 0
	// when debugging we would like to drop out of certain binary data coming over the console...
	switch(opCode) {
	default:
		return;
	}
#endif
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

	// Can't read anything while still processing the previous message.
	if (_readBusy) {
		return;
	}

	// An escape shouldn't be followed by a special byte.
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
		control_packet_header_t* controlHeader = (control_packet_header_t*)payload;
		if (header->size < sizeof(*controlHeader)) {
			LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
			break;
		}
		if (header->size < controlHeader->payloadSize + sizeof(*controlHeader)) {
			LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
			break;
		}
		TYPIFY(CMD_CONTROL_CMD) controlCmd;
		controlCmd.type = (CommandHandlerTypes)controlHeader->commandType;
		controlCmd.accessLevel = ADMIN;
		controlCmd.data = payload + sizeof(*controlHeader);
		controlCmd.size = controlHeader->payloadSize;
		controlCmd.source = cmd_source_t(CS_CMD_SOURCE_UART);
		event_t event(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd));
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_ENABLE_ADVERTISEMENT: {
		if (header->size < 1) { break; }
		event_t event(CS_TYPE::CMD_ENABLE_ADVERTISEMENT, payload, 1);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_ENABLE_MESH: {
		if (header->size < 1) { break; }
		event_t event(CS_TYPE::CMD_ENABLE_MESH, payload, 1);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_GET_ID: {
		TYPIFY(CONFIG_CROWNSTONE_ID) crownstoneId;
		State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &crownstoneId, sizeof(crownstoneId));
		writeMsg(UART_OPCODE_TX_OWN_ID, (uint8_t*)&crownstoneId, sizeof(crownstoneId));
		break;
	}
	case UART_OPCODE_RX_GET_MAC: {
		uint32_t err_code;
		ble_gap_addr_t address;
		err_code = sd_ble_gap_addr_get(&address);
		APP_ERROR_CHECK(err_code);
		writeMsg(UART_OPCODE_TX_OWN_MAC, address.addr, sizeof(address.addr));
		break;
	}
	case UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_CURRENT: {
		event_t event(CS_TYPE::CMD_INC_CURRENT_RANGE);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_CURRENT: {
		event_t event(CS_TYPE::CMD_DEC_CURRENT_RANGE);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_VOLTAGE: {
		event_t event(CS_TYPE::CMD_INC_VOLTAGE_RANGE);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_VOLTAGE: {
		event_t event(CS_TYPE::CMD_DEC_VOLTAGE_RANGE);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_CURRENT: {
		event_t event(CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT, payload, 1);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_VOLTAGE: {
		event_t event(CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE, payload, 1);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_ADC_CONFIG_VOLTAGE_PIN: {
		event_t event(CS_TYPE::CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_POWER_LOG_CURRENT: {
		if (header->size < 1) { break; }
		event_t event(CS_TYPE::CMD_ENABLE_LOG_CURRENT, payload, 1);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_POWER_LOG_VOLTAGE: {
		if (header->size < 1) { break; }
		event_t event(CS_TYPE::CMD_ENABLE_LOG_VOLTAGE, payload, 1);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_POWER_LOG_FILTERED_CURRENT: {
		if (header->size < 1) { break; }
		event_t event(CS_TYPE::CMD_ENABLE_LOG_FILTERED_CURRENT, payload, 1);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	case UART_OPCODE_RX_POWER_LOG_POWER: {
		if (header->size < 1) { break; }
		event_t event(CS_TYPE::CMD_ENABLE_LOG_POWER, payload, 1);
		EventDispatcher::getInstance().dispatch(event);
		break;
	}
	}

	// When done, ALWAYS reset and set readBusy to false!
	// TODO(@vliedel): Reset invalidates the data, right?
	_readBusy = false;
	reset();
}

void UartProtocol::handleEvent(event_t & event) {
	switch(event.type) {
	case CS_TYPE::CONFIG_UART_ENABLED: {
		TYPIFY(CONFIG_UART_ENABLED) enabled = *(TYPIFY(CONFIG_UART_ENABLED)*)event.data;
		serial_enable((serial_enable_t)enabled);
		break;
	}
	default:
		break;
	}
}
