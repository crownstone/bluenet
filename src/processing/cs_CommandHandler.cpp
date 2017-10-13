/*
 * Author: Crownstone
 * Copyright: Crownstone
 * Date: May 18, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT
 */


#include <cfg/cs_Boards.h>
#include <cfg/cs_Strings.h>
#include <drivers/cs_Serial.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_Scheduler.h>
#include <processing/cs_FactoryReset.h>

#include <processing/cs_Switch.h>
#include <processing/cs_TemperatureGuard.h>
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
	Timer::getInstance().createSingleShot(_delayTimerId, execute_delayed);
	Timer::getInstance().createSingleShot(_resetTimerId, (app_timer_timeout_handler_t) reset);
}

void CommandHandler::resetDelayed(uint8_t opCode) {
	LOGi("Reset in 2s, code=%u", opCode);
	static uint8_t resetOpCode = opCode;
	//! TODO: do we really have to make a new timer here every time?
//	app_timer_id_t resetTimer;
//	Timer::getInstance().createSingleShot(resetTimer, (app_timer_timeout_handler_t) reset);
	Timer::getInstance().start(_resetTimerId, MS_TO_TICKS(2000), &resetOpCode);
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

	switch (type) {
	case CMD_NOP: {
		// a nop command to keep the connection alive
		// don't need to do anything here, the connection keep alive is handled in the stack
		break;
	}
	case CMD_GOTO_DFU: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "goto dfu");
		resetDelayed(GPREGRET_DFU_RESET);
		break;
	}
	case CMD_RESET: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "reset");

