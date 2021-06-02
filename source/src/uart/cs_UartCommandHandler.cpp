/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <encryption/cs_KeysAndAccess.h>
#include <events/cs_EventDispatcher.h>
#include <protocol/cs_UartMsgTypes.h>
#include <storage/cs_State.h>
#include <uart/cs_UartCommandHandler.h>
#include <uart/cs_UartConnection.h>
#include <uart/cs_UartHandler.h>

#define LOGUartCommandHandlerDebug LOGnone

void UartCommandHandler::handleCommand(UartOpcodeRx opCode,
			cs_data_t commandData,
			EncryptionAccessLevel accessLevel,
			bool wasEncrypted,
			cs_data_t resultBuffer
			) {
	LOGUartCommandHandlerDebug("Handle cmd opCode=%u size=%u accessLevel=%u", opCode, commandData.len, accessLevel);

	EncryptionAccessLevel requiredAccess = getRequiredAccessLevel(opCode);
	if (!KeysAndAccess::getInstance().allowAccess(requiredAccess, accessLevel)) {
		LOGi("No access: required=%u given=%u", requiredAccess, accessLevel);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	}

	cmd_source_with_counter_t source(cmd_source_t(CS_CMD_SOURCE_TYPE_UART, 63), 0); // Least likely real device ID.

	switch (opCode) {
		case UART_OPCODE_RX_HELLO:
			handleCommandHello(commandData);
			break;
		case UART_OPCODE_RX_SESSION_NONCE:
			handleCommandSessionNonce(commandData);
			break;
		case UART_OPCODE_RX_HEARTBEAT:
			handleCommandHeartBeat(commandData, wasEncrypted);
			break;
		case UART_OPCODE_RX_STATUS:
			handleCommandStatus(commandData);
			break;
		case UART_OPCODE_RX_GET_MAC:
			handleCommandGetMacAddress(commandData);
			break;
		case UART_OPCODE_RX_CONTROL:
			handleCommandControl(commandData, source, accessLevel, resultBuffer);
			break;
		case UART_OPCODE_RX_HUB_DATA_REPLY:
			handleCommandHubDataReply(commandData, source, accessLevel, resultBuffer);
			break;

#ifdef DEBUG
		case UART_OPCODE_RX_ENABLE_ADVERTISEMENT:
			dispatchEventForCommand(CS_TYPE::CMD_ENABLE_ADVERTISEMENT, commandData);
			break;
		case UART_OPCODE_RX_ENABLE_MESH:
			dispatchEventForCommand(CS_TYPE::CMD_ENABLE_MESH, commandData);
			break;
		case UART_OPCODE_RX_GET_ID:
			handleCommandGetId(commandData);
			break;

		case UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_CURRENT:
			dispatchEventForCommand(CS_TYPE::CMD_INC_CURRENT_RANGE, commandData);
			break;
		case UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_CURRENT:
			dispatchEventForCommand(CS_TYPE::CMD_DEC_CURRENT_RANGE, commandData);
			break;
		case UART_OPCODE_RX_ADC_CONFIG_INC_RANGE_VOLTAGE:
			dispatchEventForCommand(CS_TYPE::CMD_INC_VOLTAGE_RANGE, commandData);
			break;
		case UART_OPCODE_RX_ADC_CONFIG_DEC_RANGE_VOLTAGE:
			dispatchEventForCommand(CS_TYPE::CMD_DEC_VOLTAGE_RANGE, commandData);
			break;
		case UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_CURRENT:
			dispatchEventForCommand(CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT, commandData);
			break;
		case UART_OPCODE_RX_ADC_CONFIG_DIFFERENTIAL_VOLTAGE:
			dispatchEventForCommand(CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE, commandData);
			break;
		case UART_OPCODE_RX_ADC_CONFIG_VOLTAGE_PIN:
			dispatchEventForCommand(CS_TYPE::CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN, commandData);
			break;


		case UART_OPCODE_RX_POWER_LOG_CURRENT:
			dispatchEventForCommand(CS_TYPE::CMD_ENABLE_LOG_CURRENT, commandData);
			break;
		case UART_OPCODE_RX_POWER_LOG_VOLTAGE:
			dispatchEventForCommand(CS_TYPE::CMD_ENABLE_LOG_VOLTAGE, commandData);
			break;
		case UART_OPCODE_RX_POWER_LOG_FILTERED_CURRENT:
			dispatchEventForCommand(CS_TYPE::CMD_ENABLE_LOG_FILTERED_CURRENT, commandData);
			break;
		case UART_OPCODE_RX_POWER_LOG_POWER:
			dispatchEventForCommand(CS_TYPE::CMD_ENABLE_LOG_POWER, commandData);
			break;


		case UART_OPCODE_RX_INJECT_EVENT:
			handleCommandInjectEvent(commandData);
			break;

	#pragma message("UART dev commands enabled")
#else
	#pragma message("UART dev commands disabled")
#endif // DEBUG
		default:
			LOGw("Unknown opcode: %u", opCode);
			UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
			break;
	}
}

