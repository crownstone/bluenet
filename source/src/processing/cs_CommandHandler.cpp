/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 18, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Advertiser.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_DeviceTypes.h>
#include <cfg/cs_Strings.h>
#include <drivers/cs_GpRegRet.h>
#include <logging/cs_Logger.h>
#include <encryption/cs_KeysAndAccess.h>
#include <ipc/cs_IpcRamData.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_FactoryReset.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_Setup.h>
#include <processing/cs_TemperatureGuard.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>
#include <storage/cs_State.h>
#include <time/cs_SystemTime.h>
#include <uart/cs_UartHandler.h>
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
		_resetTimerId(nullptr),
		_boardConfig(nullptr)
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
//	while (true) {}; // This doesn't seem to work
}

void CommandHandler::handleCommand(
		uint8_t protocolVersion,
		const CommandHandlerTypes type,
		cs_data_t commandData,
		const cmd_source_with_counter_t source,
		const EncryptionAccessLevel accessLevel,
		cs_result_t & result
		) {
	if (protocolVersion != CS_CONNECTION_PROTOCOL_VERSION) {
		LOGw("Wrong protocol: %u", protocolVersion);
		result.returnCode = ERR_PROTOCOL_UNSUPPORTED;
		return;
	}

	// Prints incoming command types.
	// Some cases are avoided because the occur often (currently only SET_SUN_TIME).
	switch (type) {
		case CTRL_CMD_SET_SUN_TIME: break;
		default: LOGd("cmd=%u lvl=%u", type, accessLevel); break;
	}

	if (!KeysAndAccess::getInstance().allowAccess(getRequiredAccessLevel(type), accessLevel)) {
		LOGCommandHandlerDebug("command message skipped, access not allowed");
		result.returnCode = ERR_NO_ACCESS;
		return;
	}

	switch (type) {
		// cases handled by specific handlers
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
		case CTRL_CMD_GET_MAC_ADDRESS:
			return handleCmdGetMacAddress(commandData, accessLevel, result);
		case CTRL_CMD_GET_HARDWARE_VERSION:
			return handleCmdGetHardwareVersion(commandData, accessLevel, result);
		case CTRL_CMD_GET_FIRMWARE_VERSION:
			return handleCmdGetFirmwareVersion(commandData, accessLevel, result);
		case CTRL_CMD_SET_SUN_TIME:
			return handleCmdSetSunTime(commandData, accessLevel, result);
		case CTRL_CMD_GET_TIME:
			return handleCmdGetTime(commandData, accessLevel, result);
		case CTRL_CMD_INCREASE_TX:
			return handleCmdIncreaseTx(commandData, accessLevel, result);
		case CTRL_CMD_DISCONNECT:
			return handleCmdDisconnect(commandData, accessLevel, result);
		case CTRL_CMD_RESET_ERRORS:
			return handleCmdResetErrors(commandData, accessLevel, result);
		case CTRL_CMD_PWM:
			return handleCmdPwm(commandData, source, accessLevel, result);
		case CTRL_CMD_SWITCH:
			return handleCmdSwitch(commandData, source, accessLevel, result);
		case CTRL_CMD_RELAY:
			return handleCmdRelay(commandData, source, accessLevel, result);
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
		case CTRL_CMD_HUB_DATA:
			return handleCmdHubData(commandData, accessLevel, result);
		case CTRL_CMD_STATE_GET:
			return handleCmdStateGet(commandData, accessLevel, result);
		case CTRL_CMD_STATE_SET:
			return handleCmdStateSet(commandData, accessLevel, result);
		case CTRL_CMD_REGISTER_TRACKED_DEVICE:
			return handleCmdRegisterTrackedDevice(commandData, accessLevel, result);
		case CTRL_CMD_TRACKED_DEVICE_HEARTBEAT:
			return handleCmdTrackedDeviceHeartbeat(commandData, accessLevel, result);
		case CTRL_CMD_GET_UPTIME:
			return handleCmdGetUptime(commandData, accessLevel, result);
		case CTRL_CMD_MICROAPP_UPLOAD:
			return handleCmdMicroappUpload(commandData, accessLevel, result);
		// cases handled by dispatchEventForCommand:
		case CTRL_CMD_SET_TIME:
			return dispatchEventForCommand(CS_TYPE::CMD_SET_TIME, commandData, source, result);
		case CTRL_CMD_SAVE_BEHAVIOUR:
			return dispatchEventForCommand(CS_TYPE::CMD_ADD_BEHAVIOUR, commandData, source, result);
		case CTRL_CMD_REPLACE_BEHAVIOUR:
			return dispatchEventForCommand(CS_TYPE::CMD_REPLACE_BEHAVIOUR, commandData, source, result);
		case CTRL_CMD_REMOVE_BEHAVIOUR:
			return dispatchEventForCommand(CS_TYPE::CMD_REMOVE_BEHAVIOUR, commandData, source, result);
		case CTRL_CMD_GET_BEHAVIOUR:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_BEHAVIOUR, commandData, source, result);
		case CTRL_CMD_GET_BEHAVIOUR_INDICES:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_BEHAVIOUR_INDICES, commandData, source, result);
		case CTRL_CMD_GET_BEHAVIOUR_DEBUG:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_BEHAVIOUR_DEBUG, commandData, source, result);
		case CTRL_CMD_GET_PRESENCE:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_PRESENCE, commandData, source, result);
		case CTRL_CMD_SET_IBEACON_CONFIG_ID:
			return dispatchEventForCommand(CS_TYPE::CMD_SET_IBEACON_CONFIG_ID, commandData, source, result);
		case CTRL_CMD_GET_ADC_RESTARTS:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_ADC_RESTARTS, commandData, source, result);
		case CTRL_CMD_GET_SWITCH_HISTORY:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_SWITCH_HISTORY, commandData, source, result);
		case CTRL_CMD_GET_POWER_SAMPLES:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_POWER_SAMPLES, commandData, source, result);
		case CTLR_CMD_GET_SCHEDULER_MIN_FREE:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_SCHEDULER_MIN_FREE, commandData, source, result);
		case CTRL_CMD_GET_RESET_REASON:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_RESET_REASON, commandData, source, result);
		case CTRL_CMD_GET_GPREGRET:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_GPREGRET, commandData, source, result);
		case CTRL_CMD_GET_ADC_CHANNEL_SWAPS:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_ADC_CHANNEL_SWAPS, commandData, source, result);
		case CTRL_CMD_GET_RAM_STATS:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_RAM_STATS, commandData, source, result);
		case CTRL_CMD_MICROAPP_GET_INFO:
			return dispatchEventForCommand(CS_TYPE::CMD_MICROAPP_GET_INFO, commandData, source, result);
		case CTRL_CMD_MICROAPP_VALIDATE:
			return dispatchEventForCommand(CS_TYPE::CMD_MICROAPP_VALIDATE, commandData, source, result);
		case CTRL_CMD_MICROAPP_REMOVE:
			return dispatchEventForCommand(CS_TYPE::CMD_MICROAPP_REMOVE, commandData, source, result);
		case CTRL_CMD_MICROAPP_ENABLE:
			return dispatchEventForCommand(CS_TYPE::CMD_MICROAPP_ENABLE, commandData, source, result);
		case CTRL_CMD_MICROAPP_DISABLE:
			return dispatchEventForCommand(CS_TYPE::CMD_MICROAPP_DISABLE, commandData, source, result);
		case CTRL_CMD_CLEAN_FLASH:
			return dispatchEventForCommand(CS_TYPE::CMD_STORAGE_GARBAGE_COLLECT, commandData, source, result);
		case CTRL_CMD_FILTER_UPLOAD:
			return dispatchEventForCommand(CS_TYPE::CMD_UPLOAD_FILTER, commandData, source, result);
		case CTRL_CMD_FILTER_REMOVE:
			return dispatchEventForCommand(CS_TYPE::CMD_REMOVE_FILTER, commandData, source, result);
		case CTRL_CMD_FILTER_COMMIT:
			return dispatchEventForCommand(CS_TYPE::CMD_COMMIT_FILTER_CHANGES, commandData, source, result);
		case CTRL_CMD_FILTER_GET_SUMMARIES:
			return dispatchEventForCommand(CS_TYPE::CMD_GET_FILTER_SUMMARIES, commandData, source, result);
		case CTRL_CMD_RESET_MESH_TOPOLOGY:
			return dispatchEventForCommand(CS_TYPE::CMD_MESH_TOPO_RESET, commandData, source, result);

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

