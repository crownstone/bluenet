/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 18, 2016
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Nordic.h>

#include <common/cs_Types.h>

#include <drivers/cs_Serial.h>

#include <processing/cs_Switch.h>
#include <processing/cs_PowerSampling.h>
#include <processing/cs_Scanner.h>

enum CommandHandlerTypes {
	CMD_SWITCH,
	CMD_PWM,
	CMD_SET_TIME,
	CMD_GOTO_DFU,
	CMD_FACTORY_RESET,
	CMD_KEEP_ALIVE_STATE,
	CMD_KEEP_ALIVE,
	CMD_ENABLE_MESH,
	CMD_ENABLE_ENCRYPTION,
	CMD_ENABLE_IBEACON,
	CMD_ENABLE_CONT_POWER_MEASURE,
	CMD_ENABLE_SCANNER,
	CMD_SCAN_DEVICES,
	CMD_SAMPLE_POWER,
	CMD_USER_FEEDBACK,
	CMD_RESET
};

#define COMMAND_HEADER_SIZE 2

/** Command message
 */
struct __attribute__((__packed__)) command_message_t {
	uint16_t commandType;
	uint8_t params[500 - COMMAND_HEADER_SIZE];
};

struct __attribute__((__packed__)) switch_message_payload_t {
	uint8_t switchState;
};

struct __attribute__((__packed__)) reset_message_payload_t {
	uint8_t resetOp;
};

struct __attribute__((__packed__)) sample_message_payload_t {
	uint8_t sampleOp;
};

struct __attribute__((__packed__)) enable_message_payload_t {
	bool enable;
};

class CommandHandler {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static CommandHandler& getInstance() {
		static CommandHandler instance;
		return instance;
	}

//	void handleCommand(buffer_ptr_t buffer, uint16_t size) {
//
//		command_message_t* msg = (command_message_t*) buffer;
//		switch (msg->commandType) {
//		case CMD_SWITCH: {
//			switch_message_payload_t* payload = (switch_message_payload_t*)msg->params;
//			uint32_t value = payload->switchState;
//
//			uint32_t current = PWM::getInstance().getValue(0);
//			LOGi("1 current pwm: %d", current);
//			if (value != current) {
//				LOGi("2 set pwm to %i", value);
//				PWM::getInstance().setValue(0, value);
//
//				StateVars::getInstance().setStateVar(SV_SWITCH_STATE, value);
//			}
//
//			break;
//		}
//		}
//
//	}

	static void reset(void* p_context) {

		uint32_t cmd = *(int32_t*)p_context;

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

	void handleCommand(CommandHandlerTypes type, buffer_ptr_t buffer, uint16_t size) {

		switch (type) {
		case CMD_PWM: {
			switch_message_payload_t* payload = (switch_message_payload_t*)buffer;
			uint8_t value = payload->switchState;

			uint8_t current = Switch::getInstance().getValue();
			LOGi("current pwm: %d", current);
			if (value != current) {
				LOGi("update pwm to %i", value);
				Switch::getInstance().setValue(value);
			}
			break;
		}
		case CMD_SWITCH: {
			switch_message_payload_t* payload = (switch_message_payload_t*)buffer;
			uint8_t value = payload->switchState;

			if (value == 0) {
				Switch::getInstance().relayOff();
			} else {
				Switch::getInstance().relayOn();
			}

			break;
		}
		case CMD_SET_TIME : {
			// todo: tbd
			break;
		}
		case CMD_GOTO_DFU : {

			static uint32_t resetOp = COMMAND_ENTER_RADIO_BOOTLOADER;

			app_timer_id_t resetTimer;
			Timer::getInstance().createSingleShot(resetTimer, (app_timer_timeout_handler_t)&CommandHandler::reset);
			Timer::getInstance().start(resetTimer, MS_TO_TICKS(100), &resetOp);
			break;
		}
		case CMD_RESET : {

			reset_message_payload_t* payload = (reset_message_payload_t*)buffer;
			static uint32_t resetOp = payload->resetOp;

			if (resetOp) {
				app_timer_id_t resetTimer;
				Timer::getInstance().createSingleShot(resetTimer, (app_timer_timeout_handler_t)&CommandHandler::reset);
				Timer::getInstance().start(resetTimer, MS_TO_TICKS(100), &resetOp);
			} else {
				LOGw("To reset write a nonzero value");
			}
			break;
		}
		case CMD_FACTORY_RESET : {
			// todo: tbd
			break;
		}
		case CMD_KEEP_ALIVE_STATE : {
			// todo: tbd
			break;
		}
		case CMD_KEEP_ALIVE : {
			// todo: tbd
			break;
		}
		case CMD_ENABLE_MESH : {
			// todo: tbd
			break;
		}
		case CMD_ENABLE_ENCRYPTION : {
			// todo: tbd
			break;
		}
		case CMD_ENABLE_IBEACON : {
			// todo: tbd
			break;
		}
		case CMD_ENABLE_CONT_POWER_MEASURE : {
			// todo: tbd
			break;
		}
		case CMD_ENABLE_SCANNER : {

			enable_message_payload_t* payload = (enable_message_payload_t*)buffer;
			bool enable = payload->enable;



			Settings::getInstance().updateFlag(CONFIG_SCANNER_ENABLED, enable, true);

			// todo: tbd
			break;
		}
		case CMD_SCAN_DEVICES : {

			enable_message_payload_t* payload = (enable_message_payload_t*)buffer;
			bool start = payload->enable;

			if (start) {
				Scanner::getInstance().manualStartScan();
			} else {
				// todo: if not tracking. do we need that still?
				Scanner::getInstance().manualStopScan();

				ScanResult* results = Scanner::getInstance().getResults();
				results->print();

				buffer_ptr_t buffer;
				uint16_t dataLength;
				results->getBuffer(buffer, dataLength);

				EventDispatcher::getInstance().dispatch(EVT_SCANNED_DEVICES, buffer, dataLength);

#if (CHAR_MESHING==1)
//				if (Settings::getInstance().isEnabled(CONFIG_MESH_ENABLED)) {
					MeshControl::getInstance().sendScanMessage(results->getList()->list, results->getSize());
//				}
#endif
			}

			break;
		}
		case CMD_SAMPLE_POWER : {

			sample_message_payload_t* payload = (sample_message_payload_t*)buffer;
			uint8_t sampleOp = payload->sampleOp;

			PowerSampling::getInstance().sampleCurrent(sampleOp);
			break;
		}
		case CMD_USER_FEEDBACK : {
			// todo: tbd
			break;
		}
		default: {
			LOGe("command type not found!!");
		}
		}
	}

private:

	CommandHandler() {

	}


};