//		if (size != sizeof(opcode_message_payload_t)) {
//			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
//			return ERR_WRONG_PAYLOAD_LENGTH;
//		}
//
//		opcode_message_payload_t* payload = (opcode_message_payload_t*) buffer;
//		uint8_t resetOp = payload->opCode;

		resetDelayed(GPREGRET_SOFT_RESET);
		break;
	}
	case CMD_ENABLE_MESH: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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

		break;
	}
	case CMD_ENABLE_ENCRYPTION: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "enable encryption, tbd");
		return ERR_NOT_IMPLEMENTED;

		if (size != sizeof(enable_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s encryption", enable ? STR_ENABLE : STR_DISABLE);
		Settings::getInstance().updateFlag(CONFIG_ENCRYPTION_ENABLED, enable, true);
		// TODO: Clear keys on disable? Check if keys are set on enable?
		// todo: stack/service/characteristics need to be refactored if we also want to update characteristics
		// on the fly
		// for now, this only takes effect on next reset
//			EventDispatcher::getInstance().dispatch(EVT_ENABLED_ENCRYPTION, &enable, 1);

		break;
	}
	case CMD_ENABLE_IBEACON: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "enable ibeacon");

		if (size != sizeof(enable_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s ibeacon", enable ? STR_ENABLE : STR_DISABLE);
		Settings::getInstance().updateFlag(CONFIG_IBEACON_ENABLED, enable, true);

		break;
	}
	case CMD_ENABLE_SCANNER: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "enable scanner");

		enable_scanner_message_payload_t* payload = (enable_scanner_message_payload_t*) buffer;
		bool enable = payload->enable;
		uint16_t delay = payload->delay;

		if (size != sizeof(enable_scanner_message_payload_t)) {
			//! if we want to disable, we don't really need the delay, so
			// we can accept the command as long as size is 1
			if (size != 1 || enable) {
				LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
		}

		LOGi("%s scanner", enable ? STR_ENABLE : STR_DISABLE);
		if (enable) {
			LOGi("delay: %d ms", delay);
			if (delay) {
				Scanner::getInstance().delayedStart(delay);
			} else {
				Scanner::getInstance().start();
			}
		} else {
			Scanner::getInstance().stop();
		}

		Settings::getInstance().updateFlag(CONFIG_SCANNER_ENABLED, enable, true);

		break;
	}
	case CMD_SCAN_DEVICES: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "scan devices");

		if (size != sizeof(enable_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool start = payload->enable;

		if (start) {
			Scanner::getInstance().manualStartScan();
		} else {
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

		break;
	}
	case CMD_REQUEST_SERVICE_DATA: {
		if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "request service");

#if BUILD_MESHING == 1
		state_item_t stateItem;
		memset(&stateItem, 0, sizeof(stateItem));

		State& state = State::getInstance();
		Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &stateItem.id);

		state.get(STATE_SWITCH_STATE, stateItem.switchState);

		//! TODO: implement setting the eventBitmask in a better way
		//! Maybe get it from service data directly? Or should we store the eventBitmask in the State?
		state_errors_t state_errors;
		state.get(STATE_ERRORS, &state_errors, sizeof(state_errors_t));
		stateItem.eventBitmask = 0;
		if (state_errors.asInt != 0) {
			stateItem.eventBitmask |= 1 << SERVICE_BITMASK_ERROR;
		}

		state.get(STATE_POWER_USAGE, (int32_t&)stateItem.powerUsage);
		state.get(STATE_ACCUMULATED_ENERGY, (int32_t&)stateItem.accumulatedEnergy);

		MeshControl::getInstance().sendServiceDataMessage(stateItem, true);
#endif

		break;
	}
	case CMD_FACTORY_RESET: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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

		break;
	}
	case CMD_SET_TIME: {
		if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "set time:");

		if (size != sizeof(uint32_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		uint32_t value = *(uint32_t*)buffer;

		Scheduler::getInstance().setTime(value);

		break;
	}
	case CMD_SCHEDULE_ENTRY_SET: {
		if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "schedule entry");
		if (size < sizeof(schedule_command_t)) {
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		schedule_command_t* entry = (schedule_command_t*)buffer;
		ERR_CODE errCode = Scheduler::getInstance().setScheduleEntry(entry->id, &(entry->entry));
		if (FAILURE(errCode)) {
			return errCode;
		}
		break;
	}
	case CMD_SCHEDULE_ENTRY_CLEAR: {
		if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		if (size < 1) {
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		uint8_t id = buffer[0];
		ERR_CODE errCode = Scheduler::getInstance().clearScheduleEntry(id);
		if (FAILURE(errCode)) {
			return errCode;
		}
		break;
	}
	case CMD_INCREASE_TX: {
		uint8_t opMode;
		State::getInstance().get(STATE_OPERATION_MODE, opMode);
		if (opMode == OPERATION_MODE_SETUP) {
			Nrf51822BluetoothStack::getInstance().changeToNormalTxPowerMode();
		}
		else {
			LOGw("validate setup only available in setup mode");
			return ERR_NOT_AVAILABLE;
		}
		break;
	}
	case CMD_VALIDATE_SETUP: {
		// we do not need to check for the setup validation since this is not encrypted
		LOGi(STR_HANDLE_COMMAND, "validate setup");

		uint8_t opMode;
		State::getInstance().get(STATE_OPERATION_MODE, opMode);
		if (opMode == OPERATION_MODE_SETUP) {

			Settings& settings = Settings::getInstance();

			uint8_t key[ENCYRPTION_KEY_LENGTH];
			uint8_t blankKey[ENCYRPTION_KEY_LENGTH] = {};

			if (settings.isSet(CONFIG_ENCRYPTION_ENABLED)) {
				// validate encryption keys are not 0
				settings.get(CONFIG_KEY_ADMIN, key);
				if (memcmp(key, blankKey, ENCYRPTION_KEY_LENGTH) == 0) {
					LOGw("owner key is not set!");
					return ERR_COMMAND_FAILED;
				}

				settings.get(CONFIG_KEY_MEMBER, key);
				if (memcmp(key, blankKey, ENCYRPTION_KEY_LENGTH) == 0) {
					LOGw("member key is not set!");
					return ERR_COMMAND_FAILED;
				}

				settings.get(CONFIG_KEY_GUEST, key);
				if (memcmp(key, blankKey, ENCYRPTION_KEY_LENGTH) == 0) {
					LOGw("guest key is not set!");
					return ERR_COMMAND_FAILED;
				}
			}

			// validate crownstone id is not 0
			uint16_t crownstoneId;
			settings.get(CONFIG_CROWNSTONE_ID, &crownstoneId);

			if (crownstoneId == 0) {
				LOGw("crownstone id has to be set during setup mode");
				return ERR_COMMAND_FAILED;
			}

			// validate major and minor
			uint16_t major;
			settings.get(CONFIG_IBEACON_MAJOR, &major);

			if (major == 0) {
				LOGw("ibeacon major is not set!");
				return ERR_COMMAND_FAILED;
			}

			uint16_t minor;
			settings.get(CONFIG_IBEACON_MINOR, &minor);

			if (minor == 0) {
				LOGw("ibeacon minor is not set!");
				return ERR_COMMAND_FAILED;
			}

			// TODO: check mesh access address

			LOGi("Setup completed, resetting to normal mode");

			//! if validation ok, set opMode to normal mode
			State::getInstance().set(STATE_OPERATION_MODE, (uint8_t)OPERATION_MODE_NORMAL);

			//! Switch relay on
			switch_message_payload_t switchPayload;
			switchPayload.switchState = SWITCH_ON;
			handleCommand(CMD_SWITCH, (uint8_t*)&switchPayload, 1, ADMIN);

			//! then reset device
			resetDelayed(GPREGRET_SOFT_RESET);
		} else {
			LOGw("validate setup only available in setup mode");
			return ERR_NOT_AVAILABLE;
		}

		break;
	}
	case CMD_KEEP_ALIVE: {
		if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "keep alive");

		EventDispatcher::getInstance().dispatch(EVT_KEEP_ALIVE);

//		Timer::getInstance().reset(_keepAliveTimerId, )

		// no params

		break;
	}
	case CMD_KEEP_ALIVE_STATE: {
		if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "keep alive state");

		if (size != sizeof(keep_alive_state_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

//		keep_alive_state_message_payload_t state = *(keep_alive_state_message_payload_t*)buffer;
//		LOGi("switch: %d", state.switchState.switchState);
//		LOGi("timeout: %d s", state.timeout);

		EventDispatcher::getInstance().dispatch(EVT_KEEP_ALIVE, buffer, size);

		// state as param: pwm or on/off (switch state) as param
		// timeout as param
		//   + 1/2 interval

		// in config

		// 1 keepalive per 2 minutes

		break;
	}
	case CMD_KEEP_ALIVE_REPEAT_LAST: {
		if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "keep alive repeat");
#if BUILD_MESHING == 1
		MeshControl::getInstance().sendLastKeepAliveMessage();
#endif
		break;
	}

	case CMD_USER_FEEDBACK: {
		if (!EncryptionHandler::getInstance().allowAccess(MEMBER, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "user feedback");
		return ERR_NOT_IMPLEMENTED;

		// todo: tbd
		break;
	}
	case CMD_DISCONNECT: {
		LOGi(STR_HANDLE_COMMAND, "disconnect");
		Nrf51822BluetoothStack::getInstance().disconnect();
		break;
	}
	case CMD_SET_LED: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "set led");

		if (_boardConfig->flags.hasLed) {
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
			} else {
				nrf_gpio_pin_clear(ledPin);
			}
		} else {
			LOGe("No LEDs on this board!");
		}
		break;
	}
	case CMD_RESET_ERRORS: {
		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "reset errors");
		if (size != sizeof(state_errors_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}
		state_errors_t* payload = (state_errors_t*) buffer;
		state_errors_t state_errors;
		State::getInstance().get(STATE_ERRORS, &state_errors, sizeof(state_errors_t));
		LOGd("old errors %u - reset %u", state_errors, *payload);
		state_errors.asInt &= ~(payload->asInt);
		LOGd("new errors %d", state_errors);
		State::getInstance().set(STATE_ERRORS, &state_errors, sizeof(state_errors_t));
		break;
	}

	// Crownstone specific commands are only available if device type is set to Crownstone.
	// E.g. GuideStone does not support power measure or switching commands
	case CMD_PWM: {
		if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
			LOGe("Commands not available for device type %d", _boardConfig->deviceType);
			return ERR_NOT_AVAILABLE;
		}

		if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
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
		break;
	}
	case CMD_SWITCH: {
		if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
			LOGe("Commands not available for device type %d", _boardConfig->deviceType);
			return ERR_NOT_AVAILABLE;
		}

		if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "switch");

		if (size != sizeof(switch_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		switch_message_payload_t* payload = (switch_message_payload_t*) buffer;
		Switch::getInstance().setSwitch(payload->switchState);

//		//! Switch off pwm, as we're using the relay
//		uint8_t currentPwm = Switch::getInstance().getPwm();
//		if (currentPwm != 0) {
//			Switch::getInstance().setPwm(0);
//		}
//
//		if (value == 0) {
//			Switch::getInstance().relayOff();
//		} else {
//			Switch::getInstance().relayOn();
//		}
		break;
	}
	case CMD_RELAY: {
		if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
			LOGe("Commands not available for device type %d", _boardConfig->deviceType);
			return ERR_NOT_AVAILABLE;
		}

		if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "relay");

		if (size != sizeof(switch_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		switch_message_payload_t* payload = (switch_message_payload_t*) buffer;
		uint8_t value = payload->switchState;

		if (value == 0) {
			Switch::getInstance().relayOff();
		} else {
			Switch::getInstance().relayOn();
		}

		break;
	}
	case CMD_MULTI_SWITCH: {
		if (!EncryptionHandler::getInstance().allowAccess(GUEST, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "multi switch");
#if BUILD_MESHING == 1
		multi_switch_message_t* multiSwitchMsg = (multi_switch_message_t*) buffer;
		MeshControl::getInstance().sendMultiSwitchMessage(multiSwitchMsg, size);
#endif
		break;
	}
	case CMD_ENABLE_CONT_POWER_MEASURE: {
		if (!IS_CROWNSTONE(_boardConfig->deviceType)) {
			LOGe("Commands not available for device type %d", _boardConfig->deviceType);
			return ERR_NOT_AVAILABLE;
		}

		if (!EncryptionHandler::getInstance().allowAccess(ADMIN, accessLevel)) return ERR_ACCESS_NOT_ALLOWED;
		LOGi(STR_HANDLE_COMMAND, "enable cont power measure");
		return ERR_NOT_IMPLEMENTED;

		if (size != sizeof(enable_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		__attribute__((unused)) bool enable = payload->enable;

		LOGi("%s continuous power measurements", enable ? STR_ENABLE : STR_DISABLE);

		// todo: tbd
		break;
	}
	default: {
		LOGe("Command type not found!!");
		return ERR_COMMAND_NOT_FOUND;
	}
	}

	return ERR_SUCCESS;
}
