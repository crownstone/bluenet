/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 18, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "cfg/cs_Boards.h"
#include "cfg/cs_Strings.h"
#include "drivers/cs_Serial.h"
#include "processing/cs_CommandHandler.h"
#include "processing/cs_Scanner.h"
#include "processing/cs_Scheduler.h"
#include "processing/cs_FactoryReset.h"
#include "processing/cs_Setup.h"
#include "processing/cs_Switch.h"
#include "processing/cs_TemperatureGuard.h"
#include "protocol/cs_UartProtocol.h"
#include "storage/cs_State.h"
#include "protocol/mesh/cs_MeshModelPacketHelper.h"

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


cs_ret_code_t CommandHandler::handleCommand(const CommandHandlerTypes type, const cmd_source_t source) {
	return handleCommand(type, NULL, 0, source);
}

cs_ret_code_t CommandHandler::handleCommand(
		const CommandHandlerTypes type,
		buffer_ptr_t buffer,
		const uint16_t size,
		const cmd_source_t source,
		const EncryptionAccessLevel accessLevel) {
	LOGd("cmd=%u lvl=%u", type, accessLevel);
	if (!EncryptionHandler::getInstance().allowAccess(getRequiredAccessLevel(type), accessLevel)) {
		return ERR_NO_ACCESS;
	}
	switch (type) {
	case CTRL_CMD_NOP:
		return handleCmdNop(buffer, size, accessLevel);
	case CTRL_CMD_GOTO_DFU:
		return handleCmdGotoDfu(buffer, size, accessLevel);
	case CTRL_CMD_RESET:
		return handleCmdReset(buffer, size, accessLevel);
	case CTRL_CMD_ENABLE_MESH:
		return handleCmdEnableMesh(buffer, size, accessLevel);
	case CTRL_CMD_ENABLE_ENCRYPTION:
		return handleCmdEnableEncryption(buffer, size, accessLevel);
	case CTRL_CMD_ENABLE_IBEACON:
		return handleCmdEnableIbeacon(buffer, size, accessLevel);
	case CTRL_CMD_ENABLE_SCANNER:
		return handleCmdEnableScanner(buffer, size, accessLevel);
	case CTRL_CMD_REQUEST_SERVICE_DATA:
		return handleCmdRequestServiceData(buffer, size, accessLevel);
	case CTRL_CMD_FACTORY_RESET:
		return handleCmdFactoryReset(buffer, size, accessLevel);
	case CTRL_CMD_SET_TIME:
		return handleCmdSetTime(buffer, size, accessLevel);
	case CTRL_CMD_SCHEDULE_ENTRY_SET:
		return handleCmdScheduleEntrySet(buffer, size, accessLevel);
	case CTRL_CMD_SCHEDULE_ENTRY_CLEAR:
		return handleCmdScheduleEntryClear(buffer, size, accessLevel);
	case CTRL_CMD_INCREASE_TX:
		return handleCmdIncreaseTx(buffer, size, accessLevel);
	case CTRL_CMD_KEEP_ALIVE:
		return handleCmdKeepAlive(buffer, size, accessLevel);
	case CTRL_CMD_KEEP_ALIVE_STATE:
		return handleCmdKeepAliveState(buffer, size, accessLevel);
	case CTRL_CMD_KEEP_ALIVE_REPEAT_LAST:
		return handleCmdKeepAliveRepeatLast(buffer, size, accessLevel);
	case CTRL_CMD_KEEP_ALIVE_MESH:
		return handleCmdKeepAliveMesh(buffer, size, accessLevel);
	case CTRL_CMD_USER_FEEDBACK:
		return handleCmdUserFeedBack(buffer, size, accessLevel);
	case CTRL_CMD_DISCONNECT:
		return handleCmdDisconnect(buffer, size, accessLevel);
	case CTRL_CMD_SET_LED:
		return handleCmdSetLed(buffer, size, accessLevel);
	case CTRL_CMD_RESET_ERRORS:
		return handleCmdResetErrors(buffer, size, accessLevel);
	case CTRL_CMD_PWM:
		return handleCmdPwm(buffer, size, accessLevel);
	case CTRL_CMD_SWITCH:
		return handleCmdSwitch(buffer, size, accessLevel);
	case CTRL_CMD_RELAY:
		return handleCmdRelay(buffer, size, accessLevel);
	case CTRL_CMD_MULTI_SWITCH:
		return handleCmdMultiSwitch(buffer, size, source, accessLevel);
	case CTRL_CMD_MULTI_SWITCH_LEGACY:
		return handleCmdMultiSwitchLegacy(buffer, size, accessLevel);
	case CTRL_CMD_MESH_COMMAND:
		return handleCmdMeshCommand(buffer, size, accessLevel);
	case CTRL_CMD_ALLOW_DIMMING:
		return handleCmdAllowDimming(buffer, size, accessLevel);
	case CTRL_CMD_LOCK_SWITCH:
		return handleCmdLockSwitch(buffer, size, accessLevel);
	case CTRL_CMD_SETUP:
		return handleCmdSetup(buffer, size, accessLevel);
	case CTRL_CMD_ENABLE_SWITCHCRAFT:
		return handleCmdEnableSwitchcraft(buffer, size, accessLevel);
	case CTRL_CMD_UART_MSG:
		return handleCmdUartMsg(buffer, size, accessLevel);
	case CTRL_CMD_UART_ENABLE:
		return handleCmdUartEnable(buffer, size, accessLevel);
	default:
		LOGe("Unknown type: %u", type);
		return ERR_UNKNOWN_TYPE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdNop(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	// A no operation command to keep the connection alive.
	// No need to do anything here, the connection keep alive is handled in the stack.
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdGotoDfu(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "goto dfu");
	resetDelayed(GPREGRET_DFU_RESET);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdReset(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "reset");
	resetDelayed(GPREGRET_SOFT_RESET);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdEnableMesh(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "enable mesh");
	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	LOGi("%s mesh", payload->enable ? STR_ENABLE : STR_DISABLE);
	TYPIFY(CONFIG_MESH_ENABLED) enable = payload->enable;
	State::getInstance().set(CS_TYPE::CONFIG_MESH_ENABLED, &enable, sizeof(enable));
	TYPIFY(CMD_ENABLE_MESH) cmdEnable = payload->enable;
	event_t event(CS_TYPE::CMD_ENABLE_MESH, &cmdEnable, sizeof(cmdEnable));
	EventDispatcher::getInstance().dispatch(event);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdEnableEncryption(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "enable encryption, tbd");

	// TODO: make it only apply after reset.
	return ERR_NOT_IMPLEMENTED;

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	TYPIFY(CONFIG_ENCRYPTION_ENABLED) enable = payload->enable;

	LOGi("%s encryption", enable ? STR_ENABLE : STR_DISABLE);
	State::getInstance().set(CS_TYPE::CONFIG_ENCRYPTION_ENABLED, &enable, sizeof(enable));

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdEnableIbeacon(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "enable ibeacon");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	TYPIFY(CONFIG_IBEACON_ENABLED) enable = payload->enable;

	LOGi("%s ibeacon", enable ? STR_ENABLE : STR_DISABLE);
	State::getInstance().set(CS_TYPE::CONFIG_IBEACON_ENABLED, &enable, sizeof(enable));

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdEnableScanner(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "enable scanner");

	// TODO: make it only apply after reset?

	enable_scanner_message_payload_t* payload = (enable_scanner_message_payload_t*) buffer;
	TYPIFY(CONFIG_SCANNER_ENABLED) enable = payload->enable;
	uint16_t delay = payload->delay;

	if (size != sizeof(enable_scanner_message_payload_t)) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	LOGi("%s scanner", enable ? STR_ENABLE : STR_DISABLE);
	if (enable) {
		LOGi("delay: %d ms", delay);
		if (delay) {
			Scanner::getInstance().delayedStart(delay);
		}
		else {
			Scanner::getInstance().start();
		}
	}
	else {
		Scanner::getInstance().stop();
	}

	// TODO: first update flag, then start scanner? The scanner is stopped to write to pstorage anyway.
	State::getInstance().set(CS_TYPE::CONFIG_SCANNER_ENABLED, &enable, sizeof(enable));

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdRequestServiceData(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "request service data");

	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t CommandHandler::handleCmdFactoryReset(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "factory reset");

	if (size != sizeof(FACTORY_RESET_CODE)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, sizeof(FACTORY_RESET_CODE));
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	factory_reset_message_payload_t* payload = (factory_reset_message_payload_t*) buffer;
	uint32_t resetCode = payload->resetCode;

	if (!FactoryReset::getInstance().factoryReset(resetCode)) {
		return ERR_WRONG_PARAMETER;
	}

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdSetTime(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "set time:");

	if (size != sizeof(uint32_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	uint32_t value = *(uint32_t*)buffer;

	Scheduler::getInstance().setTime(value);

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdScheduleEntrySet(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "schedule entry");
	if (size < sizeof(schedule_command_t)) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	schedule_command_t* entry = (schedule_command_t*)buffer;
	cs_ret_code_t errCode = Scheduler::getInstance().setScheduleEntry(entry->id, &(entry->entry));
	if (FAILURE(errCode)) {
		return errCode;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdScheduleEntryClear(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	if (size < 1) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	uint8_t id = buffer[0];
	cs_ret_code_t errCode = Scheduler::getInstance().clearScheduleEntry(id);
	if (FAILURE(errCode)) {
		return errCode;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdIncreaseTx(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "increase TX");
	Stack::getInstance().changeToNormalTxPowerMode();
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdSetup(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "setup");
	cs_ret_code_t errCode = Setup::getInstance().handleCommand(buffer, size);
	return errCode;
}

cs_ret_code_t CommandHandler::handleCmdKeepAlive(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "keep alive");

	event_t event(CS_TYPE::EVT_KEEP_ALIVE);
	EventDispatcher::getInstance().dispatch(event);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdKeepAliveState(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "keep alive state");

	if (size != sizeof(keep_alive_state_item_cmd_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	event_t event(CS_TYPE::EVT_KEEP_ALIVE_STATE, buffer, size);
	EventDispatcher::getInstance().dispatch(event);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdKeepAliveRepeatLast(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "mesh keep alive repeat");
//#if BUILD_MESHING == 1
	cs_mesh_msg_t meshMsg;
	meshMsg.type = CS_MESH_MODEL_TYPE_CMD_KEEP_ALIVE;
	meshMsg.payload = NULL;
	meshMsg.size = 0;
	meshMsg.reliability = CS_MESH_RELIABILITY_LOW;
	meshMsg.urgency = CS_MESH_URGENCY_LOW;
	event_t cmd(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	EventDispatcher::getInstance().dispatch(cmd);
//#endif
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdKeepAliveMesh(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "mesh keep alive");
//#if BUILD_MESHING == 1
	cs_mesh_model_msg_keep_alive_t* keepAlivePacket = (cs_mesh_model_msg_keep_alive_t*)buffer;
	if (!MeshModelPacketHelper::keepAliveStateIsValid(keepAlivePacket, size)) {
		return ERR_INVALID_MESSAGE;
	}
	for (int i=0; i<keepAlivePacket->count; ++i) {
		TYPIFY(CMD_SEND_MESH_MSG_KEEP_ALIVE) item;
		item.id = keepAlivePacket->items[i].id;
		item.cmd.timeout = keepAlivePacket->timeout;
		if (keepAlivePacket->items[i].actionSwitchCmd == 255) {
			item.cmd.action = NO_CHANGE;
			item.cmd.switchCmd = 0;
		}
		else {
			item.cmd.action = CHANGE;
			item.cmd.switchCmd = keepAlivePacket->items[i].actionSwitchCmd;
		}
		if (cs_keep_alive_state_item_is_valid(&item, sizeof(item))) {
			event_t cmd(CS_TYPE::CMD_SEND_MESH_MSG_KEEP_ALIVE, &item, sizeof(item));
			EventDispatcher::getInstance().dispatch(cmd);
		}
	}
//#endif
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdUserFeedBack(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "user feedback");
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t CommandHandler::handleCmdDisconnect(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "disconnect");
	Stack::getInstance().disconnect();
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdSetLed(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "set led");

	if (_boardConfig->flags.hasLed) {
		LOGe("No LEDs on this board!");
		return ERR_NOT_AVAILABLE;
	}

	if (size != sizeof(led_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	led_message_payload_t* payload = (led_message_payload_t*) buffer;
	uint8_t led = payload->led;
	bool enable = payload->enable;
	LOGi("set led %d %s", led, enable ? "ON" : "OFF");

	uint8_t ledPin = led == 1 ? _boardConfig->pinLedGreen : _boardConfig->pinLedRed;
	if (_boardConfig->flags.ledInverted) {
		enable = !enable;
	}
	if (enable) {
		nrf_gpio_pin_set(ledPin);
	}
	else {
		nrf_gpio_pin_clear(ledPin);
	}
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdResetErrors(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "reset errors");
	if (size != sizeof(state_errors_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	state_errors_t* payload = (state_errors_t*) buffer;
	TYPIFY(STATE_ERRORS) stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	LOGd("old errors %u - reset %u", stateErrors.asInt, payload->asInt);
	stateErrors.asInt &= ~(payload->asInt);
	LOGd("new errors %u", stateErrors.asInt);
	State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdPwm(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		return ERR_NOT_AVAILABLE;
	}

	LOGi(STR_HANDLE_COMMAND, "PWM");

	if (size != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) buffer;
	uint8_t value = payload->switchState;

	uint8_t current = Switch::getInstance().getPwm();
	if (value != current) {
		Switch::getInstance().setPwm(value);
	}
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdSwitch(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		return ERR_NOT_AVAILABLE;
	}

	LOGi(STR_HANDLE_COMMAND, "switch");

	if (size != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) buffer;
	Switch::getInstance().setSwitch(payload->switchState);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdRelay(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		return ERR_NOT_AVAILABLE;
	}

	LOGi(STR_HANDLE_COMMAND, "relay");

	if (size != sizeof(switch_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	switch_message_payload_t* payload = (switch_message_payload_t*) buffer;
	uint8_t value = payload->switchState;
	if (value == 0) {
		Switch::getInstance().relayOff();
	}
	else {
		Switch::getInstance().relayOn();
	}
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdMultiSwitchLegacy(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "legacy multi switch");
//#if BUILD_MESHING == 1
	cs_legacy_multi_switch_t* multiSwitchPacket = (cs_legacy_multi_switch_t*)buffer;
	if (!cs_legacy_multi_switch_is_valid(multiSwitchPacket, size)) {
		return ERR_INVALID_MESSAGE;
	}
	for (int i=0; i<multiSwitchPacket->count; ++i) {
		TYPIFY(CMD_MULTI_SWITCH) item;
		item.id = multiSwitchPacket->items[i].id;
		item.cmd.switchCmd = multiSwitchPacket->items[i].switchCmd;
		item.cmd.timeout = multiSwitchPacket->items[i].timeout;
		item.cmd.source.flagExternal = false;
		item.cmd.source.sourceId = CS_CMD_SOURCE_CONNECTION;
		if (cs_multi_switch_item_is_valid(&item, sizeof(item))) {
			event_t cmd(CS_TYPE::CMD_MULTI_SWITCH, &item, sizeof(item));
			EventDispatcher::getInstance().dispatch(cmd);
		}
	}
//#endif
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdMultiSwitch(buffer_ptr_t buffer, const uint16_t size, const cmd_source_t source, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "multi switch");
	multi_switch_t* multiSwitchPacket = (multi_switch_t*)buffer;
	if (!cs_multi_switch_packet_is_valid(multiSwitchPacket, size)) {
		LOGw("invalid message");
		return ERR_INVALID_MESSAGE;
	}
	for (int i=0; i<multiSwitchPacket->count; ++i) {
		TYPIFY(CMD_MULTI_SWITCH) item;
		item.id = multiSwitchPacket->items[i].id;
		item.cmd.switchCmd = multiSwitchPacket->items[i].switchCmd;
		item.cmd.timeout = 0;
		item.cmd.source = source;
		if (cs_multi_switch_item_is_valid(&item, sizeof(item))) {
			event_t cmd(CS_TYPE::CMD_MULTI_SWITCH, &item, sizeof(item));
			EventDispatcher::getInstance().dispatch(cmd);
		}
		else {
			LOGw("invalid item ind=%u id=%u", i, item.id);
		}
	}
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdMeshCommand(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "mesh command");
	BLEutil::printArray(buffer, size);
//#if BUILD_MESHING == 1
	// Only support control command NOOP and SET_TIME for now, with idCount of 0. These are the only ones used by the app.
	// Command packet: type, flags, count, {control packet: type, reserved, length, length, payload...}
	//                 0     1      2                       3     4          5      6       7
	//                 00    00     00                      15    01         00     00
	//                 00    00     00                      02    01         04     00      56 34 12 00
	if (size < 3) {
		return ERR_BUFFER_TOO_SMALL;
	}
	// Check command type, flags, id count.
	if (buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 0) {
		return ERR_NOT_IMPLEMENTED;
	}
	if (size < 3+4) {
		return ERR_BUFFER_TOO_SMALL;
	}
	uint16_t payloadSize = *((uint16_t*)&(buffer[5]));
	uint8_t* payload = &(buffer[7]);
	if (size < 3+4+payloadSize) {
		return ERR_BUFFER_TOO_SMALL;
	}
	// Check control type and payload size
	switch (buffer[3]) {
	case CTRL_CMD_NOP:{
		if (payloadSize != 0) {
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		break;
	}
	case CTRL_CMD_SET_TIME:{
		if (payloadSize != sizeof(uint32_t)) {
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		break;
	}
	default:
		return ERR_NOT_IMPLEMENTED;
	}
	// Check access
	EncryptionAccessLevel requiredAccessLevel = getRequiredAccessLevel((CommandHandlerTypes)buffer[3]);
	if (!EncryptionHandler::getInstance().allowAccess(requiredAccessLevel, accessLevel)) {
		return ERR_NO_ACCESS;
	}

	cs_mesh_msg_t meshMsg;
	switch (buffer[3]) {
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
		return ERR_NOT_IMPLEMENTED;
	}
	event_t cmd(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	EventDispatcher::getInstance().dispatch(cmd);
//#endif
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdAllowDimming(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "allow dimming");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	TYPIFY(CONFIG_PWM_ALLOWED) enable = payload->enable;

	LOGi("allow dimming: %u", enable);

	if (enable && State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED)) {
		LOGw("unlock switch");
		TYPIFY(CONFIG_SWITCH_LOCKED) lockEnable = false;
		State::getInstance().set(CS_TYPE::CONFIG_SWITCH_LOCKED, &lockEnable, sizeof(lockEnable));
		event_t event(CS_TYPE::EVT_SWITCH_LOCKED, &lockEnable, sizeof(lockEnable));
		EventDispatcher::getInstance().dispatch(event);
	}

	State::getInstance().set(CS_TYPE::CONFIG_PWM_ALLOWED, &enable, sizeof(enable));
	event_t event(CS_TYPE::EVT_DIMMING_ALLOWED, &enable, sizeof(enable));
	EventDispatcher::getInstance().dispatch(event);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdLockSwitch(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "lock switch");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	TYPIFY(CONFIG_SWITCH_LOCKED) enable = payload->enable;

	LOGi("lock switch: %u", enable);

	if (enable && State::getInstance().isTrue(CS_TYPE::CONFIG_PWM_ALLOWED)) {
		LOGw("can't lock switch");
		return ERR_NOT_AVAILABLE;
	}

	State::getInstance().set(CS_TYPE::CONFIG_SWITCH_LOCKED, &enable, sizeof(enable));
	event_t event(CS_TYPE::EVT_SWITCH_LOCKED, &enable, sizeof(enable));
	EventDispatcher::getInstance().dispatch(event);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdEnableSwitchcraft(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "enable switchcraft");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	TYPIFY(CONFIG_SWITCHCRAFT_ENABLED) enable = payload->enable;
	State::getInstance().set(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED, &enable, sizeof(enable));
	event_t event(CS_TYPE::EVT_SWITCHCRAFT_ENABLED, &enable, sizeof(enable));
	EventDispatcher::getInstance().dispatch(event);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdUartMsg(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGd(STR_HANDLE_COMMAND, "UART msg");

	if (!size) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_BLE_MSG, buffer, size);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdUartEnable(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGd(STR_HANDLE_COMMAND, "UART enable");

	if (size != 1) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	TYPIFY(CONFIG_UART_ENABLED) enable = *(uint8_t*) buffer;
	cs_ret_code_t errCode = State::getInstance().set(CS_TYPE::CONFIG_UART_ENABLED, &enable, sizeof(enable));
	if (errCode != ERR_SUCCESS) {
		return errCode;
	}
	serial_enable((serial_enable_t)enable);
	return ERR_SUCCESS;
}



EncryptionAccessLevel CommandHandler::getRequiredAccessLevel(const CommandHandlerTypes type) {
	switch (type) {
	case CTRL_CMD_INCREASE_TX:
	case CTRL_CMD_SETUP:
		return BASIC; // These commands are only available in setup mode.

	case CTRL_CMD_SWITCH:
	case CTRL_CMD_PWM:
	case CTRL_CMD_RELAY:
	case CTRL_CMD_KEEP_ALIVE:
	case CTRL_CMD_DISCONNECT:
	case CTRL_CMD_NOP:
	case CTRL_CMD_KEEP_ALIVE_REPEAT_LAST:
	case CTRL_CMD_MULTI_SWITCH:
	case CTRL_CMD_MULTI_SWITCH_LEGACY:
	case CTRL_CMD_MESH_COMMAND:
		return BASIC;

	case CTRL_CMD_SET_TIME:
	case CTRL_CMD_KEEP_ALIVE_STATE:
	case CTRL_CMD_SCHEDULE_ENTRY_SET:
	case CTRL_CMD_REQUEST_SERVICE_DATA:
	case CTRL_CMD_SCHEDULE_ENTRY_CLEAR:
	case CTRL_CMD_KEEP_ALIVE_MESH:
		return MEMBER;

	case CTRL_CMD_GOTO_DFU:
	case CTRL_CMD_RESET:
	case CTRL_CMD_FACTORY_RESET:
	case CTRL_CMD_ENABLE_MESH:
	case CTRL_CMD_ENABLE_ENCRYPTION:
	case CTRL_CMD_ENABLE_IBEACON:
	case CTRL_CMD_ENABLE_SCANNER:
	case CTRL_CMD_USER_FEEDBACK:
	case CTRL_CMD_SET_LED:
	case CTRL_CMD_RESET_ERRORS:
	case CTRL_CMD_ALLOW_DIMMING:
	case CTRL_CMD_LOCK_SWITCH:
	case CTRL_CMD_ENABLE_SWITCHCRAFT:
	case CTRL_CMD_UART_MSG:
	case CTRL_CMD_UART_ENABLE:
		return ADMIN;
	default:
		return NOT_SET;
	}
}

bool CommandHandler::allowedAsMeshCommand(const CommandHandlerTypes type) {
	switch (type) {
	case CTRL_CMD_SWITCH:
	case CTRL_CMD_PWM:
	case CTRL_CMD_RELAY:
	case CTRL_CMD_SET_TIME:
	case CTRL_CMD_RESET:
	case CTRL_CMD_FACTORY_RESET:
	case CTRL_CMD_KEEP_ALIVE_STATE:
	case CTRL_CMD_KEEP_ALIVE:
	case CTRL_CMD_ENABLE_SCANNER:
	case CTRL_CMD_USER_FEEDBACK:
	case CTRL_CMD_SCHEDULE_ENTRY_SET:
	case CTRL_CMD_REQUEST_SERVICE_DATA:
	case CTRL_CMD_SET_LED:
	case CTRL_CMD_RESET_ERRORS:
	case CTRL_CMD_SCHEDULE_ENTRY_CLEAR:
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
			if (event.size != TypeSize(CS_TYPE::CMD_RESET_DELAYED)) {
				LOGe(FMT_WRONG_PAYLOAD_LENGTH, event.size);
				return;
			}
			reset_delayed_t* payload = (reset_delayed_t*)event.data;
			resetDelayed(payload->resetCode, payload->delayMs);
			break;
		}
		case CS_TYPE::CMD_CONTROL_CMD: {
			TYPIFY(CMD_CONTROL_CMD)* cmd = (TYPIFY(CMD_CONTROL_CMD)*)event.data;
			handleCommand(cmd->type, cmd->data, cmd->size, cmd->source, cmd->accessLevel);
			break;
		}
		default: {}
	}
}
