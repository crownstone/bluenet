/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 18, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_Boards.h>
#include <cfg/cs_Strings.h>
#include <drivers/cs_Serial.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_Scheduler.h>
#include <processing/cs_FactoryReset.h>
#include <processing/cs_Setup.h>
#include <processing/cs_Switch.h>
#include <processing/cs_TemperatureGuard.h>
#include <protocol/cs_UartProtocol.h>
#include <storage/cs_State.h>

#if BUILD_MESHING == 1
#include <ble/cs_NordicMesh.h>
#include <mesh/cs_MeshControl.h>
#include <mesh/cs_Mesh.h>
#endif

void reset(void* p_context) {

	uint32_t cmd = *(int32_t*) p_context;

	if (cmd == GPREGRET_DFU_RESET) {
		LOGi(MSG_FIRMWARE_UPDATE);
	} else {
		LOGi(MSG_RESET);
	}

	LOGi("Executing reset: %d", cmd);
	//! copy to make sure this is nothing more than one value
	uint8_t err_code;
	uint32_t gpregret_id = 0;
	uint32_t gpregret_msk = 0xFF;
	err_code = sd_power_gpregret_clr(gpregret_id, gpregret_msk);
	APP_ERROR_CHECK(err_code);
	err_code = sd_power_gpregret_set(gpregret_id, cmd);
	APP_ERROR_CHECK(err_code);
	sd_nvic_SystemReset();
}

void execute_delayed(void * p_context) {
	delayed_command_t* buf = (delayed_command_t*)p_context;
	CommandHandler::getInstance().handleCommand(buf->type, buf->buffer, buf->size);
	free(buf->buffer);
	free(buf);
}

CommandHandler::CommandHandler() :
		_delayTimerId(NULL),
		_resetTimerId(NULL),
		_boardConfig(NULL)
{
		_delayTimerData = { {0} };
		_delayTimerId = &_delayTimerData;
		_resetTimerData = { {0} };
		_resetTimerId = &_resetTimerData;
}

void CommandHandler::init(const boards_config_t* board) {
	_boardConfig = board;
	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_delayTimerId, execute_delayed);
	Timer::getInstance().createSingleShot(_resetTimerId, (app_timer_timeout_handler_t) reset);
}

void CommandHandler::resetDelayed(uint8_t opCode, uint16_t delayMs) {
	LOGi("Reset in %u ms, code=%u", delayMs, opCode);
	static uint8_t resetOpCode = opCode;
	Timer::getInstance().start(_resetTimerId, MS_TO_TICKS(delayMs), &resetOpCode);
//	//! Loop until reset trigger
//	while(true) {}; //! TODO: this doesn't seem to work
}

