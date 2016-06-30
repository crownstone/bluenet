/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 18, 2016
 * License: LGPLv3+
 */

#include <processing/cs_CommandHandler.h>

#include <storage/cs_Settings.h>
#include <drivers/cs_Serial.h>
#include <processing/cs_PowerSampling.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_Scheduler.h>
#include <processing/cs_Switch.h>
#include <mesh/cs_MeshControl.h>
#include <mesh/cs_Mesh.h>
#include <storage/cs_State.h>
#include <cfg/cs_Strings.h>

//#define PRINT_DEBUG

void reset(void* p_context) {

	uint32_t cmd = *(int32_t*) p_context;

	if (cmd == GPREGRET_DFU_RESET) {
		LOGi(MSG_FIRMWARE_UPDATE);
	} else {
		LOGi(MSG_RESET);
	}

	LOGi("executing reset: %d", cmd);
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

CommandHandler::CommandHandler() : _stack(NULL) {
	Timer::getInstance().createSingleShot(_delayTimer, execute_delayed);
}

void CommandHandler::resetDelayed(uint8_t opCode) {
	static uint8_t restOpCode = opCode;
	app_timer_id_t resetTimer;
	Timer::getInstance().createSingleShot(resetTimer, (app_timer_timeout_handler_t) reset);
	Timer::getInstance().start(resetTimer, MS_TO_TICKS(2000), &restOpCode);
}

ERR_CODE CommandHandler::handleCommandDelayed(CommandHandlerTypes type, buffer_ptr_t buffer, uint16_t size, uint32_t delay) {
	delayed_command_t* buf = new delayed_command_t();
	buf->type = type;
	buf->buffer = new uint8_t[size];
	memcpy(buf->buffer, buffer, size);
	buf->size = size;
	Timer::getInstance().start(_delayTimer, MS_TO_TICKS(delay), buf);
	LOGi("execute with delay %d", delay);
	return NRF_SUCCESS;
}

ERR_CODE CommandHandler::handleCommand(CommandHandlerTypes type) {
	return handleCommand(type, NULL, 0);
}

ERR_CODE CommandHandler::handleCommand(CommandHandlerTypes type, buffer_ptr_t buffer, uint16_t size) {

	switch (type) {
	case CMD_GOTO_DFU: {
		LOGi(STR_HANDLE_COMMAND, "goto dfu");
		resetDelayed(GPREGRET_DFU_RESET);
		break;
	}
	case CMD_RESET: {
		LOGi(STR_HANDLE_COMMAND, "reset");

		if (size != sizeof(opcode_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		opcode_message_payload_t* payload = (opcode_message_payload_t*) buffer;
		static uint8_t resetOp = payload->opCode;

		resetDelayed(resetOp);
		break;
	}
	case CMD_ENABLE_MESH: {
		LOGi(STR_HANDLE_COMMAND, "enable mesh");

		if (size != sizeof(enable_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s mesh", enable ? STR_ENABLE : STR_DISABLE);
		Settings::getInstance().updateFlag(CONFIG_MESH_ENABLED, enable, true);

		if (enable) {
			Mesh::getInstance().start();
		} else {
			Mesh::getInstance().stop();
		}

		break;
	}
	case CMD_ENABLE_ENCRYPTION: {
		LOGi(STR_HANDLE_COMMAND, "enable encryption");
		return ERR_NOT_IMPLEMENTED;

		if (size != sizeof(enable_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s encryption", enable ? STR_ENABLE : STR_DISABLE);
		Settings::getInstance().updateFlag(CONFIG_ENCRYPTION_ENABLED, enable, true);
		// todo: stack/service/characteristics need to be refactored if we also want to update characteristics
		// on the fly
		// for now, this only takes effect on next reset
//			EventDispatcher::getInstance().dispatch(EVT_ENABLED_ENCRYPTION, &enable, 1);

		break;
	}
	case CMD_ENABLE_IBEACON: {
		LOGi(STR_HANDLE_COMMAND, "enable ibeacon");

		if (size != sizeof(enable_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s ibeacon", enable ? STR_ENABLE : STR_DISABLE);
		Settings::getInstance().updateFlag(CONFIG_IBEACON_ENABLED, enable, true);
//		EventDispatcher::getInstance().dispatch(EVT_ENABLED_IBEACON, &enable, sizeof(enable));

		break;
	}
	case CMD_ENABLE_SCANNER: {
		LOGi(STR_HANDLE_COMMAND, "enable scanner");

		enable_scanner_message_payload_t* payload = (enable_scanner_message_payload_t*) buffer;
		bool enable = payload->enable;
		uint16_t delay = payload->delay;

		if (size != sizeof(enable_scanner_message_payload_t)) {
			//! if we want tod isable, we don't really need the delay, so
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

			if (Settings::getInstance().isSet(CONFIG_MESH_ENABLED)) {
				MeshControl::getInstance().sendScanMessage(results->getList()->list, results->getSize());
			}
		}

		break;
	}
	case CMD_REQUEST_SERVICE_DATA: {
		LOGi(STR_HANDLE_COMMAND, "request service");

		service_data_mesh_message_t serviceData;
		memset(&serviceData, 0, sizeof(serviceData));

		State& state = State::getInstance();
		Settings::getInstance().get(CONFIG_CROWNSTONE_ID, &serviceData.crownstoneId);

		state.get(STATE_SWITCH_STATE, serviceData.switchState);

		// todo get event bitmask
		serviceData.eventBitmask = 9;

		state.get(STATE_POWER_USAGE, (int32_t&)serviceData.powerUsage);
		state.get(STATE_ACCUMULATED_ENERGY, (int32_t&)serviceData.accumulatedEnergy);
		state.get(STATE_TEMPERATURE, (int32_t&)serviceData.temperature);

		MeshControl::getInstance().sendServiceDataMessage(&serviceData);

		break;
	}
	case CMD_FACTORY_RESET: {
		LOGi(STR_HANDLE_COMMAND, "factory reset");

		if (size != sizeof(FACTORY_RESET_CODE)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, sizeof(FACTORY_RESET_CODE));
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		factory_reset_message_payload_t* payload = (factory_reset_message_payload_t*) buffer;
		uint32_t resetCode = payload->resetCode;

		if (resetCode == FACTORY_RESET_CODE) {
//			LOGf("factory reset");

			Settings::getInstance().factoryReset(resetCode);
			State::getInstance().factoryReset(resetCode);
			// todo: might not be neccessary if we only use dm in setup mode we can handle it specifically
			//   there. maybe with a mode factory reset
			// todo: remove stack again from CommandHandler if we don't need it here
			_stack->device_manager_reset();

			LOGi("factory reset done, rebooting device in 2s ...");

			resetDelayed(GPREGRET_SOFT_RESET);

		} else {
			LOGi("wrong code received: %p", resetCode);
//			LOGi("factory reset code is: %p", FACTORY_RESET_CODE);
			return ERR_WRONG_PARAMETER;
		}

		break;
	}
	case CMD_SET_TIME: {
		LOGi(STR_HANDLE_COMMAND, "set time:");

		if (size != sizeof(uint32_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		uint32_t value = *(uint32_t*)buffer;

		Scheduler::getInstance().setTime(value);

		break;
	}
	case CMD_SCHEDULE_ENTRY: {
		LOGi(STR_HANDLE_COMMAND, "schedule entry");

#if SCHEDULER_ENABLED==1
		Scheduler& scheduler = Scheduler::getInstance();

		ScheduleEntry entry;

		if (entry.assign(buffer, size)) {
			return -1;
		}
		schedule_entry_t* entryStruct = entry.getStruct();
		if (!entryStruct->nextTimestamp) {
			scheduler.removeScheduleEntry(entryStruct);
		}
		else {

			// Check if entry is correct
			if (entryStruct->nextTimestamp < scheduler.getTime()) {
				return -1;
			}
			switch (ScheduleEntry::getTimeType(entryStruct)) {
			case SCHEDULE_TIME_TYPE_REPEAT:
				if (entryStruct->repeat.repeatTime == 0) {
					return -1;
				}
				break;
			case SCHEDULE_TIME_TYPE_DAILY:
				if (entryStruct->daily.nextDayOfWeek > 6) {
					return -1;
				}
				break;
			case SCHEDULE_TIME_TYPE_ONCE:
				break;
			}

			scheduler.addScheduleEntry(entryStruct);
		}
#endif

		break;
	}
	case CMD_VALIDATE_SETUP: {
		LOGi(STR_HANDLE_COMMAND, "validate setup");

		uint8_t opMode;
		State::getInstance().get(STATE_OPERATION_MODE, opMode);
		if (opMode == OPERATION_MODE_SETUP) {

			Settings& settings = Settings::getInstance();

			uint8_t key[ENCYRPTION_KEY_LENGTH];
			uint8_t blankKey[ENCYRPTION_KEY_LENGTH] = {};

			// validate encryption keys are not 0
			settings.get(CONFIG_KEY_OWNER, key);
			if (memcmp(key, blankKey, ENCYRPTION_KEY_LENGTH) == 0) {
//				LOGw("owner key is not set!");
				return ERR_COMMAND_FAILED;
			}

			settings.get(CONFIG_KEY_MEMBER, key);
			if (memcmp(key, blankKey, ENCYRPTION_KEY_LENGTH) == 0) {
//				LOGw("member key is not set!");
				return ERR_COMMAND_FAILED;
			}

			settings.get(CONFIG_KEY_GUEST, key);
			if (memcmp(key, blankKey, ENCYRPTION_KEY_LENGTH) == 0) {
//				LOGw("guest key is not set!");
				return ERR_COMMAND_FAILED;
			}

			// validate crownstone id is not 0
			uint16_t crownstoneId;
			settings.get(CONFIG_CROWNSTONE_ID, &crownstoneId);

			if (crownstoneId == 0) {
//				LOGw("crownstone id has to be set during setup mode");
				return ERR_COMMAND_FAILED;
			}

			// validate major and minor
			uint16_t major;
			settings.get(CONFIG_IBEACON_MAJOR, &major);

			if (major == 0) {
//				LOGw("ibeacon major is not set!");
				return ERR_COMMAND_FAILED;
			}

			uint16_t minor;
			settings.get(CONFIG_IBEACON_MINOR, &minor);

			if (minor == 0) {
//				LOGw("ibeacon minor is not set!");
				return ERR_COMMAND_FAILED;
			}

			LOGi("Setup completed, resetting to normal mode");

			//! if validation ok, set opMode to normal mode
			State::getInstance().set(STATE_OPERATION_MODE, (uint8_t)OPERATION_MODE_NORMAL);

			//! then reset device
			resetDelayed(GPREGRET_SOFT_RESET);
		} else {
			LOGw("validate setup only available in setup mode");
			return ERR_NOT_AVAILABLE;
		}

		break;
	}

	case CMD_USER_FEEDBACK: {
		LOGi(STR_HANDLE_COMMAND, "user feedback");
		return ERR_NOT_IMPLEMENTED;

		// todo: tbd
		break;
	}
	case CMD_KEEP_ALIVE_STATE: {
		LOGi(STR_HANDLE_COMMAND, "keep alive");
		return ERR_NOT_IMPLEMENTED;

		// todo: tbd
		break;
	}
	case CMD_KEEP_ALIVE: {
		LOGi(STR_HANDLE_COMMAND, "keep alive");
		return ERR_NOT_IMPLEMENTED;

		// todo: tbd
		break;
	}
#if DEVICE_TYPE==DEVICE_CROWNSTONE
	// Crownstone specific commands are only available if device type is set to Crownstone.
	// E.g. GuideStone does not support power measure or switching commands
	case CMD_SWITCH: {
		LOGi(STR_HANDLE_COMMAND, "switch");
		// for now, same as pwm, but switch command should decide itself if relay or
		// pwm is used
	}
	case CMD_PWM: {
		LOGi(STR_HANDLE_COMMAND, "PWM");

		if (size != sizeof(switch_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		switch_message_payload_t* payload = (switch_message_payload_t*) buffer;
		uint8_t value = payload->switchState;

		uint8_t current = Switch::getInstance().getValue();
//		LOGi("current pwm: %d", current);
		if (value != current) {
			Switch::getInstance().setValue(value);
		}
		break;
	}
	case CMD_RELAY: {
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
	case CMD_ENABLE_CONT_POWER_MEASURE: {
		LOGi(STR_HANDLE_COMMAND, "enable cont power measure");
		return ERR_NOT_IMPLEMENTED;

		if (size != sizeof(enable_message_payload_t)) {
			LOGe(FMT_WRONG_PAYLOAD_LENGTH, size);
			return ERR_WRONG_PAYLOAD_LENGTH;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s continuous power measurements", enable ? STR_ENABLE : STR_DISABLE);

		// todo: tbd
		break;
	}
#else
	// Crownstone specific commands are only available if device type is set to Crownstone.
	// E.g. GuideStone does not support power measure or switching commands
	case CMD_SWITCH:
	case CMD_PWM:
	case CMD_RELAY:
	case CMD_ENABLE_CONT_POWER_MEASURE: {
		LOGe("Commands not available for device type %d", DEVICE_TYPE);
		return ERR_NOT_AVAILABLE;
	}
#endif
	default: {
		LOGe("Command type not found!!");
		return ERR_COMMAND_NOT_FOUND;
	}
	}

	return ERR_SUCCESS;
}