EncryptionAccessLevel UartCommandHandler::getRequiredAccessLevel(const UartOpcodeRx opCode) {
	if (!UartProtocol::mustBeEncryptedRx(opCode)) {
		return EncryptionAccessLevel::ENCRYPTION_DISABLED;
	}

	switch (opCode) {
		case UART_OPCODE_RX_HEARTBEAT:
		case UART_OPCODE_RX_STATUS:
		case UART_OPCODE_RX_CONTROL:
			return EncryptionAccessLevel::MEMBER;

		default:
			LOGw("Unknown opcode: %i", opCode);
			return EncryptionAccessLevel::NO_ONE;
	}
}

void UartCommandHandler::dispatchEventForCommand(
		CS_TYPE type,
		cs_data_t commandData
		) {
//	event_t event(type, commandData.data, commandData.len, source, result);
//	event.dispatch();
//	result.returnCode = event.result.returnCode;
//	result.dataSize = event.result.dataSize;

	cmd_source_with_counter_t source(CS_CMD_SOURCE_TYPE_UART);
	event_t event(type, commandData.data, commandData.len, source);
	event.dispatch();
}

void UartCommandHandler::handleCommandHello(cs_data_t commandData) {
	LOGd(STR_HANDLE_COMMAND, "hello");
	if (commandData.len < sizeof(uart_msg_status_user_flags_t)) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	}
	uart_msg_status_user_flags_t* userFlags = reinterpret_cast<uart_msg_status_user_flags_t*>(commandData.data);
	UartConnection::getInstance().onHello(*userFlags);
}

void UartCommandHandler::handleCommandSessionNonce(cs_data_t commandData) {
	LOGd(STR_HANDLE_COMMAND, "session nonce");
	if (commandData.len < sizeof(uart_msg_session_nonce_t)) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	}
	uart_msg_session_nonce_t* sessionNonce = reinterpret_cast<uart_msg_session_nonce_t*>(commandData.data);
	UartConnection::getInstance().onSessionNonce(*sessionNonce);
}

void UartCommandHandler::handleCommandHeartBeat(cs_data_t commandData, bool wasEncrypted) {
	LOGUartCommandHandlerDebug(STR_HANDLE_COMMAND, "heartbeat wasEncrypted=%u", wasEncrypted);
	if (commandData.len < sizeof(uart_msg_heartbeat_t)) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	}
	uart_msg_heartbeat_t* heartbeat = reinterpret_cast<uart_msg_heartbeat_t*>(commandData.data);
	UartConnection::getInstance().onHeartBeat(heartbeat->timeoutSeconds, wasEncrypted);
}

void UartCommandHandler::handleCommandStatus(cs_data_t commandData) {
	LOGUartCommandHandlerDebug(STR_HANDLE_COMMAND, "status");
	if (commandData.len < sizeof(uart_msg_status_user_t)) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	}
	uart_msg_status_user_t* userStatus = reinterpret_cast<uart_msg_status_user_t*>(commandData.data);
	UartConnection::getInstance().onUserStatus(*userStatus);
}