cs_ret_code_t CommandHandler::handleCommandDelayed(const CommandHandlerTypes type, buffer_ptr_t buffer, const uint16_t size,
		const uint32_t delay) {
	delayed_command_t* buf = new delayed_command_t();
	buf->type = type;
	buf->buffer = new uint8_t[size];
	memcpy(buf->buffer, buffer, size);
	buf->size = size;
	Timer::getInstance().start(_delayTimerId, MS_TO_TICKS(delay), buf);
	LOGi("execute with delay %d", delay);
	return NRF_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCommand(const CommandHandlerTypes type) {
	return handleCommand(type, NULL, 0);
}

cs_ret_code_t CommandHandler::handleCommand(const CommandHandlerTypes type, buffer_ptr_t buffer, const uint16_t size,
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
		return handleCmdMultiSwitch(buffer, size, accessLevel);
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
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "goto dfu");
	resetDelayed(GPREGRET_DFU_RESET);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdReset(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "reset");
	resetDelayed(GPREGRET_SOFT_RESET);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdEnableMesh(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "enable mesh");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool enable = payload->enable;

	LOGi("%s mesh", enable ? STR_ENABLE : STR_DISABLE);
	State::getInstance().set(CS_TYPE::CONFIG_MESH_ENABLED, (void*)&enable, sizeof(enable), PersistenceMode::STRATEGY1);

#if BUILD_MESHING == 1
	if (enable) {
		Mesh::getInstance().start();
	} else {
		Mesh::getInstance().stop();
	}
#endif

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdEnableEncryption(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "enable encryption, tbd");

	// TODO: make it only apply after reset.
	return ERR_NOT_IMPLEMENTED;

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool enable = payload->enable;

	LOGi("%s encryption", enable ? STR_ENABLE : STR_DISABLE);
	State::getInstance().set(CS_TYPE::CONFIG_ENCRYPTION_ENABLED, (void*)&enable, sizeof(enable), PersistenceMode::STRATEGY1);

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdEnableIbeacon(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "enable ibeacon");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool enable = payload->enable;

	LOGi("%s ibeacon", enable ? STR_ENABLE : STR_DISABLE);
	State::getInstance().set(CS_TYPE::CONFIG_IBEACON_ENABLED, (void*)&enable, sizeof(enable), PersistenceMode::STRATEGY1);

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdEnableScanner(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "enable scanner");

	// TODO: make it only apply after reset?

	enable_scanner_message_payload_t* payload = (enable_scanner_message_payload_t*) buffer;
	bool enable = payload->enable;
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
	State::getInstance().set(CS_TYPE::CONFIG_SCANNER_ENABLED, (void*)&enable, sizeof(enable), PersistenceMode::STRATEGY1);

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdRequestServiceData(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "request service data");

	return ERR_NOT_IMPLEMENTED;

	// TODO: use ServiceData function for this?
//#if BUILD_MESHING == 1
//		state_item_t stateItem;
//		memset(&stateItem, 0, sizeof(stateItem));
//
//		State& state = State::getInstance();
//		State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &stateItem.id);
//
//		state.get(STATE_SWITCH_STATE, stateItem.switchState);
//
//		// TODO: implement setting the eventBitmask in a better way
//		// Maybe get it from service data directly? Or should we store the eventBitmask in the State?
//		state_errors_t state_errors;
//		state.get(STATE_ERRORS, &state_errors, sizeof(state_errors_t));
//		stateItem.eventBitmask = 0;
//		if (state_errors.asInt != 0) {
//			stateItem.eventBitmask |= 1 << SERVICE_BITMASK_ERROR;
//		}
//
//		state.get(STATE_POWER_USAGE, (int32_t&)stateItem.powerUsage);
//		state.get(STATE_ACCUMULATED_ENERGY, (int32_t&)stateItem.accumulatedEnergy);
//
//		MeshControl::getInstance().sendServiceDataMessage(stateItem, true);
//#endif

	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdFactoryReset(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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
//	if (!EncryptionHandler::getInstance().allowAccess(SETUP, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
//	uint8_t opMode;
//	State::getInstance().get(CS_TYPE::STATE_OPERATION_MODE, opMode);
//	if (opMode != OPERATION_MODE_SETUP) {
//		LOGw("only available in setup mode");
//		return ERR_NOT_AVAILABLE;
//	}
	Stack::getInstance().changeToNormalTxPowerMode();
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdSetup(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "setup");
	cs_ret_code_t errCode = Setup::getInstance().handleCommand(buffer, size);
	return errCode;
}

cs_ret_code_t CommandHandler::handleCmdKeepAlive(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "keep alive");

	event_t event(CS_TYPE::EVT_KEEP_ALIVE);
	EventDispatcher::getInstance().dispatch(event);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdKeepAliveState(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "keep alive state");

	if (size != sizeof(keep_alive_state_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	event_t event(CS_TYPE::EVT_KEEP_ALIVE, buffer, size);
	EventDispatcher::getInstance().dispatch(event);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdKeepAliveRepeatLast(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "keep alive repeat");
#if BUILD_MESHING == 1
	MeshControl::getInstance().sendLastKeepAliveMessage();
#endif
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdKeepAliveMesh(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "keep alive mesh");
#if BUILD_MESHING == 1
	keep_alive_message_t* keepAliveMsg = (keep_alive_message_t*) buffer;
	// This function also checks the validity of the msg.
	return MeshControl::getInstance().sendKeepAliveMessage(keepAliveMsg, size);
#endif
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdUserFeedBack(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "user feedback");
	return ERR_NOT_IMPLEMENTED;
}

cs_ret_code_t CommandHandler::handleCmdDisconnect(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "disconnect");
	Stack::getInstance().disconnect();
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdSetLed(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "reset errors");
	if (size != sizeof(state_errors_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	state_errors_t* payload = (state_errors_t*) buffer;
	state_errors_t stateErrors;
	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors.asInt, PersistenceMode::STRATEGY1);
	LOGd("old errors %u - reset %u", stateErrors.asInt, payload->asInt);
	stateErrors.asInt &= ~(payload->asInt);
	LOGd("new errors %u", stateErrors.asInt);
	State::getInstance().set(CS_TYPE::STATE_ERRORS, &stateErrors.asInt, sizeof(stateErrors), PersistenceMode::STRATEGY1);
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdPwm(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		return ERR_NOT_AVAILABLE;
	}

//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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

//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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

//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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

cs_ret_code_t CommandHandler::handleCmdMultiSwitch(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "multi switch");
#if BUILD_MESHING == 1
	multi_switch_message_t* multiSwitchMsg = (multi_switch_message_t*) buffer;
	// This function also checks the validity of the msg.
	return MeshControl::getInstance().sendMultiSwitchMessage(multiSwitchMsg, size);
#endif
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdMeshCommand(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "mesh command");
#if BUILD_MESHING == 1
	command_message_t* commandMsg = (command_message_t*) buffer;
	if (!is_valid_command_message(commandMsg, size)) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	uint8_t* payload;
	uint16_t payloadLength;
	get_command_msg_payload(commandMsg, size, &payload, payloadLength);
	bool sendMeshMsg = false;
	EncryptionAccessLevel requiredAccessLevel = NOT_SET;
	switch (commandMsg->messageType) {
	case CONTROL_MESSAGE: {
		control_mesh_message_t* controlMsg = (control_mesh_message_t*)payload;
		if (!is_valid_command_control_mesh_message(controlMsg, payloadLength)) {
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		if (allowedAsMeshCommand((CommandHandlerTypes)controlMsg->header.type)) {
			sendMeshMsg = true;
			requiredAccessLevel = getRequiredAccessLevel((CommandHandlerTypes)controlMsg->header.type);
		}
		break;
	}
	}
	if (sendMeshMsg && EncryptionHandler::getInstance().allowAccess(requiredAccessLevel, accessLevel)) {
		// Send the message into the mesh.
		MeshControl::getInstance().sendCommandMessage(commandMsg, size);
	}
#endif
	return ERR_SUCCESS;
}

cs_ret_code_t CommandHandler::handleCmdAllowDimming(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "allow dimming");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool enable = payload->enable;

	LOGi("allow dimming: %u", enable);

	if (enable && State::getInstance().isSet(CS_TYPE::CONFIG_SWITCH_LOCKED)) {
		LOGw("unlock switch");
		bool lockEnable = false;
		State::getInstance().set(CS_TYPE::CONFIG_SWITCH_LOCKED, &lockEnable, sizeof(lockEnable), PersistenceMode::STRATEGY1);
		event_t event(CS_TYPE::EVT_SWITCH_LOCKED, &lockEnable, sizeof(bool));
		EventDispatcher::getInstance().dispatch(event);
	}

	State::getInstance().set(CS_TYPE::CONFIG_PWM_ALLOWED, &enable, sizeof(enable), PersistenceMode::STRATEGY1);
	event_t event(CS_TYPE::EVT_DIMMING_ALLOWED, &enable, sizeof(bool));
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
	bool enable = payload->enable;

	LOGi("lock switch: %u", enable);

	if (enable && State::getInstance().isSet(CS_TYPE::CONFIG_PWM_ALLOWED)) {
		LOGw("can't lock switch");
		return ERR_NOT_AVAILABLE;
	}

	State::getInstance().set(CS_TYPE::CONFIG_SWITCH_LOCKED, &enable, sizeof(enable), PersistenceMode::STRATEGY1);
	event_t event(CS_TYPE::EVT_SWITCH_LOCKED, &enable, sizeof(bool));
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
	bool enable = payload->enable;

	State::getInstance().set(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED, &enable, sizeof(enable), PersistenceMode::STRATEGY1);
	event_t event(CS_TYPE::EVT_SWITCHCRAFT_ENABLED, &enable, sizeof(bool));
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
	uint8_t enable = *(uint8_t*) buffer;
	cs_ret_code_t errCode = State::getInstance().set(CS_TYPE::CONFIG_UART_ENABLED, buffer, size, PersistenceMode::STRATEGY1);
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
		return GUEST; // These commands are only available in setup mode.

	case CTRL_CMD_SWITCH:
	case CTRL_CMD_PWM:
	case CTRL_CMD_RELAY:
	case CTRL_CMD_KEEP_ALIVE:
	case CTRL_CMD_DISCONNECT:
	case CTRL_CMD_NOP:
	case CTRL_CMD_KEEP_ALIVE_REPEAT_LAST:
	case CTRL_CMD_MULTI_SWITCH:
	case CTRL_CMD_MESH_COMMAND:
		return GUEST;

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
			stream_buffer_header_t* streamHeader = (stream_buffer_header_t*)event.data;
			if ((event.size < sizeof(stream_buffer_header_t)) || (event.size - sizeof(stream_buffer_header_t) < streamHeader->length)) {
				LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
				break;
			}
			uint8_t* streamPayload = (uint8_t*)event.data + sizeof(stream_buffer_header_t);
			handleCommand((CommandHandlerTypes)streamHeader->type, streamPayload, streamHeader->length);
			break;
		}
		default: {}
	}
}
