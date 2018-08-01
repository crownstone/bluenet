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
#include <storage/cs_Settings.h>
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
	err_code = sd_power_gpregret_clr(0xFF);
	APP_ERROR_CHECK(err_code);
	err_code = sd_power_gpregret_set(cmd);
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

ERR_CODE CommandHandler::handleCommandDelayed(const CommandHandlerTypes type, buffer_ptr_t buffer, const uint16_t size, 
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

ERR_CODE CommandHandler::handleCommand(const CommandHandlerTypes type) {
	return handleCommand(type, NULL, 0);
}


ERR_CODE CommandHandler::handleCommand(const CommandHandlerTypes type, buffer_ptr_t buffer, const uint16_t size, 
		const EncryptionAccessLevel accessLevel) {
	LOGd("cmd=%u lvl=%u", type, accessLevel);
	if (!EncryptionHandler::getInstance().allowAccess(getRequiredAccessLevel(type), accessLevel)) {
		return ERR_NO_ACCESS;
	}
	switch (type) {
	case CMD_NOP:
		return handleCmdNop(buffer, size, accessLevel);
	case CMD_GOTO_DFU:
		return handleCmdGotoDfu(buffer, size, accessLevel);
	case CMD_RESET:
		return handleCmdReset(buffer, size, accessLevel);
	case CMD_ENABLE_MESH:
		return handleCmdEnableMesh(buffer, size, accessLevel);
	case CMD_ENABLE_ENCRYPTION:
		return handleCmdEnableEncryption(buffer, size, accessLevel);
	case CMD_ENABLE_IBEACON:
		return handleCmdEnableIbeacon(buffer, size, accessLevel);
	case CMD_ENABLE_SCANNER:
		return handleCmdEnableScanner(buffer, size, accessLevel);
	case CMD_SCAN_DEVICES:
		return handleCmdScanDevices(buffer, size, accessLevel);
	case CMD_REQUEST_SERVICE_DATA:
		return handleCmdRequestServiceData(buffer, size, accessLevel);
	case CMD_FACTORY_RESET:
		return handleCmdFactoryReset(buffer, size, accessLevel);
	case CMD_SET_TIME:
		return handleCmdSetTime(buffer, size, accessLevel);
	case CMD_SCHEDULE_ENTRY_SET:
		return handleCmdScheduleEntrySet(buffer, size, accessLevel);
	case CMD_SCHEDULE_ENTRY_CLEAR:
		return handleCmdScheduleEntryClear(buffer, size, accessLevel);
	case CMD_INCREASE_TX:
		return handleCmdIncreaseTx(buffer, size, accessLevel);
	case CMD_VALIDATE_SETUP:
		return handleCmdValidateSetup(buffer, size, accessLevel);
	case CMD_KEEP_ALIVE:
		return handleCmdKeepAlive(buffer, size, accessLevel);
	case CMD_KEEP_ALIVE_STATE:
		return handleCmdKeepAliveState(buffer, size, accessLevel);
	case CMD_KEEP_ALIVE_REPEAT_LAST:
		return handleCmdKeepAliveRepeatLast(buffer, size, accessLevel);
	case CMD_KEEP_ALIVE_MESH:
		return handleCmdKeepAliveMesh(buffer, size, accessLevel);
	case CMD_USER_FEEDBACK:
		return handleCmdUserFeedBack(buffer, size, accessLevel);
	case CMD_DISCONNECT:
		return handleCmdDisconnect(buffer, size, accessLevel);
	case CMD_SET_LED:
		return handleCmdSetLed(buffer, size, accessLevel);
	case CMD_RESET_ERRORS:
		return handleCmdResetErrors(buffer, size, accessLevel);
	case CMD_PWM:
		return handleCmdPwm(buffer, size, accessLevel);
	case CMD_SWITCH:
		return handleCmdSwitch(buffer, size, accessLevel);
	case CMD_RELAY:
		return handleCmdRelay(buffer, size, accessLevel);
	case CMD_MULTI_SWITCH:
		return handleCmdMultiSwitch(buffer, size, accessLevel);
	case CMD_MESH_COMMAND:
		return handleCmdMeshCommand(buffer, size, accessLevel);
	case CMD_ENABLE_CONT_POWER_MEASURE:
		return handleCmdEnableContPowerMeasure(buffer, size, accessLevel);
	case CMD_ALLOW_DIMMING:
		return handleCmdAllowDimming(buffer, size, accessLevel);
	case CMD_LOCK_SWITCH:
		return handleCmdLockSwitch(buffer, size, accessLevel);
	case CMD_SETUP:
		return handleCmdSetup(buffer, size, accessLevel);
	case CMD_ENABLE_SWITCHCRAFT:
		return handleCmdEnableSwitchcraft(buffer, size, accessLevel);
	case CMD_UART_MSG:
		return handleCmdUartMsg(buffer, size, accessLevel);
	case CMD_UART_ENABLE:
		return handleCmdUartEnable(buffer, size, accessLevel);
	default:
		LOGe("Unknown type: %u", type);
		return ERR_UNKNOWN_TYPE;
	}
	return ERR_SUCCESS;
}



ERR_CODE CommandHandler::handleCmdNop(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	// A no operation command to keep the connection alive.
	// No need to do anything here, the connection keep alive is handled in the stack.
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdGotoDfu(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "goto dfu");
	resetDelayed(GPREGRET_DFU_RESET);
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdReset(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "reset");
	resetDelayed(GPREGRET_SOFT_RESET);
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdEnableMesh(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "enable mesh");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool enable = payload->enable;

	LOGi("%s mesh", enable ? STR_ENABLE : STR_DISABLE);
	Settings::getInstance().updateFlag(CONFIG_MESH_ENABLED, enable, true);

#if BUILD_MESHING == 1
	if (enable) {
		Mesh::getInstance().start();
	} else {
		Mesh::getInstance().stop();
	}
#endif

	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdEnableEncryption(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
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
	Settings::getInstance().updateFlag(CONFIG_ENCRYPTION_ENABLED, enable, true);

	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdEnableIbeacon(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "enable ibeacon");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool enable = payload->enable;

	LOGi("%s ibeacon", enable ? STR_ENABLE : STR_DISABLE);
	Settings::getInstance().updateFlag(CONFIG_IBEACON_ENABLED, enable, true);

	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdEnableScanner(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
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
	Settings::getInstance().updateFlag(CONFIG_SCANNER_ENABLED, enable, true);

	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdScanDevices(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "scan devices");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	// TODO: deprecate this function?
	return ERR_NOT_IMPLEMENTED;

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool start = payload->enable;

	if (start) {
		Scanner::getInstance().manualStartScan();
	}
	else {
		// todo: if not tracking. do we need that still?
//				if (!_trackMode) {
		Scanner::getInstance().manualStopScan();

		ScanResult* results = Scanner::getInstance().getResults();

#ifdef PRINT_DEBUG
		results->print();
#endif

		buffer_ptr_t buffer;
		uint16_t dataLength;
		results->getBuffer(buffer, dataLength);

		EventDispatcher::getInstance().dispatch(EVT_SCANNED_DEVICES, buffer, dataLength);

#if BUILD_MESHING == 1
		if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
			MeshControl::getInstance().sendScanMessage(results->getList()->list, results->getSize());
		}
#endif
	}

	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdRequestServiceData(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "request service data");

	return ERR_NOT_IMPLEMENTED;

	// TODO: use ServiceData function for this?
//#if BUILD_MESHING == 1
//		state_item_t stateItem;
//		memset(&stateItem, 0, sizeof(stateItem));
//
//		State& state = State::getInstance();
//		Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &stateItem.id);
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


ERR_CODE CommandHandler::handleCmdFactoryReset(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
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


ERR_CODE CommandHandler::handleCmdSetTime(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
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


ERR_CODE CommandHandler::handleCmdScheduleEntrySet(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "schedule entry");
	if (size < sizeof(schedule_command_t)) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	schedule_command_t* entry = (schedule_command_t*)buffer;
	ERR_CODE errCode = Scheduler::getInstance().setScheduleEntry(entry->id, &(entry->entry));
	if (FAILURE(errCode)) {
		return errCode;
	}
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdScheduleEntryClear(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	if (size < 1) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	uint8_t id = buffer[0];
	ERR_CODE errCode = Scheduler::getInstance().clearScheduleEntry(id);
	if (FAILURE(errCode)) {
		return errCode;
	}
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdIncreaseTx(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "increase TX");
//	if (!EncryptionHandler::getInstance().allowAccess(SETUP, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
//	uint8_t opMode;
//	State::getInstance().get(STATE_OPERATION_MODE, opMode);
//	if (opMode != OPERATION_MODE_SETUP) {
//		LOGw("only available in setup mode");
//		return ERR_NOT_AVAILABLE;
//	}
	Stack::getInstance().changeToNormalTxPowerMode();
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdSetup(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "setup");
	ERR_CODE errCode = Setup::getInstance().handleCommand(buffer, size);
	return errCode;
}


ERR_CODE CommandHandler::handleCmdValidateSetup(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "validate setup");
	return ERR_NOT_IMPLEMENTED;
}


ERR_CODE CommandHandler::handleCmdKeepAlive(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "keep alive");

	EventDispatcher::getInstance().dispatch(EVT_KEEP_ALIVE);
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdKeepAliveState(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "keep alive state");

	if (size != sizeof(keep_alive_state_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	EventDispatcher::getInstance().dispatch(EVT_KEEP_ALIVE, buffer, size);
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdKeepAliveRepeatLast(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "keep alive repeat");
#if BUILD_MESHING == 1
	MeshControl::getInstance().sendLastKeepAliveMessage();
#endif
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdKeepAliveMesh(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "keep alive mesh");
#if BUILD_MESHING == 1
	keep_alive_message_t* keepAliveMsg = (keep_alive_message_t*) buffer;
	// This function also checks the validity of the msg.
	return MeshControl::getInstance().sendKeepAliveMessage(keepAliveMsg, size);
#endif
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdUserFeedBack(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "user feedback");
	return ERR_NOT_IMPLEMENTED;
}


ERR_CODE CommandHandler::handleCmdDisconnect(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "disconnect");
	Stack::getInstance().disconnect();
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdSetLed(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
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


ERR_CODE CommandHandler::handleCmdResetErrors(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "reset errors");
	if (size != sizeof(state_errors_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	state_errors_t* payload = (state_errors_t*) buffer;
	state_errors_t stateErrors;
	State::getInstance().get(STATE_ERRORS, stateErrors.asInt);
	LOGd("old errors %u - reset %u", stateErrors.asInt, *payload);
	stateErrors.asInt &= ~(payload->asInt);
	LOGd("new errors %u", stateErrors.asInt);
	State::getInstance().set(STATE_ERRORS, stateErrors.asInt);
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdPwm(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
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


ERR_CODE CommandHandler::handleCmdSwitch(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
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


ERR_CODE CommandHandler::handleCmdRelay(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
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


ERR_CODE CommandHandler::handleCmdMultiSwitch(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
//	if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "multi switch");
#if BUILD_MESHING == 1
	multi_switch_message_t* multiSwitchMsg = (multi_switch_message_t*) buffer;
	// This function also checks the validity of the msg.
	return MeshControl::getInstance().sendMultiSwitchMessage(multiSwitchMsg, size);
#endif
	return ERR_SUCCESS;
}


ERR_CODE CommandHandler::handleCmdMeshCommand(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
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


ERR_CODE CommandHandler::handleCmdEnableContPowerMeasure(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
		LOGe("Commands not available for device type %d", _boardConfig->deviceType);
		return ERR_NOT_AVAILABLE;
	}

//	if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
	LOGi(STR_HANDLE_COMMAND, "enable cont power measure");
	return ERR_NOT_IMPLEMENTED;
}

ERR_CODE CommandHandler::handleCmdAllowDimming(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "allow dimming");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool enable = payload->enable;

	LOGi("allow dimming: %u", enable);

	if (enable && Settings::getInstance().isSet(CONFIG_SWITCH_LOCKED)) {
		LOGw("unlock switch");
		bool lockEnable = false;
		Settings::getInstance().updateFlag(CONFIG_SWITCH_LOCKED, lockEnable, true);
		EventDispatcher::getInstance().dispatch(EVT_SWITCH_LOCKED, &lockEnable, sizeof(bool));
	}

	Settings::getInstance().updateFlag(CONFIG_PWM_ALLOWED, enable, true);
	EventDispatcher::getInstance().dispatch(EVT_PWM_ALLOWED, &enable, sizeof(bool));
	return ERR_SUCCESS;
}

ERR_CODE CommandHandler::handleCmdLockSwitch(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "lock switch");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool enable = payload->enable;

	LOGi("lock switch: %u", enable);

	if (enable && Settings::getInstance().isSet(CONFIG_PWM_ALLOWED)) {
		LOGw("can't lock switch");
		return ERR_NOT_AVAILABLE;
	}

	Settings::getInstance().updateFlag(CONFIG_SWITCH_LOCKED, enable, true);
	EventDispatcher::getInstance().dispatch(EVT_SWITCH_LOCKED, &enable, sizeof(bool));
	return ERR_SUCCESS;
}

ERR_CODE CommandHandler::handleCmdEnableSwitchcraft(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGi(STR_HANDLE_COMMAND, "enable switchcraft");

	if (size != sizeof(enable_message_payload_t)) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
	bool enable = payload->enable;

	Settings::getInstance().updateFlag(CONFIG_SWITCHCRAFT_ENABLED, enable, true);
	EventDispatcher::getInstance().dispatch(EVT_SWITCHCRAFT_ENABLED, &enable, sizeof(bool));
	return ERR_SUCCESS;
}

ERR_CODE CommandHandler::handleCmdUartMsg(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGd(STR_HANDLE_COMMAND, "UART msg");

	if (!size) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_BLE_MSG, buffer, size);
	return ERR_SUCCESS;
}

ERR_CODE CommandHandler::handleCmdUartEnable(buffer_ptr_t buffer, const uint16_t size, const EncryptionAccessLevel accessLevel) {
	LOGd(STR_HANDLE_COMMAND, "UART enable");

	if (size != 1) {
		LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	uint8_t enable = *(uint8_t*) buffer;
//	ERR_CODE errCode = Settings::getInstance().writeToStorage(CONFIG_UART_ENABLED, buffer, size);
	ERR_CODE errCode = Settings::getInstance().set(CONFIG_UART_ENABLED, buffer, size);
	if (errCode != ERR_SUCCESS) {
		return errCode;
	}
	serial_enable((serial_enable_t)enable);
	return ERR_SUCCESS;
}



EncryptionAccessLevel CommandHandler::getRequiredAccessLevel(const CommandHandlerTypes type) {
//	switch (type) {
//	case CMD_SWITCH:
//	case CMD_PWM:
//	case CMD_SET_TIME:
//	case CMD_GOTO_DFU:
//	case CMD_RESET:
//	case CMD_FACTORY_RESET:
//	case CMD_KEEP_ALIVE_STATE:
//	case CMD_KEEP_ALIVE:
//	case CMD_ENABLE_MESH:
//	case CMD_ENABLE_ENCRYPTION:
//	case CMD_ENABLE_IBEACON:
//	case CMD_ENABLE_CONT_POWER_MEASURE:
//	case CMD_ENABLE_SCANNER:
//	case CMD_SCAN_DEVICES:
//	case CMD_USER_FEEDBACK:
//	case CMD_SCHEDULE_ENTRY_SET:
//	case CMD_RELAY:
//	case CMD_VALIDATE_SETUP:
//	case CMD_REQUEST_SERVICE_DATA:
//	case CMD_DISCONNECT:
//	case CMD_SET_LED:
//	case CMD_NOP:
//	case CMD_INCREASE_TX:
//	case CMD_RESET_ERRORS:
//	case CMD_KEEP_ALIVE_REPEAT_LAST:
//	case CMD_MULTI_SWITCH:
//	case CMD_SCHEDULE_ENTRY_CLEAR:
//	case CMD_KEEP_ALIVE_MESH:
//	case CMD_MESH_COMMAND:
//	}

	switch (type) {
	case CMD_VALIDATE_SETUP:
	case CMD_INCREASE_TX:
	case CMD_SETUP:
		return GUEST; // These commands are only available in setup mode.

	case CMD_SWITCH:
	case CMD_PWM:
	case CMD_RELAY:
	case CMD_KEEP_ALIVE:
	case CMD_DISCONNECT:
	case CMD_NOP:
	case CMD_KEEP_ALIVE_REPEAT_LAST:
	case CMD_MULTI_SWITCH:
	case CMD_MESH_COMMAND:
		return GUEST;

	case CMD_SET_TIME:
	case CMD_KEEP_ALIVE_STATE:
	case CMD_SCHEDULE_ENTRY_SET:
	case CMD_REQUEST_SERVICE_DATA:
	case CMD_SCHEDULE_ENTRY_CLEAR:
	case CMD_KEEP_ALIVE_MESH:
		return MEMBER;

	case CMD_GOTO_DFU:
	case CMD_RESET:
	case CMD_FACTORY_RESET:
	case CMD_ENABLE_MESH:
	case CMD_ENABLE_ENCRYPTION:
	case CMD_ENABLE_IBEACON:
	case CMD_ENABLE_CONT_POWER_MEASURE:
	case CMD_ENABLE_SCANNER:
	case CMD_SCAN_DEVICES:
	case CMD_USER_FEEDBACK:
	case CMD_SET_LED:
	case CMD_RESET_ERRORS:
	case CMD_ALLOW_DIMMING:
	case CMD_LOCK_SWITCH:
	case CMD_ENABLE_SWITCHCRAFT:
	case CMD_UART_MSG:
	case CMD_UART_ENABLE:
		return ADMIN;
	default:
		return NOT_SET;
	}
}

bool CommandHandler::allowedAsMeshCommand(const CommandHandlerTypes type) {
	switch (type) {
	case CMD_SWITCH:
	case CMD_PWM:
	case CMD_RELAY:
	case CMD_SET_TIME:
	case CMD_RESET:
	case CMD_FACTORY_RESET:
	case CMD_KEEP_ALIVE_STATE:
	case CMD_KEEP_ALIVE:
	case CMD_ENABLE_SCANNER:
	case CMD_USER_FEEDBACK:
	case CMD_SCHEDULE_ENTRY_SET:
	case CMD_REQUEST_SERVICE_DATA:
	case CMD_SET_LED:
	case CMD_RESET_ERRORS:
	case CMD_SCHEDULE_ENTRY_CLEAR:
	case CMD_UART_MSG:
		return true;
	default:
		return false;
	}
	return false;
}

void CommandHandler::handleEvent(uint16_t evt, void* p_data, uint16_t length) {
	switch (evt) {
	case EVT_DO_RESET_DELAYED: {
		if (length != sizeof(evt_do_reset_delayed_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, length);
			return;
		}
		evt_do_reset_delayed_t* payload = (evt_do_reset_delayed_t*)p_data;
		resetDelayed(payload->resetCode, payload->delayMs);
		break;
	}
	case EVT_RX_CONTROL: {
		stream_header_t* streamHeader = (stream_header_t*)p_data;
		if (length - sizeof(stream_header_t) < streamHeader->length) {
			LOGw(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
			break;
		}
		uint8_t* streamPayload = (uint8_t*)p_data + sizeof(stream_header_t);
		handleCommand((CommandHandlerTypes)streamHeader->type, streamPayload, streamHeader->length);
	}
	}
}