void CommandHandler::handleCmdGetHardwareVersion(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "get hardware version");

	// TODO: use UICR to determine hardware version, use struct instead of string.
	result.returnCode = ERR_NOT_IMPLEMENTED;
	return;
}

void CommandHandler::handleCmdGetFirmwareVersion(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "get firmware version");

//	// Let std string handle the null termination.
//	std::string firmwareVersion;
//	if (strcmp(g_BUILD_TYPE, "Release") == 0) {
//		firmwareVersion = g_FIRMWARE_VERSION;
//	}
//	else {
//		firmwareVersion = g_GIT_SHA1;
//	}
//	uint16_t dataSize = firmwareVersion.size();
//
//	LOGd("Firmware version: %s", firmwareVersion.c_str());
//	if (result.buf.len < dataSize) {
//		result.returnCode = ERR_BUFFER_TOO_SMALL;
//		return;
//	}
//
//	memcpy(result.buf.data, firmwareVersion.c_str(), dataSize);
//	result.dataSize = dataSize;
//	result.returnCode = ERR_SUCCESS;

	// TODO: use struct instead of string?
	result.returnCode = ERR_NOT_IMPLEMENTED;
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
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(FACTORY_RESET_CODE));
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

void CommandHandler::handleCmdGetMacAddress(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "get MAC");

	if (result.buf.len < MAC_ADDRESS_LEN) {
		result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}

	ble_gap_addr_t address;
	if (sd_ble_gap_addr_get(&address) != NRF_SUCCESS) {
		result.returnCode = ERR_NOT_FOUND;
		return;
	}
	memcpy(result.buf.data, address.addr, MAC_ADDRESS_LEN);
	result.dataSize = MAC_ADDRESS_LEN;
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdStateGet(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "state get");

	// Check if command data is large enough for header.
	if (commandData.len < sizeof(state_packet_header_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(state_packet_header_t));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	// Read out header.
	state_packet_header_t* stateHeader = (state_packet_header_t*) commandData.data;
	LOGi("State type=%u id=%u persistenceMode=%u", stateHeader->stateType, stateHeader->stateId, stateHeader->persistenceMode);
	CS_TYPE stateType = toCsType(stateHeader->stateType);
	cs_state_id_t stateId = stateHeader->stateId;

	// Check access.
	if (!KeysAndAccess::getInstance().allowAccess(getUserAccessLevelGet(stateType), accessLevel)) {
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
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(state_packet_header_t));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	// Read out header.
	state_packet_header_t* stateHeader = (state_packet_header_t*) commandData.data;
	LOGi("State type=%u id=%u persistenceMode=%u", stateHeader->stateType, stateHeader->stateId, stateHeader->persistenceMode);
	CS_TYPE stateType = toCsType(stateHeader->stateType);
	cs_state_id_t stateId = stateHeader->stateId;

	// Check access.
	if (!KeysAndAccess::getInstance().allowAccess(getUserAccessLevelSet(stateType), accessLevel)) {
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
			persistenceMode = PersistenceMode::STRATEGY1;
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

void CommandHandler::handleCmdSetSunTime(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGCommandHandlerDebug(STR_HANDLE_COMMAND, "set sun time");
	if (commandData.len != sizeof(sun_time_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(sun_time_t));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}
	sun_time_t* payload = reinterpret_cast<sun_time_t*>(commandData.data);
	result.returnCode = SystemTime::setSunTimes(*payload);
}

void CommandHandler::handleCmdGetTime(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "get time");

	if (result.buf.len < sizeof(uint32_t)) {
		result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}

	uint32_t* timestamp = (uint32_t*)result.buf.data;
	*timestamp = SystemTime::posix();
	result.dataSize = sizeof(*timestamp);
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
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(state_errors_t));
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

void CommandHandler::handleCmdPwm(cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		result.returnCode = ERR_NOT_AVAILABLE;
		return;
	}

	LOGi(STR_HANDLE_COMMAND, "PWM");

	if (commandData.len != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(switch_message_payload_t));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) commandData.data;
	
	TYPIFY(CMD_SET_DIMMER) switchCmd;
	switchCmd = payload->switchState;

	event_t evt(CS_TYPE::CMD_SET_DIMMER, &switchCmd, sizeof(switchCmd), source);
	EventDispatcher::getInstance().dispatch(evt);

	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdSwitch(cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		result.returnCode = ERR_NOT_AVAILABLE;
		return;
	}

	LOGi(STR_HANDLE_COMMAND, "switch");

	if (commandData.len != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(switch_message_payload_t));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) commandData.data;

	TYPIFY(CMD_SWITCH) switch_cmd;
	switch_cmd.switchCmd = payload->switchState;
	event_t evt(CS_TYPE::CMD_SWITCH, &switch_cmd, sizeof(switch_cmd), source);
	EventDispatcher::getInstance().dispatch(evt);

	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdRelay(cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		result.returnCode = ERR_NOT_AVAILABLE;
		return;
	}

	LOGi(STR_HANDLE_COMMAND, "relay");

	if (commandData.len != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(switch_message_payload_t));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) commandData.data;
	TYPIFY(CMD_SET_RELAY) relay_switch_state;
	relay_switch_state = payload->switchState != 0;
	event_t evt(CS_TYPE::CMD_SET_RELAY, &relay_switch_state, sizeof(relay_switch_state), source);
	EventDispatcher::getInstance().dispatch(evt);
	
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdMultiSwitch(cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
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

		if (cs_multi_switch_item_is_valid(&item, sizeof(item))) {
			event_t cmd(CS_TYPE::CMD_MULTI_SWITCH, &item, sizeof(item), source);
			EventDispatcher::getInstance().dispatch(cmd);
		}
		else {
			LOGw("invalid item ind=%u id=%u", i, item.id);
		}
	}
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdMeshCommand(uint8_t protocol, cs_data_t commandData, const cmd_source_with_counter_t source, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	uint16_t size = commandData.len;
	buffer_ptr_t buffer = commandData.data;
	_log(SERIAL_INFO, false, STR_HANDLE_COMMAND, "mesh command: ");
	_logArray(SERIAL_INFO, true, buffer, size);

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

	// Check permissions
	CommandHandlerTypes controlCmdType = meshCtrlCmd.controlCommand.type;
	if (!allowedAsMeshCommand(controlCmdType)) {
		LOGw("Command %u is not allowed via mesh", controlCmdType);
		result.returnCode = ERR_NOT_AVAILABLE;
		return;
	}
	if (!KeysAndAccess::getInstance().allowAccess(getRequiredAccessLevel(controlCmdType), accessLevel)) {
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
		_log(SERIAL_INFO, false, "Result: id=%u cmdType=%u retCode=%u data: ", resultHeader.stoneId, resultHeader.resultHeader.commandType, resultHeader.resultHeader.returnCode);
		_logArray(SERIAL_INFO, true, result.buf.data, result.dataSize);

		UartHandler::getInstance().writeMsgStart(UART_OPCODE_TX_MESH_RESULT, sizeof(resultHeader) + result.dataSize);
		UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_MESH_RESULT, (uint8_t*)&resultHeader, sizeof(resultHeader));
		UartHandler::getInstance().writeMsgPart(UART_OPCODE_TX_MESH_RESULT, result.buf.data, result.dataSize);
		UartHandler::getInstance().writeMsgEnd(UART_OPCODE_TX_MESH_RESULT);
//		LOGd("success id=%u", resultHeader.stoneId);

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
	if (!KeysAndAccess::getInstance().allowAccess(requiredAccessLevel, accessLevel)) {
		LOGw("No access for command payload. Required=%u", requiredAccessLevel);
		result.returnCode = ERR_NO_ACCESS;
		return;
	}

	// All permission checks must have been done already!
	// Also the nested ones!
	cs_data_t eventData((buffer_ptr_t)&meshCtrlCmd, sizeof(meshCtrlCmd));
	dispatchEventForCommand(CS_TYPE::CMD_SEND_MESH_CONTROL_COMMAND, eventData, source, result);
}

