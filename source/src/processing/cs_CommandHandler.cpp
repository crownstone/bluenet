/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 18, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Advertiser.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_DeviceTypes.h>
#include <cfg/cs_Strings.h>
#include <drivers/cs_GpRegRet.h>
#include <drivers/cs_Serial.h>
#include <ipc/cs_IpcRamData.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_FactoryReset.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_Setup.h>
#include <processing/cs_TemperatureGuard.h>
#include <protocol/cs_UartProtocol.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <util/cs_WireFormat.h>

#define LOGCommandHandlerDebug LOGnone

void reset(void* p_context) {

	uint8_t cmd = *(uint8_t*) p_context;

	switch (cmd) {
		case CS_RESET_CODE_SOFT_RESET: {
			LOGi(MSG_RESET);
			GpRegRet::setCounter(CS_GPREGRET_COUNTER_SOFT_RESET);
			break;
		}
		case CS_RESET_CODE_GO_TO_DFU_MODE: {
			LOGi(MSG_FIRMWARE_UPDATE);
			GpRegRet::setFlag(GpRegRet::FLAG_DFU);
			break;
		}
		default:
			LOGw("Unknown reset code: %u", cmd);
			return;
	}
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

void CommandHandler::handleCommand(
		uint8_t protocolVersion,
		const CommandHandlerTypes type,
		cs_data_t commandData,
		const cmd_source_t source,
		const EncryptionAccessLevel accessLevel,
		cs_result_t & result
		) {
	if (protocolVersion != CS_CONNECTION_PROTOCOL_VERSION) {
		LOGw("Wrong protocol: %u", protocolVersion);
		result.returnCode = ERR_PROTOCOL_UNSUPPORTED;
		return;
	}

	switch (type) {
		case CTRL_CMD_SET_SUN_TIME:
			break;
		case CTRL_CMD_NOP:
		case CTRL_CMD_GOTO_DFU:
		case CTRL_CMD_GET_BOOTLOADER_VERSION:
		case CTRL_CMD_GET_UICR_DATA:
		case CTRL_CMD_SET_IBEACON_CONFIG_ID:
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
			result.returnCode = ERR_UNKNOWN_TYPE;
			return;
	}

	if (!EncryptionHandler::getInstance().allowAccess(getRequiredAccessLevel(type), accessLevel)) {
		result.returnCode = ERR_NO_ACCESS;
		return;
	}

	switch (type) {
	case CTRL_CMD_NOP:
		return handleCmdNop(commandData, accessLevel, result);
	case CTRL_CMD_GOTO_DFU:
		return handleCmdGotoDfu(commandData, accessLevel, result);
	case CTRL_CMD_GET_BOOTLOADER_VERSION:
		return handleCmdGetBootloaderVersion(commandData, accessLevel, result);
	case CTRL_CMD_GET_UICR_DATA:
		return handleCmdGetUicrData(commandData, accessLevel, result);
	case CTRL_CMD_RESET:
		return handleCmdReset(commandData, accessLevel, result);
	case CTRL_CMD_FACTORY_RESET:
		return handleCmdFactoryReset(commandData, accessLevel, result);
	case CTRL_CMD_SET_TIME:
		return handleCmdSetTime(commandData, accessLevel, result);
	case CTRL_CMD_SET_SUN_TIME:
		return handleCmdSetSunTime(commandData, accessLevel, result);
	case CTRL_CMD_INCREASE_TX:
		return handleCmdIncreaseTx(commandData, accessLevel, result);
	case CTRL_CMD_DISCONNECT:
		return handleCmdDisconnect(commandData, accessLevel, result);
	case CTRL_CMD_RESET_ERRORS:
		return handleCmdResetErrors(commandData, accessLevel, result);
	case CTRL_CMD_PWM:
		return handleCmdPwm(commandData, accessLevel, result);
	case CTRL_CMD_SWITCH:
		return handleCmdSwitch(commandData, accessLevel, result);
	case CTRL_CMD_RELAY:
		return handleCmdRelay(commandData, accessLevel, result);
	case CTRL_CMD_MULTI_SWITCH:
		return handleCmdMultiSwitch(commandData, source, accessLevel, result);
	case CTRL_CMD_MESH_COMMAND:
		return handleCmdMeshCommand(protocolVersion, commandData, source, accessLevel, result);
	case CTRL_CMD_ALLOW_DIMMING:
		return handleCmdAllowDimming(commandData, accessLevel, result);
	case CTRL_CMD_LOCK_SWITCH:
		return handleCmdLockSwitch(commandData, accessLevel, result);
	case CTRL_CMD_SETUP:
		return handleCmdSetup(commandData, accessLevel, result);
	case CTRL_CMD_UART_MSG:
		return handleCmdUartMsg(commandData, accessLevel, result);
	case CTRL_CMD_STATE_GET:
		return handleCmdStateGet(commandData, accessLevel, result);
	case CTRL_CMD_STATE_SET:
		return handleCmdStateSet(commandData, accessLevel, result);
	case CTRL_CMD_SAVE_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::CMD_ADD_BEHAVIOUR, commandData, result);
	case CTRL_CMD_REPLACE_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::CMD_REPLACE_BEHAVIOUR, commandData, result);
	case CTRL_CMD_REMOVE_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::CMD_REMOVE_BEHAVIOUR, commandData, result);
	case CTRL_CMD_GET_BEHAVIOUR:
		return dispatchEventForCommand(CS_TYPE::CMD_GET_BEHAVIOUR, commandData, result);
	case CTRL_CMD_GET_BEHAVIOUR_INDICES:
		return dispatchEventForCommand(CS_TYPE::CMD_GET_BEHAVIOUR_INDICES, commandData, result);
	case CTRL_CMD_GET_BEHAVIOUR_DEBUG:
		return dispatchEventForCommand(CS_TYPE::CMD_GET_BEHAVIOUR_DEBUG, commandData, result);
	case CTRL_CMD_REGISTER_TRACKED_DEVICE:
		return handleCmdRegisterTrackedDevice(commandData, accessLevel, result);
	case CTRL_CMD_SET_IBEACON_CONFIG_ID:
		return dispatchEventForCommand(CS_TYPE::CMD_SET_IBEACON_CONFIG_ID, commandData, result);
	case CTRL_CMD_UNKNOWN:
		result.returnCode = ERR_UNKNOWN_TYPE;
		return;
	}
	LOGe("Unknown type: %u", type);
	result.returnCode = ERR_UNKNOWN_TYPE;
	return;
}

