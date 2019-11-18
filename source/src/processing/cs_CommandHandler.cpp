/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 18, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "ble/cs_Advertiser.h"
#include "cfg/cs_Boards.h"
#include "cfg/cs_DeviceTypes.h"
#include "cfg/cs_Strings.h"
#include "drivers/cs_Serial.h"
#include "processing/cs_CommandHandler.h"
#include "processing/cs_Scanner.h"
#include "processing/cs_Scheduler.h"
#include "processing/cs_FactoryReset.h"
#include "processing/cs_Setup.h"
#include "processing/cs_TemperatureGuard.h"
#include "protocol/cs_UartProtocol.h"
#include "protocol/mesh/cs_MeshModelPacketHelper.h"
#include "storage/cs_State.h"
#include "time/cs_SystemTime.h"

#include <util/cs_WireFormat.h>

#include <sstream>

void reset(void* p_context) {

	uint32_t cmd = *(int32_t*) p_context;

	if (cmd == GPREGRET_DFU_RESET) {
		LOGi(MSG_FIRMWARE_UPDATE);
	} else {
		LOGi(MSG_RESET);
	}

	LOGi("Executing reset: %d", cmd);
	// copy to make sure this is nothing more than one value
	uint8_t err_code;
	uint32_t gpregret_id = 0;
	uint32_t gpregret_msk = 0xFF;
	err_code = sd_power_gpregret_clr(gpregret_id, gpregret_msk);
	APP_ERROR_CHECK(err_code);
	err_code = sd_power_gpregret_set(gpregret_id, cmd);
	APP_ERROR_CHECK(err_code);
	sd_nvic_SystemReset();
}


CommandHandler::CommandHandler() :
		_resetTimerId(NULL),
		_boardConfig(NULL)
{
		_resetTimerData = { {0} };
		_resetTimerId = &_resetTimerData;
}

void CommandHandler::init(const boards_config_t* board) {
	_boardConfig = board;
	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_resetTimerId, (app_timer_timeout_handler_t) reset);
}

void CommandHandler::resetDelayed(uint8_t opCode, uint16_t delayMs) {
	LOGi("Reset in %u ms, code=%u", delayMs, opCode);
	static uint8_t resetOpCode = opCode;
	Timer::getInstance().start(_resetTimerId, MS_TO_TICKS(delayMs), &resetOpCode);
//	// Loop until reset trigger
//	while(true) {}; // This doesn't seem to work
}


command_result_t CommandHandler::handleCommand(const CommandHandlerTypes type, const cmd_source_t source) {
	return handleCommand(type, cs_data_t(), source);
}

command_result_t CommandHandler::handleCommand(
		const CommandHandlerTypes type,
		cs_data_t commandData,
		const cmd_source_t source,
		const EncryptionAccessLevel accessLevel,
		cs_data_t resultData
		) {
	LOGd("cmd=%u lvl=%u", type, accessLevel);
	if (!EncryptionHandler::getInstance().allowAccess(getRequiredAccessLevel(type), accessLevel)) {
		return command_result_t(ERR_NO_ACCESS);
	}
	switch (type) {
	case CTRL_CMD_NOP:
		return handleCmdNop(commandData, accessLevel, resultData);
	case CTRL_CMD_GOTO_DFU:
		return handleCmdGotoDfu(commandData, accessLevel, resultData);
	case CTRL_CMD_RESET:
		return handleCmdReset(commandData, accessLevel, resultData);
	case CTRL_CMD_FACTORY_RESET:
		return handleCmdFactoryReset(commandData, accessLevel, resultData);
	case CTRL_CMD_SET_TIME:
		return handleCmdSetTime(commandData, accessLevel, resultData);
	case CTRL_CMD_SET_SUN_TIME:
		return handleCmdSetSunTime(commandData, accessLevel, resultData);
	case CTRL_CMD_INCREASE_TX:
		return handleCmdIncreaseTx(commandData, accessLevel, resultData);
	case CTRL_CMD_DISCONNECT:
		return handleCmdDisconnect(commandData, accessLevel, resultData);
	case CTRL_CMD_RESET_ERRORS:
		return handleCmdResetErrors(commandData, accessLevel, resultData);
	case CTRL_CMD_PWM:
		return handleCmdPwm(commandData, accessLevel, resultData);
	case CTRL_CMD_SWITCH:
		return handleCmdSwitch(commandData, accessLevel, resultData);
	case CTRL_CMD_RELAY:
		return handleCmdRelay(commandData, accessLevel, resultData);
	case CTRL_CMD_MULTI_SWITCH:
		return handleCmdMultiSwitch(commandData, source, accessLevel, resultData);
	case CTRL_CMD_MESH_COMMAND:
		return handleCmdMeshCommand(commandData, accessLevel, resultData);
	case CTRL_CMD_ALLOW_DIMMING:
		return handleCmdAllowDimming(commandData, accessLevel, resultData);
	case CTRL_CMD_LOCK_SWITCH:
		return handleCmdLockSwitch(commandData, accessLevel, resultData);
	case CTRL_CMD_SETUP:
		return handleCmdSetup(commandData, accessLevel, resultData);
	case CTRL_CMD_UART_MSG:
		return handleCmdUartMsg(commandData, accessLevel, resultData);
	case CTRL_CMD_STATE_GET:
		return handleCmdStateGet(commandData, accessLevel, resultData);
	case CTRL_CMD_STATE_SET:
		return handleCmdStateSet(commandData, accessLevel, resultData);
	case CTRL_CMD_SAVE_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::EVT_SAVE_BEHAVIOUR,commandData,resultData);
	case CTRL_CMD_REPLACE_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::EVT_REPLACE_BEHAVIOUR,commandData,resultData);
	case CTRL_CMD_REMOVE_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::EVT_REMOVE_BEHAVIOUR,commandData,resultData);
	case CTRL_CMD_GET_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::EVT_GET_BEHAVIOUR,commandData,resultData);
	case CTRL_CMD_GET_BEHAVIOUR_INDICES:
		return dispatchEventForCommand(CS_TYPE::EVT_GET_BEHAVIOUR_INDICES,commandData,resultData);

	case CTRL_CMD_UNKNOWN:
		return command_result_t(ERR_UNKNOWN_TYPE);
	}
	LOGe("Unknown type: %u", type);
	return command_result_t(ERR_UNKNOWN_TYPE);
}

