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
#include <ipc/cs_IpcRamData.h>
#include "processing/cs_CommandHandler.h"
#include "processing/cs_Scanner.h"
#include "processing/cs_FactoryReset.h"
#include "processing/cs_Setup.h"
#include "processing/cs_TemperatureGuard.h"
#include "protocol/cs_UartProtocol.h"
#include "protocol/mesh/cs_MeshModelPacketHelper.h"
#include "storage/cs_State.h"
#include "time/cs_SystemTime.h"

#include <util/cs_WireFormat.h>

#include <sstream>

#define LOGCommandHandlerDebug LOGnone

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
	switch (type) {
		case CTRL_CMD_SET_SUN_TIME:
			break;
		case CTRL_CMD_NOP:
		case CTRL_CMD_GOTO_DFU:
		case CTRL_CMD_GET_BOOTLOADER_VERSION:
		case CTRL_CMD_RESET:
		case CTRL_CMD_FACTORY_RESET:
		case CTRL_CMD_SET_TIME:
		case CTRL_CMD_INCREASE_TX:
		case CTRL_CMD_DISCONNECT:
		case CTRL_CMD_RESET_ERRORS:
		case CTRL_CMD_PWM:
		case CTRL_CMD_SWITCH:
		case CTRL_CMD_RELAY:
		case CTRL_CMD_MULTI_SWITCH:
		case CTRL_CMD_MESH_COMMAND:
		case CTRL_CMD_ALLOW_DIMMING:
		case CTRL_CMD_LOCK_SWITCH:
		case CTRL_CMD_SETUP:
		case CTRL_CMD_UART_MSG:
		case CTRL_CMD_STATE_GET:
		case CTRL_CMD_STATE_SET:
		case CTRL_CMD_SAVE_BEHAVIOUR:
		case CTRL_CMD_REPLACE_BEHAVIOUR:
		case CTRL_CMD_REMOVE_BEHAVIOUR:
		case CTRL_CMD_GET_BEHAVIOUR:
		case CTRL_CMD_GET_BEHAVIOUR_INDICES:
		case CTRL_CMD_GET_BEHAVIOUR_DEBUG:
		case CTRL_CMD_REGISTER_TRACKED_DEVICE:
			LOGd("cmd=%u lvl=%u", type, accessLevel);
			break;
		case CTRL_CMD_UNKNOWN:
		default:
			LOGe("Unknown type: %u", type);
			return command_result_t(ERR_UNKNOWN_TYPE);
	}

	if (!EncryptionHandler::getInstance().allowAccess(getRequiredAccessLevel(type), accessLevel)) {
		return command_result_t(ERR_NO_ACCESS);
	}

	switch (type) {
	case CTRL_CMD_NOP:
		return handleCmdNop(commandData, accessLevel, resultData);
	case CTRL_CMD_GOTO_DFU:
		return handleCmdGotoDfu(commandData, accessLevel, resultData);
	case CTRL_CMD_GET_BOOTLOADER_VERSION:
		return handleCmdGetBootloaderVersion(commandData, accessLevel, resultData);
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
		return handleCmdMeshCommand(commandData, source, accessLevel, resultData);
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
		return dispatchEventForCommand(CS_TYPE::CMD_ADD_BEHAVIOUR, commandData, resultData);
	case CTRL_CMD_REPLACE_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::CMD_REPLACE_BEHAVIOUR, commandData, resultData);
	case CTRL_CMD_REMOVE_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::CMD_REMOVE_BEHAVIOUR, commandData, resultData);
	case CTRL_CMD_GET_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::CMD_GET_BEHAVIOUR, commandData, resultData);
	case CTRL_CMD_GET_BEHAVIOUR_INDICES:
		return dispatchEventForCommand(CS_TYPE::CMD_GET_BEHAVIOUR_INDICES, commandData, resultData);
	case CTRL_CMD_GET_BEHAVIOUR_DEBUG:
		return dispatchEventForCommand(CS_TYPE::CMD_GET_BEHAVIOUR_DEBUG, commandData, resultData);
	case CTRL_CMD_REGISTER_TRACKED_DEVICE:
		return handleCmdRegisterTrackedDevice(commandData, accessLevel, resultData);
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
	event_t event(CS_TYPE::EVT_GOING_TO_DFU);
	event.dispatch();
	resetDelayed(GPREGRET_DFU_RESET);
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdGetBootloaderVersion(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "get bootloader version");

	uint8_t dataSize;
	int retCode = getRamData(IPC_INDEX_BOOTLOADER_VERSION, resultData.data, resultData.len, &dataSize);
	if (retCode != IPC_RET_SUCCESS) {
		LOGw("No IPC data found, error = %i", retCode);
		return command_result_t(ERR_NOT_FOUND);
	}
	command_result_t result;
	result.returnCode = ERR_SUCCESS;
	result.data.data = resultData.data;
	result.data.len = dataSize;
	return result;
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
	LOGi("State type=%u id=%u persistenceMode=%u", stateHeader->stateType, stateHeader->stateId, stateHeader->persistenceMode);
	CS_TYPE stateType = toCsType(stateHeader->stateType);
	cs_state_id_t stateId = stateHeader->stateId;
	if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelGet(stateType), accessLevel)) {
		return command_result_t(ERR_NO_ACCESS);
	}

	if (resultData.len < sizeof(state_packet_header_t)) {
		return command_result_t(ERR_BUFFER_TOO_SMALL);
	}
	cs_data_t stateDataBuf(resultData.data + sizeof(state_packet_header_t) , resultData.len - sizeof(state_packet_header_t));
	state_packet_header_t* resultHeader = (state_packet_header_t*) resultData.data;
	resultHeader->stateType = stateHeader->stateType;
	resultHeader->stateId = stateHeader->stateId;
	resultHeader->persistenceMode = stateHeader->persistenceMode;
	cs_state_data_t stateData(stateType, stateId, stateDataBuf.data, stateDataBuf.len);
	command_result_t result;
	result.returnCode = State::getInstance().verifySizeForGet(stateData);
	result.data.data = resultData.data;
	result.data.len = sizeof(state_packet_header_t);
	if (FAILURE(result.returnCode)) {
		return result;
	}
	PersistenceMode persistenceMode = PersistenceMode::NEITHER_RAM_NOR_FLASH;
	PersistenceModeGet persistenceModeGet = toPersistenceModeGet(stateHeader->persistenceMode);
	switch (persistenceModeGet) {
		case PersistenceModeGet::CURRENT:
			persistenceMode = PersistenceMode::STRATEGY1;
			break;
		case PersistenceModeGet::STORED:
			persistenceMode = PersistenceMode::FLASH;
			break;
		case PersistenceModeGet::FIRMWARE_DEFAULT:
			persistenceMode = PersistenceMode::FIRMWARE_DEFAULT;
			break;
		case PersistenceModeGet::UNKNOWN:
			break;
	}
	result.returnCode = State::getInstance().get(stateData, persistenceMode);

	if (persistenceModeGet == PersistenceModeGet::STORED && result.returnCode == ERR_NOT_FOUND) {
		// Try default instead.
		result.returnCode = State::getInstance().get(stateData, PersistenceMode::FIRMWARE_DEFAULT);
	}

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
	LOGi("State type=%u id=%u persistenceMode=%u", stateHeader->stateType, stateHeader->stateId, stateHeader->persistenceMode);
	CS_TYPE stateType = toCsType(stateHeader->stateType);
	cs_state_id_t stateId = stateHeader->stateId;
	if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelSet(stateType), accessLevel)) {
		return command_result_t(ERR_NO_ACCESS);
	}
	if (resultData.len < sizeof(state_packet_header_t)) {
		return command_result_t(ERR_BUFFER_TOO_SMALL);
	}
	state_packet_header_t* resultHeader = (state_packet_header_t*) resultData.data;
	resultHeader->stateType = stateHeader->stateType;
	resultHeader->stateId = stateHeader->stateId;
	resultHeader->persistenceMode = stateHeader->persistenceMode;
	uint16_t payloadSize = commandData.len - sizeof(state_packet_header_t);
	buffer_ptr_t payload = commandData.data + sizeof(state_packet_header_t);
	cs_state_data_t stateData(stateType, stateId, payload, payloadSize);
	command_result_t result;
	result.returnCode = State::getInstance().verifySizeForSet(stateData);
	result.data.data = resultData.data;
	result.data.len = sizeof(state_packet_header_t);
	if (FAILURE(result.returnCode)) {
		return result;
	}
	PersistenceMode persistenceMode = PersistenceMode::NEITHER_RAM_NOR_FLASH;
	PersistenceModeSet persistenceModeSet = toPersistenceModeSet(stateHeader->persistenceMode);
	switch (persistenceModeSet) {
		case PersistenceModeSet::TEMPORARY:
			persistenceMode = PersistenceMode::RAM;
			break;
		case PersistenceModeSet::STORED:
			persistenceMode = PersistenceMode::FLASH;
			break;
		case PersistenceModeSet::UNKNOWN:
			break;
	}
	cs_ret_code_t retCode = State::getInstance().set(stateData, persistenceMode);
	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE:
			result.returnCode = ERR_SUCCESS;
			break;
		default:
			result.returnCode = retCode;
	}
	return result;
}