void CommandHandler::handleCmdNop(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	// A no operation command to keep the connection alive.
	// No need to do anything here, the connection keep alive is handled in the stack.
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdGotoDfu(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "goto dfu");
	event_t event(CS_TYPE::EVT_GOING_TO_DFU);
	event.dispatch();
	resetDelayed(CS_RESET_CODE_GO_TO_DFU_MODE);
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdGetBootloaderVersion(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "get bootloader version");

	uint8_t dataSize;
	int retCode = getRamData(IPC_INDEX_BOOTLOADER_VERSION, result.buf.data, result.buf.len, &dataSize);
	if (retCode != IPC_RET_SUCCESS) {
		LOGw("No IPC data found, error = %i", retCode);
		result.returnCode = ERR_NOT_FOUND;
		return;
	}
	result.dataSize = dataSize;
	result.returnCode = ERR_SUCCESS;
	return;
}

void CommandHandler::handleCmdGetUicrData(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "get UICR data");

	if (result.buf.len < sizeof(cs_uicr_data_t)) {
		result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}

	cs_uicr_data_t* uicrData = (cs_uicr_data_t*)result.buf.data;
	uint8_t uicrIndex = UICR_BOARD_INDEX;
	uicrData->board = NRF_UICR->CUSTOMER[uicrIndex++];
	uicrData->productRegionFamily.asInt = NRF_UICR->CUSTOMER[uicrIndex++];
	uicrData->majorMinorPatch.asInt = NRF_UICR->CUSTOMER[uicrIndex++];
	uicrData->productionDateHousing.asInt = NRF_UICR->CUSTOMER[uicrIndex++];
	LOGd("board=%u", uicrData->board);
	LOGd("productType=%u region=%u productFamily=%u int=0x%X",
			uicrData->productRegionFamily.fields.productType,
			uicrData->productRegionFamily.fields.region,
			uicrData->productRegionFamily.fields.productFamily,
			uicrData->productRegionFamily.asInt);
	LOGd("major=%u minor=%u patch=%u int=0x%X",
			uicrData->majorMinorPatch.fields.major,
			uicrData->majorMinorPatch.fields.minor,
			uicrData->majorMinorPatch.fields.patch,
			uicrData->majorMinorPatch.asInt);
	LOGd("year=%u week=%u housing=%u int=0x%X",
			uicrData->productionDateHousing.fields.year,
			uicrData->productionDateHousing.fields.week,
			uicrData->productionDateHousing.fields.housing,
			uicrData->productionDateHousing.asInt);

	result.dataSize = sizeof(*uicrData);
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdReset(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "reset");
	resetDelayed(CS_RESET_CODE_SOFT_RESET);
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdFactoryReset(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "factory reset");

	if (commandData.len != sizeof(FACTORY_RESET_CODE)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, sizeof(FACTORY_RESET_CODE));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	factory_reset_message_payload_t* payload = (factory_reset_message_payload_t*) commandData.data;
	uint32_t resetCode = payload->resetCode;

	if (!FactoryReset::getInstance().factoryReset(resetCode)) {
		result.returnCode = ERR_WRONG_PARAMETER;
		return;
	}
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdStateGet(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "state get");

	// Check if command data is large enough for header.
	if (commandData.len < sizeof(state_packet_header_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	// Read out header.
	state_packet_header_t* stateHeader = (state_packet_header_t*) commandData.data;
	LOGi("State type=%u id=%u persistenceMode=%u", stateHeader->stateType, stateHeader->stateId, stateHeader->persistenceMode);
	CS_TYPE stateType = toCsType(stateHeader->stateType);
	cs_state_id_t stateId = stateHeader->stateId;

	// Check access.
	if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelGet(stateType), accessLevel)) {
		result.returnCode = ERR_NO_ACCESS;
		return;
	}

	// Check if there's enough space in result buffer for a header.
	if (result.buf.len < sizeof(state_packet_header_t)) {
		result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}

	// Fill the result buffer with a header.
	cs_data_t stateDataBuf(result.buf.data + sizeof(state_packet_header_t) , result.buf.len - sizeof(state_packet_header_t));
	state_packet_header_t* resultHeader = (state_packet_header_t*) result.buf.data;
	resultHeader->stateType = stateHeader->stateType;
	resultHeader->stateId = stateHeader->stateId;
	resultHeader->persistenceMode = stateHeader->persistenceMode;

	// Check if there's enough space in result buffer the state data.
	cs_state_data_t stateData(stateType, stateId, stateDataBuf.data, stateDataBuf.len);
	result.returnCode = State::getInstance().verifySizeForGet(stateData);
	result.dataSize = sizeof(state_packet_header_t);
	if (FAILURE(result.returnCode)) {
		return;
	}

	// Determine persistence mode.
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

	// Get the state value.
	result.returnCode = State::getInstance().get(stateData, persistenceMode);

	if (persistenceModeGet == PersistenceModeGet::STORED && result.returnCode == ERR_NOT_FOUND) {
		// Try default instead.
		result.returnCode = State::getInstance().get(stateData, PersistenceMode::FIRMWARE_DEFAULT);
	}

	result.dataSize = sizeof(state_packet_header_t) + stateData.size;
}