command_result_t CommandHandler::handleCmdNop(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	// A no operation command to keep the connection alive.
	// No need to do anything here, the connection keep alive is handled in the stack.
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdGotoDfu(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "goto dfu");
	switch (_boardConfig->hardwareBoard) {
	case PCA10036:
	case PCA10040:
	case ACR01B1A:
	case ACR01B1B:
	case ACR01B1C:
	case ACR01B1D:
	case ACR01B1E:
	case ACR01B10B:
	case ACR01B2A:
	case ACR01B2B:
	case ACR01B2C:
	case ACR01B2E:
	case ACR01B2G: {
		// Turn relay on, as the bootloader doesn't manage to turn off the dimmer fast enough.
		TYPIFY(CMD_SWITCH) switchVal;
		switchVal.switchCmd = 100;
		switchVal.delay = 0;
		switchVal.source = cmd_source_t(CS_CMD_SOURCE_CONNECTION);
		event_t cmd(CS_TYPE::CMD_SWITCH, &switchVal, sizeof(switchVal));
		EventDispatcher::getInstance().dispatch(cmd);
		break;
	}
	case GUIDESTONE:
	case CS_USB_DONGLE:
	case ACR01B10C:
		break;
	default:
		LOGe("Unknown board");
		break;
	}
	resetDelayed(GPREGRET_DFU_RESET);
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdReset(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "reset");
	resetDelayed(GPREGRET_SOFT_RESET);
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdFactoryReset(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "factory reset");

	if (commandData.len != sizeof(FACTORY_RESET_CODE)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, sizeof(FACTORY_RESET_CODE));
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	factory_reset_message_payload_t* payload = (factory_reset_message_payload_t*) commandData.data;
	uint32_t resetCode = payload->resetCode;

	if (!FactoryReset::getInstance().factoryReset(resetCode)) {
		return command_result_t(ERR_WRONG_PARAMETER);
	}

	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdStateGet(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "state get");
	if (commandData.len < sizeof(state_packet_header_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}
	state_packet_header_t* stateHeader = (state_packet_header_t*) commandData.data;
	CS_TYPE stateType = toCsType(stateHeader->stateType);
	if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelGet(stateType), accessLevel)) {
		return command_result_t(ERR_NO_ACCESS);
	}

	if (resultData.len < sizeof(state_packet_header_t)) {
		return command_result_t(ERR_BUFFER_TOO_SMALL);
	}
	cs_data_t stateDataBuf(resultData.data + sizeof(state_packet_header_t) , resultData.len - sizeof(state_packet_header_t));
	state_packet_header_t* resultHeader = (state_packet_header_t*) resultData.data;
	resultHeader->stateType = stateHeader->stateType;
	cs_state_data_t stateData(stateType, stateDataBuf.data, stateDataBuf.len);
	command_result_t result;
	result.returnCode = State::getInstance().verifySizeForGet(stateData);
	if (FAILURE(result.returnCode)) {
		return result;
	}
	result.returnCode = State::getInstance().get(stateData);
	result.data.data = resultData.data;
	result.data.len = sizeof(state_packet_header_t) + stateData.size;
	return result;
}

