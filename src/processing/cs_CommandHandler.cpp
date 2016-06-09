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

void reset(void* p_context) {

	uint32_t cmd = *(int32_t*) p_context;

	if (cmd == COMMAND_ENTER_RADIO_BOOTLOADER) {
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

void stop_mesh(void* p_context) {
	Mesh::getInstance().stop();
}


void CommandHandler::handleCommand(CommandHandlerTypes type) {
	handleCommand(type, NULL, 0);
}

void CommandHandler::handleCommand(CommandHandlerTypes type, buffer_ptr_t buffer, uint16_t size) {

	switch (type) {
	case CMD_PWM: {
		LOGi("handle PWM command");

		if (size != sizeof(switch_message_payload_t)) {
			LOGe("wrong payload length received: %d", size);
			return;
		}

		switch_message_payload_t* payload = (switch_message_payload_t*) buffer;
		uint8_t value = payload->switchState;

		uint8_t current = Switch::getInstance().getValue();
//		LOGi("current pwm: %d", current);
		if (value != current) {
			LOGi("update pwm to %i", value);
			Switch::getInstance().setValue(value);
		}
		break;
	}
	case CMD_SWITCH: {
		LOGi("handle switch command");

		if (size != sizeof(switch_message_payload_t)) {
			LOGe("wrong payload length received: %d", size);
			return;
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
	case CMD_GOTO_DFU: {
		LOGi("handle goto dfu command");

		static uint32_t resetOp = COMMAND_ENTER_RADIO_BOOTLOADER;

		app_timer_id_t resetTimer;
		Timer::getInstance().createSingleShot(resetTimer, (app_timer_timeout_handler_t) reset);
		Timer::getInstance().start(resetTimer, MS_TO_TICKS(100), &resetOp);
		break;
	}
	case CMD_RESET: {
		LOGi("handle reset command");

		if (size != sizeof(opcode_message_payload_t)) {
			LOGe("wrong payload length received: %d", size);
			return;
		}

		opcode_message_payload_t* payload = (opcode_message_payload_t*) buffer;
		static uint32_t resetOp = payload->opCode;

//			if (resetOp) {
		app_timer_id_t resetTimer;
		Timer::getInstance().createSingleShot(resetTimer, (app_timer_timeout_handler_t) reset);
		Timer::getInstance().start(resetTimer, MS_TO_TICKS(100), &resetOp);
//			} else {
		//todo: why nonzero?
//				LOGw("To reset write a nonzero value: %d", payload->resetOp);
//			}
		break;
	}
	case CMD_ENABLE_MESH: {
		LOGi("handle enable mesh command");

		if (size != sizeof(enable_message_payload_t)) {
			LOGe("wrong payload length received: %d", size);
			return;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s mesh", enable ? "Enabling" : "Disabling");
		Settings::getInstance().updateFlag(CONFIG_MESH_ENABLED, enable, true);

		if (enable) {
			Mesh::getInstance().start();
		} else {
			//! breaks if it is called directly...
//			Mesh::getInstance().stop();
			app_timer_id_t meshStopTimer;
			Timer::getInstance().createSingleShot(meshStopTimer, (app_timer_timeout_handler_t) stop_mesh);
			Timer::getInstance().start(meshStopTimer, MS_TO_TICKS(100), NULL);
		}

		break;
	}
	case CMD_ENABLE_ENCRYPTION: {
		LOGi("handle enable encryption command: tbd");

		if (size != sizeof(enable_message_payload_t)) {
			LOGe("wrong payload length received: %d", size);
			return;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s encryption", enable ? "Enabling" : "Disabling");
		Settings::getInstance().updateFlag(CONFIG_ENCRYPTION_ENABLED, enable, true);
		// todo: stack/service/characteristics need to be refactored if we also want to update characteristics
		// on the fly
		// for now, this only takes effect on next reset
//			EventDispatcher::getInstance().dispatch(EVT_ENABLED_ENCRYPTION, &enable, 1);

		break;
	}
	case CMD_ENABLE_IBEACON: {
		LOGi("handle enable ibeacon command");

		if (size != sizeof(enable_message_payload_t)) {
			LOGe("wrong payload length received: %d", size);
			return;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s ibeacon", enable ? "Enabling" : "Disabling");
		Settings::getInstance().updateFlag(CONFIG_IBEACON_ENABLED, enable, true);
		EventDispatcher::getInstance().dispatch(EVT_ENABLED_IBEACON, &enable, 1);

		break;
	}
	case CMD_ENABLE_CONT_POWER_MEASURE: {
		LOGi("handle enable cont power measure command: tbd");

		if (size != sizeof(enable_message_payload_t)) {
			LOGe("wrong payload length received: %d", size);
			return;
		}

		enable_message_payload_t* payload = (enable_message_payload_t*) buffer;
		bool enable = payload->enable;

		LOGi("%s continues power measurements", enable ? "Enabling" : "Disabling");

		// todo: tbd
		break;
	}
	case CMD_ENABLE_SCANNER: {
		LOGi("handle enable scanner command");

		enable_scanner_message_payload_t* payload = (enable_scanner_message_payload_t*) buffer;
		bool enable = payload->enable;
		uint16_t delay = payload->delay;

		if (size != sizeof(enable_scanner_message_payload_t)) {
			//! if we want tod isable, we don't really need the delay, so
			// we can accept the command as long as size is 1
			if (size != 1 || enable) {
				LOGe("wrong payload length received: %d", size);
				return;
			}
		}

		LOGi("%s scanner", enable ? "Enabling" : "Disabling");
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
		LOGi("handle scan devices command");

		if (size != sizeof(enable_message_payload_t)) {
			LOGe("wrong payload length received: %d", size);
			return;
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
			results->print();

			buffer_ptr_t buffer;
			uint16_t dataLength;
			results->getBuffer(buffer, dataLength);

			EventDispatcher::getInstance().dispatch(EVT_SCANNED_DEVICES, buffer, dataLength);

			if (Settings::getInstance().isEnabled(CONFIG_MESH_ENABLED)) {
				MeshControl::getInstance().sendScanMessage(results->getList()->list, results->getSize());
			}
		}

		break;
	}
	case CMD_SAMPLE_POWER: {
		LOGi("handle sample power command: tbd");

//		opcode_message_payload_t* payload = (opcode_message_payload_t*) buffer;
//		uint8_t sampleOp = payload->opCode;
//
//		if (sampleOp == 0) {
//			PowerSampling::getInstance().stopSampling();
//		} else {
//			PowerSampling::getInstance().startSampling();
//		}
		break;
	}
	case CMD_FACTORY_RESET: {
		LOGi("handle factory reset command");

		if (size != sizeof(FACTORY_RESET_CODE)) {
			LOGe("expected reset code of length %d", sizeof(FACTORY_RESET_CODE));
			return;
		}

		factory_reset_message_payload_t* payload = (factory_reset_message_payload_t*) buffer;
		uint32_t resetCode = payload->resetCode;

		if (resetCode == FACTORY_RESET_CODE) {
			LOGf("factory reset");

			Settings::getInstance().factoryReset(resetCode);
			State::getInstance().factoryReset(resetCode);
			// todo: might not be neccessary if we only use dm in setup mode we can handle it specifically
			//   there. maybe with a mode factory reset
			// todo: remove stack again from CommandHandler if we don't need it here
			_stack->device_manager_reset();

			LOGi("factory reset done, rebooting device in 2s ...");

			static uint32_t resetOp = 1;

			app_timer_id_t resetTimer;
			Timer::getInstance().createSingleShot(resetTimer, (app_timer_timeout_handler_t) reset);
			Timer::getInstance().start(resetTimer, MS_TO_TICKS(2000), &resetOp);

		} else {
			LOGi("wrong code received: %p", resetCode);
			LOGi("factory reset code is: %p", FACTORY_RESET_CODE);
		}

		break;
	}
	case CMD_SCHEDULE_ENTRY: {
		LOGi("handle schedule entry command");

#if SCHEDULER_ENABLED==1
		Scheduler& scheduler = Scheduler::getInstance();

		ScheduleEntry entry;

		if (entry.assign(buffer, size)) {
			return;
		}
		schedule_entry_t* entryStruct = entry.getStruct();
		if (!entryStruct->nextTimestamp) {
			scheduler.removeScheduleEntry(entryStruct);
		}
		else {

			// Check if entry is correct
			if (entryStruct->nextTimestamp < scheduler.getTime()) {
				return;
			}
			switch (ScheduleEntry::getTimeType(entryStruct)) {
			case SCHEDULE_TIME_TYPE_REPEAT:
				if (entryStruct->repeat.repeatTime == 0) {
					return;
				}
				break;
			case SCHEDULE_TIME_TYPE_DAILY:
				if (entryStruct->daily.nextDayOfWeek > 6) {
					return;
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

	case CMD_USER_FEEDBACK: {
		LOGi("handle user feedback command: tbd");

		// todo: tbd
		break;
	}
	case CMD_SET_TIME: {
		LOGi("handle set time command: tbd");

		// todo: tbd
		break;
	}
	case CMD_KEEP_ALIVE_STATE: {
		LOGi("handle keep alive command: tbd");

		// todo: tbd
		break;
	}
	case CMD_KEEP_ALIVE: {
		LOGi("handle keep alive command: tbd");

		// todo: tbd
		break;
	}
	default: {
		LOGe("command type not found!!");
	}
	}
}