command_result_t CommandHandler::handleCmdSetTime(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGCommandHandlerDebug(STR_HANDLE_COMMAND, "set time:");
	if (commandData.len != sizeof(uint32_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}
	uint32_t value = reinterpret_cast<uint32_t*>(commandData.data)[0];
	SystemTime::setTime(value);
	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdSetSunTime(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData){
	LOGCommandHandlerDebug(STR_HANDLE_COMMAND, "set sun time:");
	if (commandData.len != sizeof(sun_time_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}
	sun_time_t* payload = reinterpret_cast<sun_time_t*>(commandData.data);
	if (payload->sunrise > 24*60*60 || payload->sunset > 24*60*60) {
		LOGw("Invalid suntimes: rise=%u set=%u", payload->sunrise, payload->sunset);
		return command_result_t(ERR_WRONG_PARAMETER);
	}
	TYPIFY(STATE_SUN_TIME) sunTime = *payload;
	cs_state_data_t stateData = cs_state_data_t(CS_TYPE::STATE_SUN_TIME, reinterpret_cast<uint8_t*>(&sunTime), sizeof(sunTime));
	State::getInstance().setThrottled(stateData, SUN_TIME_THROTTLE_PERIOD_SECONDS);
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
	
	TYPIFY(CMD_SET_DIMMER) switchCmd;
	switchCmd = payload->switchState;

	event_t evt(CS_TYPE::CMD_SET_DIMMER, &switchCmd, sizeof(switchCmd));
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

command_result_t CommandHandler::handleCmdMeshCommand(cs_data_t commandData, const cmd_source_t source, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "mesh command");
	uint16_t size = commandData.len;
	buffer_ptr_t buffer = commandData.data;
	BLEutil::printArray(buffer, size);

	// Keep up the required size, and where in the buffer we are.
	uint16_t bufIndex = 0;
	size16_t requiredSize = 0;
	TYPIFY(CMD_SEND_MESH_CONTROL_COMMAND) meshCtrlCmd;

	// Mesh command header.
	requiredSize += sizeof(meshCtrlCmd.header);
	LOGCommandHandlerDebug("requiredSize = %u", requiredSize);
	if (size < requiredSize) {
		LOGd("too small for header size=%u required=%u", size, requiredSize);
		return command_result_t(ERR_INVALID_MESSAGE);
	}
	memcpy(&(meshCtrlCmd.header), &(buffer[bufIndex]), sizeof(meshCtrlCmd.header));
	bufIndex += sizeof(meshCtrlCmd.header);

	if (meshCtrlCmd.header.type != 0) {
		return command_result_t(ERR_WRONG_PARAMETER);
	}

	// List of IDs.
	requiredSize += meshCtrlCmd.header.idCount * sizeof(stone_id_t);
	LOGCommandHandlerDebug("requiredSize = %u", requiredSize);
	if (size < requiredSize) {
		LOGd("too small for ids size=%u required=%u", size, requiredSize);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}
	meshCtrlCmd.targetIds = &(buffer[bufIndex]);
	bufIndex += meshCtrlCmd.header.idCount;

	// Control command header
	control_packet_header_t controlPacketHeader;
	requiredSize += sizeof(controlPacketHeader);
	LOGCommandHandlerDebug("requiredSize = %u", requiredSize);
	if (size < requiredSize) {
		LOGd("too small for control header size=%u required=%u", size, requiredSize);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}
	memcpy(&controlPacketHeader, &(buffer[bufIndex]), sizeof(controlPacketHeader));
	bufIndex += sizeof(controlPacketHeader);

	// Control command payload
	requiredSize += controlPacketHeader.payloadSize;
	LOGCommandHandlerDebug("requiredSize = %u", requiredSize);
	if (size < requiredSize) {
		LOGd("too small for control payload size=%u required=%u", size, requiredSize);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}
	meshCtrlCmd.controlCommand.type = (CommandHandlerTypes) controlPacketHeader.commandType;
	meshCtrlCmd.controlCommand.data = &(buffer[bufIndex]);
	meshCtrlCmd.controlCommand.size = controlPacketHeader.payloadSize;
	meshCtrlCmd.controlCommand.accessLevel = accessLevel;
	meshCtrlCmd.controlCommand.source = source;

	// Check permissions
	CommandHandlerTypes controlCmdType = meshCtrlCmd.controlCommand.type;
	if (!allowedAsMeshCommand(controlCmdType)) {
		LOGw("Command %u is not allowed via mesh", controlCmdType);
		return command_result_t(ERR_NOT_AVAILABLE);
	}
	if (!EncryptionHandler::getInstance().allowAccess(getRequiredAccessLevel(controlCmdType), accessLevel)) {
		LOGw("No access for command %u", controlCmdType);
		return command_result_t(ERR_NO_ACCESS);
	}

	// Handle command if the command is for this stone.
	bool forSelf = (meshCtrlCmd.header.idCount == 0);
	bool forOthers = (meshCtrlCmd.header.idCount == 0);
	TYPIFY(CONFIG_CROWNSTONE_ID) ownId = 0;
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &ownId, sizeof(ownId));
	for (uint8_t i = 0; i < meshCtrlCmd.header.idCount; ++i) {
		forSelf = forSelf || (meshCtrlCmd.targetIds[i] == ownId);
		forOthers = forOthers || (meshCtrlCmd.targetIds[i] != ownId);
	}
	if (forSelf) {
		cs_data_t meshCommandCtrlCmdData(meshCtrlCmd.controlCommand.data, meshCtrlCmd.controlCommand.size);
		command_result_t cmdResult = handleCommand(meshCtrlCmd.controlCommand.type, meshCommandCtrlCmdData, source, accessLevel, resultData);
		if (!forOthers) {
			return cmdResult;
		}
	}
	if (!forOthers) {
		return command_result_t(ERR_NOT_FOUND);
	}

	// Check nested permissions
	EncryptionAccessLevel requiredAccessLevel = ENCRYPTION_DISABLED;
	switch (controlCmdType) {
		case CTRL_CMD_STATE_SET:
		case CTRL_CMD_STATE_GET: {
			if (meshCtrlCmd.controlCommand.size < sizeof(state_packet_header_t)) {
				LOGd("too small for state packet header");
				return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
			}
			state_packet_header_t* stateHeader = (state_packet_header_t*) meshCtrlCmd.controlCommand.data;
			LOGd("State type=%u id=%u persistenceMode=%u", stateHeader->stateType, stateHeader->stateId, stateHeader->persistenceMode);
			CS_TYPE stateType = toCsType(stateHeader->stateType);
			requiredAccessLevel = (controlCmdType == CTRL_CMD_STATE_SET) ? getUserAccessLevelSet(stateType) : getUserAccessLevelGet(stateType);
			break;
		}
		default:
			break;
	}
	if (!EncryptionHandler::getInstance().allowAccess(getRequiredAccessLevel(controlCmdType), accessLevel)) {
		LOGw("No access for command payload. Required=%u", requiredAccessLevel);
		return command_result_t(ERR_NO_ACCESS);
	}

	// All permission checks must have been done already!
	// Also the nested ones!
	cs_data_t eventData((buffer_ptr_t)&meshCtrlCmd, sizeof(meshCtrlCmd));
	return dispatchEventForCommand(CS_TYPE::CMD_SEND_MESH_CONTROL_COMMAND, eventData, resultData);




//	// Only support control command NOOP and SET_TIME for now, with idCount of 0. These are the only ones used by the app.
//	// Command packet: type, flags, count, {control packet: type, type, length, length, payload...}
//	//                 0     1      2                       3     4     5      6       7
//	//                 00    00     00                      12    00    00     00
//	//                 00    00     00                      30    00    04     00      56 34 12 00
//	// Check command type, flags, id count.
//	if (buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 0) {
//		return command_result_t(ERR_NOT_IMPLEMENTED);
//	}
//	if (size < 3+4) {
//		return command_result_t(ERR_BUFFER_TOO_SMALL);
//	}
//	uint16_t payloadSize = *((uint16_t*)&(buffer[5]));
//	uint8_t* payload = &(buffer[7]);
//	if (size < 3+4+payloadSize) {
//		return command_result_t(ERR_BUFFER_TOO_SMALL);
//	}
//	// Check control type and payload size
//	uint8_t cmdType = buffer[3];
//	switch (cmdType) {
//	case CTRL_CMD_NOP:{
//		if (payloadSize != 0) {
//			return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
//		}
//		break;
//	}
//	case CTRL_CMD_SET_TIME:{
//		if (payloadSize != sizeof(uint32_t)) {
//			return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
//		}
//		break;
//	}
//	default:
//		return command_result_t(ERR_NOT_IMPLEMENTED);
//	}
//	// Check access
//	EncryptionAccessLevel requiredAccessLevel = getRequiredAccessLevel((CommandHandlerTypes)cmdType);
//	if (!EncryptionHandler::getInstance().allowAccess(requiredAccessLevel, accessLevel)) {
//		return command_result_t(ERR_NO_ACCESS);
//	}
//
//	cs_mesh_msg_t meshMsg;
//	switch (cmdType) {
//		case CTRL_CMD_NOP: {
//			meshMsg.type = CS_MESH_MODEL_TYPE_CMD_NOOP;
//			meshMsg.payload = payload;
//			meshMsg.size = payloadSize;
//			meshMsg.reliability = CS_MESH_RELIABILITY_LOW;
//			meshMsg.urgency = CS_MESH_URGENCY_LOW;
//			break;
//		}
//		case CTRL_CMD_SET_TIME: {
//			meshMsg.type = CS_MESH_MODEL_TYPE_CMD_TIME;
//			meshMsg.payload = payload;
//			meshMsg.size = payloadSize;
//			meshMsg.reliability = CS_MESH_RELIABILITY_MEDIUM;
//			meshMsg.urgency = CS_MESH_URGENCY_HIGH;
//			break;
//		}
//		default:
//			return command_result_t(ERR_NOT_IMPLEMENTED);
//	}
//	event_t cmd(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
//	EventDispatcher::getInstance().dispatch(cmd);
//
//	// Also handle command on this crownstone.
//	if (cmdType == CTRL_CMD_SET_TIME) {
//		event_t event(CS_TYPE::CMD_SET_TIME, payload, payloadSize);
//		EventDispatcher::getInstance().dispatch(event);
//	}
//	return command_result_t(ERR_SUCCESS);
}

command_result_t CommandHandler::handleCmdAllowDimming(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "allow dimming");

	if (commandData.len != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) commandData.data;
	TYPIFY(CONFIG_PWM_ALLOWED) allow = payload->enable;

	event_t evt(CS_TYPE::CMD_DIMMING_ALLOWED, &allow, sizeof(allow));
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
	TYPIFY(CMD_SWITCHING_ALLOWED) allow = !payload->enable;

	event_t evt(CS_TYPE::CMD_SWITCHING_ALLOWED, &allow, sizeof(allow));
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

command_result_t CommandHandler::handleCmdRegisterTrackedDevice(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_data_t resultData) {
	LOGi(STR_HANDLE_COMMAND, "register tracked device");
	if (commandData.len != sizeof(register_tracked_device_packet_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		return command_result_t(ERR_WRONG_PAYLOAD_LENGTH);
	}

	TYPIFY(CMD_REGISTER_TRACKED_DEVICE) evtData;
	evtData.data = *((register_tracked_device_packet_t*)commandData.data);
	evtData.accessLevel = accessLevel;
	event_t event(CS_TYPE::CMD_REGISTER_TRACKED_DEVICE, &evtData, sizeof(evtData));
	event.result.buf = resultData;
	event.dispatch();

	return command_result_t(event.result.returnCode);
}

command_result_t CommandHandler::dispatchEventForCommand(CS_TYPE typ, cs_data_t commandData, cs_data_t resultData) {
	event_t event(typ, commandData.data, commandData.len);
	event.result.buf = resultData;
	event.dispatch();

	command_result_t cmdResult;
	cmdResult.returnCode = event.result.returnCode;
	cmdResult.data.data = event.result.buf.data;
	cmdResult.data.len = event.result.dataSize;

	return cmdResult;
}

EncryptionAccessLevel CommandHandler::getRequiredAccessLevel(const CommandHandlerTypes type) {
	switch (type) {
		case CTRL_CMD_GET_BOOTLOADER_VERSION:
			return ENCRYPTION_DISABLED;

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
		case CTRL_CMD_REGISTER_TRACKED_DEVICE:
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
		case CTRL_CMD_GET_BEHAVIOUR_DEBUG:
			return ADMIN;
		case CTRL_CMD_UNKNOWN:
			return NOT_SET;
	}
	return NOT_SET;
}

bool CommandHandler::allowedAsMeshCommand(const CommandHandlerTypes type) {
	switch (type) {
		case CTRL_CMD_FACTORY_RESET:
		case CTRL_CMD_NOP:
		case CTRL_CMD_RESET_ERRORS:
		case CTRL_CMD_RESET:
		case CTRL_CMD_SET_TIME:
		case CTRL_CMD_STATE_SET:
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

			[[maybe_unused]] auto result = handleCommand(
				cmd->type, 
				cs_data_t(cmd->data, cmd->size), 
				cmd->source, 
				cmd->accessLevel,
				cs_data_t(result_buffer,sizeof(result_buffer))
			);

			LOGCommandHandlerDebug("control command result.returnCode %d, len: %d", result.returnCode,result.data.len);
			for(auto i = 0; i < 50; i+=10){
				LOGCommandHandlerDebug("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
				result_buffer[i+0],result_buffer[i+1],result_buffer[i+2],result_buffer[i+3],result_buffer[i+4],
				result_buffer[i+5],result_buffer[i+6],result_buffer[i+7],result_buffer[i+8],result_buffer[i+9]);
			}
			
			break;
		}
		default: {}
	}
}