void CommandHandler::handleCmdAllowDimming(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "allow dimming");

	if (commandData.len != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(enable_message_payload_t));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) commandData.data;
	TYPIFY(CONFIG_DIMMING_ALLOWED) allow = payload->enable;

	event_t evt(CS_TYPE::CMD_DIMMING_ALLOWED, &allow, sizeof(allow));
	EventDispatcher::getInstance().dispatch(evt);
	
	result.returnCode = ERR_SUCCESS;
}

void CommandHandler::handleCmdLockSwitch(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "lock switch");

	if (commandData.len != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(enable_message_payload_t));
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
		LOGe(FMT_ZERO_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	result.returnCode = UartHandler::getInstance().writeMsg(UART_OPCODE_TX_BLE_MSG, commandData.data, commandData.len);
}


void CommandHandler::handleCmdHubData(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGd(STR_HANDLE_COMMAND, "hub data");

	if (commandData.len < sizeof(hub_data_header_t)) {
		LOGe(FMT_ZERO_PAYLOAD_LENGTH, commandData.len);
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	hub_data_header_t* header = reinterpret_cast<hub_data_header_t*>(commandData.data);
	buffer_ptr_t hubDataPtr =      commandData.data + sizeof(*header);
	cs_buffer_size_t hubDataSize = commandData.len  - sizeof(*header);
	UartProtocol::Encrypt encrypt = static_cast<UartProtocol::Encrypt>(header->encrypt);

	if (header->reserved != 0) {
		result.returnCode = ERR_WRONG_PARAMETER;
		return;
	}

	result.returnCode = UartHandler::getInstance().writeMsg(UART_OPCODE_TX_HUB_DATA, hubDataPtr, hubDataSize, encrypt);
	if (result.returnCode == ERR_SUCCESS) {
		result.returnCode = ERR_WAIT_FOR_SUCCESS;
	}
}


void CommandHandler::handleCmdRegisterTrackedDevice(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "register tracked device");
	if (commandData.len != sizeof(register_tracked_device_packet_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(register_tracked_device_packet_t));
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

void CommandHandler::handleCmdTrackedDeviceHeartbeat(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "tracked device heartbeat");
//	result.returnCode = ERR_NOT_IMPLEMENTED;
//	return;

	if (commandData.len != sizeof(tracked_device_heartbeat_packet_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(tracked_device_heartbeat_packet_t));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	TYPIFY(CMD_TRACKED_DEVICE_HEARTBEAT) evtData;
	evtData.data = *((tracked_device_heartbeat_packet_t*)commandData.data);
	evtData.accessLevel = accessLevel;
	event_t event(CS_TYPE::CMD_TRACKED_DEVICE_HEARTBEAT, &evtData, sizeof(evtData), result);
	event.dispatch();
	result.returnCode = event.result.returnCode;
	result.dataSize = event.result.dataSize;
	return;
}

void CommandHandler::handleCmdGetUptime(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "get uptime");
	if (result.buf.len < sizeof(uint32_t)) {
		result.returnCode = ERR_BUFFER_TOO_SMALL;
		return;
	}
	uint32_t* uptime = (uint32_t*) result.buf.data;
	*uptime = SystemTime::up();
	result.dataSize = sizeof(uint32_t);
	result.returnCode = ERR_SUCCESS;
	return;
}

void CommandHandler::handleCmdMicroappUpload(cs_data_t commandData, const EncryptionAccessLevel accessLevel, cs_result_t & result) {
	LOGi(STR_HANDLE_COMMAND, "microapp upload");
	if (commandData.len < sizeof(microapp_upload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, commandData.len, sizeof(microapp_upload_t));
		result.returnCode = ERR_WRONG_PAYLOAD_LENGTH;
		return;
	}

	TYPIFY(CMD_MICROAPP_UPLOAD) evtData;
	evtData.header = *reinterpret_cast<microapp_upload_t*>(commandData.data);
	evtData.data.len = commandData.len - sizeof(evtData.header);
	evtData.data.data = commandData.data + sizeof(evtData.header);
	event_t event(CS_TYPE::CMD_MICROAPP_UPLOAD, &evtData, sizeof(evtData), result);
	event.dispatch();
	result.returnCode = event.result.returnCode;
	result.dataSize = event.result.dataSize;
	return;
}

void CommandHandler::dispatchEventForCommand(CS_TYPE type, cs_data_t commandData, const cmd_source_with_counter_t& source, cs_result_t & result) {
	event_t event(type, commandData.data, commandData.len, source, result);
	event.dispatch();
	result.returnCode = event.result.returnCode;
	result.dataSize = event.result.dataSize;
}

EncryptionAccessLevel CommandHandler::getRequiredAccessLevel(const CommandHandlerTypes type) {
	switch (type) {
		case CTRL_CMD_GET_BOOTLOADER_VERSION:
		case CTRL_CMD_GET_UICR_DATA:
		case CTRL_CMD_GET_MAC_ADDRESS:
		case CTRL_CMD_GET_HARDWARE_VERSION:
		case CTRL_CMD_GET_FIRMWARE_VERSION:
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
		case CTRL_CMD_GET_TIME:
		case CTRL_CMD_REGISTER_TRACKED_DEVICE:
		case CTRL_CMD_TRACKED_DEVICE_HEARTBEAT:
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
		case CTRL_CMD_HUB_DATA:
		case CTRL_CMD_GET_PRESENCE:
		case CTRL_CMD_GET_BEHAVIOUR_DEBUG:
		case CTRL_CMD_SET_IBEACON_CONFIG_ID:
		case CTRL_CMD_GET_UPTIME:
		case CTRL_CMD_GET_ADC_RESTARTS:
		case CTRL_CMD_GET_SWITCH_HISTORY:
		case CTRL_CMD_GET_POWER_SAMPLES:
		case CTLR_CMD_GET_SCHEDULER_MIN_FREE:
		case CTRL_CMD_GET_RESET_REASON:
		case CTRL_CMD_GET_GPREGRET:
		case CTRL_CMD_GET_ADC_CHANNEL_SWAPS:
		case CTRL_CMD_GET_RAM_STATS:
		case CTRL_CMD_MICROAPP_GET_INFO:
		case CTRL_CMD_MICROAPP_UPLOAD:
		case CTRL_CMD_MICROAPP_VALIDATE:
		case CTRL_CMD_MICROAPP_REMOVE:
		case CTRL_CMD_MICROAPP_ENABLE:
		case CTRL_CMD_MICROAPP_DISABLE:
		case CTRL_CMD_CLEAN_FLASH:
		case CTRL_CMD_FILTER_UPLOAD:
		case CTRL_CMD_FILTER_REMOVE:
		case CTRL_CMD_FILTER_COMMIT:
		case CTRL_CMD_FILTER_GET_SUMMARIES:
		case CTRL_CMD_RESET_MESH_TOPOLOGY:
			return ADMIN;
		case CTRL_CMD_UNKNOWN:
			return NOT_SET;
	}
	return NOT_SET;
}

bool CommandHandler::allowedAsMeshCommand(const CommandHandlerTypes type) {
	switch (type) {
		case CTRL_CMD_FACTORY_RESET:
		case CTRL_CMD_RESET_ERRORS:
		case CTRL_CMD_RESET:
		case CTRL_CMD_SET_TIME:
		case CTRL_CMD_STATE_SET:
		case CTRL_CMD_UART_MSG:
		case CTRL_CMD_SET_IBEACON_CONFIG_ID:
		case CTRL_CMD_RESET_MESH_TOPOLOGY:
		case CTRL_CMD_LOCK_SWITCH:
		case CTRL_CMD_ALLOW_DIMMING:
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
			handleCommand(
				cmd->protocolVersion,
				cmd->type,
				cs_data_t(cmd->data, cmd->size),
				event.source,
				cmd->accessLevel,
				event.result
			);
			break;
		}
		default: {}
	}
}