void CommandHandler::handleCmdStateSet(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "state set");

	// Check if command data is large enough for header.
	if (commandData.len < sizeof(state_packet_header_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	// Read out header.
	state_packet_header_t* stateHeader = (state_packet_header_t*) commandData.data;
	LOGi("State type=%u id=%u persistenceMode=%u", stateHeader->stateType, stateHeader->stateId, stateHeader->persistenceMode);
	CS_TYPE stateType = toCsType(stateHeader->stateType);
	cs_state_id_t stateId = stateHeader->stateId;

	// Check access.
	if (!EncryptionHandler::getInstance().allowAccess(getUserAccessLevelSet(stateType), accessLevel)) {
		result.returnCode = ERR_NO_ACCESS;
		return;
	}

	// Check if the state data is of correct size.
	uint16_t payloadSize = commandData.len - sizeof(state_packet_header_t);
	buffer_ptr_t payload = commandData.data + sizeof(state_packet_header_t);
	cs_state_data_t stateData(stateType, stateId, payload, payloadSize);
	result.returnCode = State::getInstance().verifySizeForSet(stateData);

	// If there's enough space in result buffer, fill it up.
	if (result.buf.len >= sizeof(state_packet_header_t)) {
		state_packet_header_t* resultHeader = (state_packet_header_t*) result.buf.data;
		resultHeader->stateType = stateHeader->stateType;
		resultHeader->stateId = stateHeader->stateId;
		resultHeader->persistenceMode = stateHeader->persistenceMode;

		result.dataSize = sizeof(state_packet_header_t);
	}

	if (FAILURE(result.returnCode)) {
		return;
	}

	// Determine persistence mode.
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

	// Finally: set the state value.
	cs_ret_code_t retCode = State::getInstance().set(stateData, persistenceMode);
	switch (retCode) {
		case ERR_SUCCESS:
		case ERR_SUCCESS_NO_CHANGE:
			result.returnCode = ERR_SUCCESS;
			break;
		default:
			result.returnCode = retCode;
	}
}

void CommandHandler::handleCmdSetTime(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGCommandHandlerDebug(STR_HANDLE_COMMAND, "set time:");
	if (commandData.len != sizeof(uint32_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}
	uint32_t value = reinterpret_cast<uint32_t*>(commandData.data)[0];
	SystemTime::setTime(value);
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdSetSunTime(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result){
	LOGCommandHandlerDebug(STR_HANDLE_COMMAND, "set sun time:");
	if (commandData.len != sizeof(sun_time_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}
	sun_time_t* payload = reinterpret_cast<sun_time_t*>(commandData.data);
	if (payload->sunrise > 24*60*60 || payload->sunset > 24*60*60) {
		LOGw("Invalid suntimes: rise=%u set=%u", payload->sunrise, payload->sunset);
		result.returnCode = ERR_WRONG_PARAMETER;
		return;
	}
	TYPIFY(STATE_SUN_TIME) sunTime = *payload;
	cs_state_data_t stateData = cs_state_data_t(CS_TYPE::STATE_SUN_TIME, reinterpret_cast<uint8_t*>(&sunTime), sizeof(sunTime));
	State::getInstance().setThrottled(stateData, SUN_TIME_THROTTLE_PERIOD_SECONDS);
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdIncreaseTx(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "increase TX");
	Advertiser::getInstance().changeToNormalTxPower();
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdSetup(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "setup");
	cs_ret_code_t errCode = Setup::getInstance().handleCommand(commandData);
	result.returnCode = errCode;
}

void CommandHandler::handleCmdDisconnect(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "disconnect");
	Stack::getInstance().disconnect();
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdResetErrors(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "reset errors");
	if (commandData.len != sizeof(state_errors_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}
	state_errors_t* payload = (state_errors_t*) commandData.data;
	TYPIFY(STATE_ERRORS) stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	LOGd("old errors %u - reset %u", stateErrors.asInt, payload->asInt);
	stateErrors.asInt &= ~(payload->asInt);
	LOGd("new errors %u", stateErrors.asInt);
	State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdPwm(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		result.returnCode = ERR_NOT_AVAILABLE;
		return;
	}

	LOGi(STR_HANDLE_COMMAND, "PWM");

	if (commandData.len != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) commandData.data;
	
	TYPIFY(CMD_SET_DIMMER) switchCmd;
	switchCmd = payload->switchState;

	event_t evt(CS_TYPE::CMD_SET_DIMMER, &switchCmd, sizeof(switchCmd));
	EventDispatcher::getInstance().dispatch(evt);

	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdSwitch(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		result.returnCode = ERR_NOT_AVAILABLE;
		return;
	}

	LOGi(STR_HANDLE_COMMAND, "switch");

	if (commandData.len != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) commandData.data;

	TYPIFY(CMD_SWITCH) switch_cmd;
	switch_cmd.switchCmd = payload->switchState;
	event_t evt(CS_TYPE::CMD_SWITCH, &switch_cmd, sizeof(switch_cmd));
	EventDispatcher::getInstance().dispatch(evt);

	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdRelay(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		result.returnCode = ERR_NOT_AVAILABLE;
		return;
	}

	LOGi(STR_HANDLE_COMMAND, "relay");

	if (commandData.len != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) commandData.data;
	TYPIFY(CMD_SET_RELAY) relay_switch_state;
	relay_switch_state = payload->switchState != 0;
	event_t evt(CS_TYPE::CMD_SET_RELAY, &relay_switch_state, sizeof(relay_switch_state));
	EventDispatcher::getInstance().dispatch(evt);
	
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdMultiSwitch(cs_data_t commandData, const cmd_source_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "multi switch");
	multi_switch_t* multiSwitchPacket = (multi_switch_t*)commandData.data;
	if (!cs_multi_switch_packet_is_valid(multiSwitchPacket, commandData.len)) {
		LOGw("invalid message");
		result.returnCode = ERR_INVALID_MESSAGE;
		return;
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
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdMeshCommand(uint8_t protocol, cs_data_t commandData, const cmd_source_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
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
		result.returnCode = ERR_INVALID_MESSAGE;
		return;
	}
	memcpy(&(meshCtrlCmd.header), &(buffer[bufIndex]), sizeof(meshCtrlCmd.header));
	bufIndex += sizeof(meshCtrlCmd.header);

	if (meshCtrlCmd.header.type != 0) {
		result.returnCode = ERR_WRONG_PARAMETER;
		return;
	}

	// List of IDs.
	requiredSize += meshCtrlCmd.header.idCount * sizeof(stone_id_t);
	LOGCommandHandlerDebug("requiredSize = %u", requiredSize);
	if (size < requiredSize) {
		LOGd("too small for ids size=%u required=%u", size, requiredSize);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}
	meshCtrlCmd.targetIds = &(buffer[bufIndex]);
	bufIndex += meshCtrlCmd.header.idCount;

	// Control command header
	control_packet_header_t controlPacketHeader;
	requiredSize += sizeof(controlPacketHeader);
	LOGCommandHandlerDebug("requiredSize = %u", requiredSize);
	if (size < requiredSize) {
		LOGd("too small for control header size=%u required=%u", size, requiredSize);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}
	memcpy(&controlPacketHeader, &(buffer[bufIndex]), sizeof(controlPacketHeader));
	bufIndex += sizeof(controlPacketHeader);

	// Control command payload
	requiredSize += controlPacketHeader.payloadSize;
	LOGCommandHandlerDebug("requiredSize = %u", requiredSize);
	if (size < requiredSize) {
		LOGd("too small for control payload size=%u required=%u", size, requiredSize);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
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
		result.returnCode = ERR_NOT_AVAILABLE;
		return;
	}
	if (!EncryptionHandler::getInstance().allowAccess(getRequiredAccessLevel(controlCmdType), accessLevel)) {
		LOGw("No access for command %u", controlCmdType);
		result.returnCode = ERR_NO_ACCESS;
		return;
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
		handleCommand(protocol, meshCtrlCmd.controlCommand.type, meshCommandCtrlCmdData, source, accessLevel, result);

		// Send out result to UART.
		uart_msg_mesh_result_packet_header_t resultHeader;
		resultHeader.stoneId = ownId;
		resultHeader.resultHeader.commandType = meshCtrlCmd.controlCommand.type;
		resultHeader.resultHeader.returnCode = result.returnCode;
		resultHeader.resultHeader.payloadSize = result.dataSize;
		LOGi("Result: id=%u cmdType=%u retCode=%u data:", resultHeader.stoneId, resultHeader.resultHeader.commandType, resultHeader.resultHeader.returnCode);
		BLEutil::printArray(result.buf.data, result.dataSize);

		UartProtocol::getInstance().writeMsgStart(UART_OPCODE_TX_MESH_RESULT, sizeof(resultHeader) + result.dataSize);
		UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_MESH_RESULT, (uint8_t*)&resultHeader, sizeof(resultHeader));
		UartProtocol::getInstance().writeMsgPart(UART_OPCODE_TX_MESH_RESULT, result.buf.data, result.dataSize);
		UartProtocol::getInstance().writeMsgEnd(UART_OPCODE_TX_MESH_RESULT);

		if (!forOthers) {
			return;
		}
	}
	if (!forOthers) {
		result.returnCode = ERR_NOT_FOUND;
		return;
	}

	// Check nested permissions
	EncryptionAccessLevel requiredAccessLevel = ENCRYPTION_DISABLED;
	switch (controlCmdType) {
		case CTRL_CMD_STATE_SET:
		case CTRL_CMD_STATE_GET: {
			if (meshCtrlCmd.controlCommand.size < sizeof(state_packet_header_t)) {
				LOGd("too small for state packet header");
				result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
				return;
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
	if (!EncryptionHandler::getInstance().allowAccess(requiredAccessLevel, accessLevel)) {
		LOGw("No access for command payload. Required=%u", requiredAccessLevel);
		result.returnCode = ERR_NO_ACCESS;
		return;
	}

	// All permission checks must have been done already!
	// Also the nested ones!
	cs_data_t eventData((buffer_ptr_t)&meshCtrlCmd, sizeof(meshCtrlCmd));
	dispatchEventForCommand(CS_TYPE::CMD_SEND_MESH_CONTROL_COMMAND, eventData, result);
}

void CommandHandler::handleCmdAllowDimming(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "allow dimming");

	if (commandData.len != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) commandData.data;
	TYPIFY(CONFIG_PWM_ALLOWED) allow = payload->enable;

	event_t evt(CS_TYPE::CMD_DIMMING_ALLOWED, &allow, sizeof(allow));
	EventDispatcher::getInstance().dispatch(evt);
	
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdLockSwitch(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "lock switch");

	if (commandData.len != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) commandData.data;
	TYPIFY(CMD_SWITCHING_ALLOWED) allow = !payload->enable;

	event_t evt(CS_TYPE::CMD_SWITCHING_ALLOWED, &allow, sizeof(allow));
	EventDispatcher::getInstance().dispatch(evt);

	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdUartMsg(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGd(STR_HANDLE_COMMAND, "UART msg");

	if (!commandData.len) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_BLE_MSG, commandData.data, commandData.len);
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdRegisterTrackedDevice(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "register tracked device");
	if (commandData.len != sizeof(register_tracked_device_packet_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	TYPIFY(CMD_REGISTER_TRACKED_DEVICE) evtData;
	evtData.data = *((register_tracked_device_packet_t*)commandData.data);
	evtData.accessLevel = accessLevel;
	event_t event(CS_TYPE::CMD_REGISTER_TRACKED_DEVICE, &evtData, sizeof(evtData), result);
	event.dispatch();
	result.returnCode = event.result.returnCode;
	result.dataSize = event.result.dataSize;
	return;
}

void CommandHandler::dispatchEventForCommand(CS_TYPE type, cs_data_t commandData, cs_result_t & result) {
	event_t event(type, commandData.data, commandData.len, result);
	event.dispatch();
	result.returnCode = event.result.returnCode;
	result.dataSize = event.result.dataSize;
}

EncryptionAccessLevel CommandHandler::getRequiredAccessLevel(const CommandHandlerTypes type) {
	switch (type) {
		case CTRL_CMD_GET_BOOTLOADER_VERSION:
		case CTRL_CMD_GET_UICR_DATA:
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
		case CTRL_CMD_SET_IBEACON_CONFIG_ID:
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
		case CTRL_CMD_SET_IBEACON_CONFIG_ID:
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

			// Allocate buffer instead of using event.result.buf, as that's often not set or too small.
			// TODO: let non-get commands just return error code when buffer is too small.
			uint8_t result_buffer[300];

			cs_result_t result(cs_data_t(result_buffer, sizeof(result_buffer)));

			handleCommand(
				cmd->protocolVersion,
				cmd->type,
				cs_data_t(cmd->data, cmd->size),
				cmd->source,
				cmd->accessLevel,
				result
//				event.result
			);
			event.result.returnCode = result.returnCode;
//			event.result.dataSize = result.data.len;
			event.result.dataSize = 0; // Can't be set, until we're actually using the event result buffer.

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