command_result_t CommandHandler::handleCmdStateSet(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "state set");
	if (commandData.len < sizeof(state_packet_header_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}
	state_packet_header_t* stateHeader = (state_packet_header_t*) commandData.data;
	CS_TYPE stateType = toCsType(stateHeader->stateType);
	if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelSet(stateType), accessLevel)) {
		return command_result_t(ERR_NO_ACCESS);
	}
	uint16_t payloadSize = commandData.len - sizeof(state_packet_header_t);
	buffer_ptr_t payload = commandData.data + sizeof(state_packet_header_t);
	cs_state_data_t stateData(stateType, payload, payloadSize);
	command_result_t result;
	result.returnCode = State::getInstance().verifySizeForSet(stateData);
	if (FAILURE(result.returnCode)) {
		return result;
	}
	result.returnCode = State::getInstance().set(stateData);
	return result;
}


command_result_t CommandHandler::handleCmdSetTime(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "set time:");

	if (commandData.len != sizeof(uint32_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	uint32_t value = reinterpret_cast<uint32_t*>(commandData.data)[0];

	SystemTime::setTime(value);

	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdSetSunTime(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData){
	LOGi(STR_HANDLE_COMMAND, "set time:");

	if (commandData.len != 2 * sizeof(uint32_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}
	
	set_sun_time_t t = {
		reinterpret_cast<uint32_t*>(commandData.data)[0],
		reinterpret_cast<uint32_t*>(commandData.data)[1]
	};

	State::getInstance().set(CS_TYPE::STATE_SUN_TIME, &t, sizeof(t));

	TimeOfDay sr(t.sunrise);
	TimeOfDay ss(t.sunset);
	
	LOGd("Sunrise %02d:%02d:%02d, sunset %02d:%02d:%02d", sr.h(),sr.m(),sr.s(), ss.h(),ss.m(),ss.s());

	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdIncreaseTx(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "increase TX");
	Advertiser::getInstance().changeToNormalTxPower();
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdSetup(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "setup");
	cs_ret_code_t errCode = Setup::getInstance().handleCommand(commandData);
	return command_result_t(errCode);
}

command_result_t CommandHandler::handleCmdDisconnect(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "disconnect");
	Stack::getInstance().disconnect();
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdResetErrors(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "reset errors");
	if (commandData.len != sizeof(state_errors_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}
	state_errors_t* payload = (state_errors_t*) commandData.data;
	TYPIFY(STATE_ERRORS) stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	LOGd("old errors %u - reset %u", stateErrors.asInt, payload->asInt);
	stateErrors.asInt &= ~(payload->asInt);
	LOGd("new errors %u", stateErrors.asInt);
	State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdPwm(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		return command_result_t(ERR_NOT_AVAILABLE);
	}

	LOGi(STR_HANDLE_COMMAND, "PWM");

	if (commandData.len != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) commandData.data;
	
	TYPIFY(CMD_SET_DIMMER) switch_cmd;
	switch_cmd = payload->switchState & ~0b10000000; // peels of the relay state

	event_t evt(CS_TYPE::CMD_SET_DIMMER, &switch_cmd, sizeof(switch_cmd));
	EventDispatcher::getInstance().dispatch(evt);

	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdSwitch(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		return command_result_t(ERR_NOT_AVAILABLE);
	}

	LOGi(STR_HANDLE_COMMAND, "switch");

	if (commandData.len != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) commandData.data;

	TYPIFY(CMD_SWITCH) switch_cmd;
	switch_cmd.switchCmd = payload->switchState;
	event_t evt(CS_TYPE::CMD_SWITCH, &switch_cmd, sizeof(switch_cmd));
	EventDispatcher::getInstance().dispatch(evt);

	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdRelay(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		return command_result_t(ERR_NOT_AVAILABLE);
	}

	LOGi(STR_HANDLE_COMMAND, "relay");

	if (commandData.len != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) commandData.data;
	TYPIFY(CMD_SET_RELAY) relay_switch_state;
	relay_switch_state = payload->switchState != 0;
	event_t evt(CS_TYPE::CMD_SET_RELAY, &relay_switch_state, sizeof(relay_switch_state));
	EventDispatcher::getInstance().dispatch(evt);
	
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdMultiSwitch(cs_data_t commandData, const cmd_source_t source, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "multi switch");
	multi_switch_t* multiSwitchPacket = (multi_switch_t*)commandData.data;
	if (!cs_multi_switch_packet_is_valid(multiSwitchPacket, commandData.len)) {
		LOGw("invalid message");
		return command_result_t(ERR_INVALID_MESSAGE);
	}
	for (int i=0; i<multiSwitchPacket->count; ++i) {
		TYPIFY(CMD_MULTI_SWITCH) item;
		item.id = multiSwitchPacket->items[i].id;
		item.cmd.switchCmd = multiSwitchPacket->items[i].switchCmd;
		item.cmd.delay = 0;
		item.cmd.source = source;
		if (cs_multi_switch_item_is_valid(&item, sizeof(item))) {
			event_t cmd(CS_TYPE::CMD_MULTI_SWITCH, &item, sizeof(item));
			EventDispatcher::getInstance().dispatch(cmd);
		}
		else {
			LOGw("invalid item ind=%u id=%u", i, item.id);
		}
	}
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdMeshCommand(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "mesh command");
	uint16_t size = commandData.len;
	buffer_ptr_t buffer = commandData.data;
	BLEutil::printArray(buffer, size);
//#if BUILD_MESHING == 1
	// Only support control command NOOP and SET_TIME for now, with idCount of 0. These are the only ones used by the app.
	// Command packet: type, flags, count, {control packet: type, type, length, length, payload...}
	//                 0     1      2                       3     4     5      6       7
	//                 00    00     00                      12    00    00     00
	//                 00    00     00                      30    00    04     00      56 34 12 00
	if (size < 3) {
		return command_result_t(ERR_BUFFER_TOO_SMALL);
	}
	// Check command type, flags, id count.
	if (buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 0) {
		return command_result_t(ERR_NOT_IMPLEMENTED);
	}
	if (size < 3+4) {
		return command_result_t(ERR_BUFFER_TOO_SMALL);
	}
	uint16_t payloadSize = *((uint16_t*)&(buffer[5]));
	uint8_t* payload = &(buffer[7]);
	if (size < 3+4+payloadSize) {
		return command_result_t(ERR_BUFFER_TOO_SMALL);
	}
	// Check control type and payload size
	uint8_t cmdType = buffer[3];
	switch (cmdType) {
	case CTRL_CMD_NOP:{
		if (payloadSize != 0) {
			return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
		}
		break;
	}
	case CTRL_CMD_SET_TIME:{
		if (payloadSize != sizeof(uint32_t)) {
			return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
		}
		break;
	}
	default:
		return command_result_t(ERR_NOT_IMPLEMENTED);
	}
	// Check access
	EncryptionAccessLevel requiredAccessLevel = getRequiredAccessLevel((CommandHandlerTypes)cmdType);
	if (!EncryptionHandler::getInstance().allowAccess(requiredAccessLevel, accessLevel)) {
		return command_result_t(ERR_NO_ACCESS);
	}

	cs_mesh_msg_t meshMsg;
	switch (cmdType) {
	case CTRL_CMD_NOP:
		meshMsg.type = CS_MESH_MODEL_TYPE_CMD_NOOP;
		meshMsg.payload = payload;
		meshMsg.size = payloadSize;
		meshMsg.reliability = CS_MESH_RELIABILITY_LOW;
		meshMsg.urgency = CS_MESH_URGENCY_LOW;
		break;
	case CTRL_CMD_SET_TIME:
		meshMsg.type = CS_MESH_MODEL_TYPE_CMD_TIME;
		meshMsg.payload = payload;
		meshMsg.size = payloadSize;
		meshMsg.reliability = CS_MESH_RELIABILITY_MEDIUM;
		meshMsg.urgency = CS_MESH_URGENCY_LOW; // Timestamp in message gets updated before actually sending.
		break;
	default:
		return command_result_t(ERR_NOT_IMPLEMENTED);
	}
	event_t cmd(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	EventDispatcher::getInstance().dispatch(cmd);
//#endif
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdAllowDimming(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "allow dimming");

	if (commandData.len != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) commandData.data;
	TYPIFY(CONFIG_PWM_ALLOWED) enable = payload->enable;

	LOGi("allow dimming: %u", enable);

	event_t evt(CS_TYPE::CMD_DIMMING_ALLOWED,&enable,sizeof(enable));
	EventDispatcher::getInstance().dispatch(evt);
	
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdLockSwitch(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "lock switch");

	if (commandData.len != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) commandData.data;
	TYPIFY(CONFIG_SWITCH_LOCKED) allow_switching = !payload->enable;

	LOGi("lock switch: %u", !allow_switching);

	event_t evt(CS_TYPE::CMD_SWITCH_LOCKED,&allow_switching,sizeof(allow_switching));
	EventDispatcher::getInstance().dispatch(evt);

	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdUartMsg(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGd(STR_HANDLE_COMMAND, "UART msg");

	if (!commandData.len) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_BLE_MSG, commandData.data, commandData.len);
	return command_result_t(ERR_SUCCESS);
}

EncryptionAccessLevel CommandHandler::getRequiredAccessLevel(const CommandHandlerTypes type) {
	switch (type) {
	case CTRL_CMD_INCREASE_TX:
	case CTRL_CMD_SETUP:
		return BASIC; // These commands are only available in setup mode.

	case CTRL_CMD_SWITCH:
	case CTRL_CMD_PWM:
	case CTRL_CMD_RELAY:
	case CTRL_CMD_DISCONNECT:
	case CTRL_CMD_NOP:
	case CTRL_CMD_MULTI_SWITCH:
	case CTRL_CMD_MESH_COMMAND:
	case CTRL_CMD_STATE_GET:
	case CTRL_CMD_STATE_SET:
		return BASIC;

	case CTRL_CMD_SET_TIME:
	case CTRL_CMD_SET_SUN_TIME:
	case CTRL_CMD_SAVE_BEHAVIOUR:
	case CTRL_CMD_REPLACE_BEHAVIOUR:
	case CTRL_CMD_REMOVE_BEHAVIOUR:
	case CTRL_CMD_GET_BEHAVIOUR:
	case CTRL_CMD_GET_BEHAVIOUR_INDICES:
		return MEMBER;

	case CTRL_CMD_GOTO_DFU:
	case CTRL_CMD_RESET:
	case CTRL_CMD_FACTORY_RESET:
	case CTRL_CMD_RESET_ERRORS:
	case CTRL_CMD_ALLOW_DIMMING:
	case CTRL_CMD_LOCK_SWITCH:
	case CTRL_CMD_UART_MSG:
		return ADMIN;
	case CTRL_CMD_UNKNOWN:
		return NOT_SET;
	}
	return NOT_SET;
}

bool CommandHandler::allowedAsMeshCommand(const CommandHandlerTypes type) {
	switch (type) {
	case CTRL_CMD_SWITCH:
	case CTRL_CMD_PWM:
	case CTRL_CMD_RELAY:
	case CTRL_CMD_SET_TIME:
	case CTRL_CMD_RESET:
	case CTRL_CMD_FACTORY_RESET:
	case CTRL_CMD_RESET_ERRORS:
	case CTRL_CMD_UART_MSG:
		return true;
	default:
		return false;
	}
	return false;
}

void CommandHandler::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::CMD_RESET_DELAYED: {
			auto payload = reinterpret_cast<TYPIFY(CMD_RESET_DELAYED)*>(event.data);
			resetDelayed(payload->resetCode, payload->delayMs);
			break;
		}
		case CS_TYPE::CMD_CONTROL_CMD: {
			auto cmd = reinterpret_cast<TYPIFY(CMD_CONTROL_CMD)*>(event.data);
			uint8_t result_buffer[300] = {};

			auto result = handleCommand(
				cmd->type, 
				cs_data_t(cmd->data, cmd->size), 
				cmd->source, 
				cmd->accessLevel,
				cs_data_t(result_buffer,sizeof(result_buffer))
			);

			LOGd("control command result.returnCode %d, len: %d", result.returnCode,result.data.len);
			for(auto i = 0; i < 50; i+=10){
				LOGd("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
				result_buffer[i+0],result_buffer[i+1],result_buffer[i+2],result_buffer[i+3],result_buffer[i+4],
				result_buffer[i+5],result_buffer[i+6],result_buffer[i+7],result_buffer[i+8],result_buffer[i+9]);
			}
			
			break;
		}
		default: {}
	}
}

command_result_t CommandHandler::dispatchEventForCommand(CS_TYPE typ, cs_data_t commandData, cs_data_t resultData){
	event_t event(typ, commandData.data, commandData.len);
	event.result.buf = resultData;

	event.dispatch();
	
	command_result_t cmdResult;
	cmdResult.returnCode = event.result.returnCode;
	cmdResult.data.data = event.result.buf.data;
	cmdResult.data.len = event.result.dataSize;

	return cmdResult;
}