void UartCommandHandler::handleCommandControl(cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_data_t resultBuffer) {
	LOGd(STR_HANDLE_COMMAND, "control");
	control_packet_header_t* controlHeader = reinterpret_cast<control_packet_header_t*>(commandData.data);
	if (commandData.len < sizeof(*controlHeader)) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	}
	if (commandData.len < controlHeader->payloadSize + sizeof(*controlHeader)) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	}
	TYPIFY(CMD_CONTROL_CMD) controlCmd;
	controlCmd.protocolVersion = controlHeader->protocolVersion;
	controlCmd.type = (CommandHandlerTypes)controlHeader->commandType;
	controlCmd.accessLevel = accessLevel;
	controlCmd.data = commandData.data + sizeof(*controlHeader);
	controlCmd.size = controlHeader->payloadSize;

	cs_result_t result(resultBuffer);

	event_t event(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd), source, result);
	EventDispatcher::getInstance().dispatch(event);

	result_packet_header_t resultHeader(controlCmd.type, event.result.returnCode, event.result.dataSize);
	UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_CONTROL_RESULT, sizeof(resultHeader) + resultHeader.payloadSize);
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_CONTROL_RESULT, (uint8_t*)&resultHeader, sizeof(resultHeader));
	UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_CONTROL_RESULT, event.result.buf.data, event.result.dataSize);
	UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_CONTROL_RESULT);
}

void UartCommandHandler::handleCommandHubDataReply(cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_data_t resultBuffer) {
	LOGd("handleCommandHubDataReply");
	uart_msg_hub_data_reply_header_t* replyHeader = reinterpret_cast<uart_msg_hub_data_reply_header_t*>(commandData.data);
	if (commandData.len < sizeof(*replyHeader)) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	}

	TYPIFY(EVT_HUB_DATA_REPLY) evtData;
	evtData.retCode = replyHeader->retCode;
	evtData.data.data = commandData.data + sizeof(*replyHeader);
	evtData.data.len  = commandData.len  - sizeof(*replyHeader);
	event_t event(CS_TYPE::EVT_HUB_DATA_REPLY, &evtData, sizeof(evtData));
	event.dispatch();

	UartHandler::getInstance().writeMsg(UART_OPCODE_TX_HUB_DATA_REPLY_ACK);
}

void UartCommandHandler::handleCommandGetId(cs_data_t commandData) {
	TYPIFY(CONFIG_CROWNSTONE_ID) crownstoneId;
	cs_ret_code_t retCode = State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &crownstoneId, sizeof(crownstoneId));
	if (retCode == ERR_SUCCESS) {
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_OWN_ID, (uint8_t*)&crownstoneId, sizeof(crownstoneId));
	}
}

void UartCommandHandler::handleCommandGetMacAddress(cs_data_t commandData) {
	ble_gap_addr_t address;
	uint32_t err_code = sd_ble_gap_addr_get(&address);
	if (err_code == NRF_SUCCESS) {
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_MAC, address.addr, sizeof(address.addr));
	}
}

void UartCommandHandler::handleCommandInjectEvent(cs_data_t commandData) {
	LOGd(STR_HANDLE_COMMAND, "inject event");

	// Header is uint16 cs type.
	if (commandData.len < sizeof(uint16_t)) {
		LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	}

	uint16_t* type = reinterpret_cast<uint16_t*>(commandData.data);
	CS_TYPE csType = toCsType(*type);

	if (csType == CS_TYPE::CONFIG_DO_NOT_USE) {
		LOGw("Invalid type: %u", type);
		UartHandler::getInstance().writeMsg(UART_OPCODE_TX_ERR_REPLY_PARSING_FAILED);
		return;
	} else {
		LOGd("dispatching event for type: %u", csType);
	}

	uint8_t* eventData = commandData.data + sizeof(uint16_t);
	uint16_t eventDataSize = commandData.len - sizeof(uint16_t);
	event_t event(csType, eventData, eventDataSize);
	event.dispatch();
}

