/**
 * Microapp command handler.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: March 16, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_UUID.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_Boards.h>
#include <common/cs_Types.h>
#include <cs_MicroappStructs.h>
#include <drivers/cs_Gpio.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <ipc/cs_IpcRamData.h>
#include <logging/cs_Logger.h>
#include <microapp/cs_MicroappCommandHandler.h>
#include <microapp/cs_MicroappController.h>
#include <microapp/cs_MicroappStorage.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Hash.h>
#include <util/cs_Utils.h>

/*
 * Forwards commands from the microapp to the relevant handler
 */
cs_ret_code_t MicroappCommandHandler::handleMicroappCommand(microapp_cmd_t* cmd) {
	uint8_t command = cmd->cmd;
	switch (command) {
		case CS_MICROAPP_COMMAND_LOG: {
			LOGd("Microapp log command");
			microapp_log_cmd_t* log_cmd = reinterpret_cast<microapp_log_cmd_t*>(cmd);
			return handleMicroappLogCommand(log_cmd);
		}
		case CS_MICROAPP_COMMAND_DELAY: {
			LOGd("Microapp delay command");
			microapp_delay_cmd_t* delay_cmd = reinterpret_cast<microapp_delay_cmd_t*>(cmd);
			return handleMicroappDelayCommand(delay_cmd);
		}
		case CS_MICROAPP_COMMAND_PIN: {
			LOGd("Microapp pin command");
			microapp_pin_cmd_t* pin_cmd = reinterpret_cast<microapp_pin_cmd_t*>(cmd);
			return handleMicroappPinCommand(pin_cmd);
		}
		case CS_MICROAPP_COMMAND_SWITCH_DIMMER: {
			LOGd("Microapp switch dimmer command");
			microapp_dimmer_switch_cmd_t* dimmer_switch_cmd = reinterpret_cast<microapp_dimmer_switch_cmd_t*>(cmd);
			return handleMicroappDimmerSwitchCommand(dimmer_switch_cmd);
		}
		case CS_MICROAPP_COMMAND_SERVICE_DATA: {
			LOGd("Microapp service data command");
			microapp_service_data_cmd_t* service_data_cmd = reinterpret_cast<microapp_service_data_cmd_t*>(cmd);
			return handleMicroappServiceDataCommand(service_data_cmd);
		}
		case CS_MICROAPP_COMMAND_TWI: {
			LOGd("Microapp TWI command");
			microapp_twi_cmd_t* twi_cmd = reinterpret_cast<microapp_twi_cmd_t*>(cmd);
			return handleMicroappTwiCommand(twi_cmd);
		}
		case CS_MICROAPP_COMMAND_BLE: {
			LOGd("Microapp BLE command");
			microapp_ble_cmd_t* ble_cmd = reinterpret_cast<microapp_ble_cmd_t*>(cmd);
			return handleMicroappBleCommand(ble_cmd);
		}
		case CS_MICROAPP_COMMAND_POWER_USAGE: {
			LOGd("Microapp power usage command");
			microapp_power_usage_cmd_t* power_usage_cmd = reinterpret_cast<microapp_power_usage_cmd_t*>(cmd);
			return handleMicroappPowerUsageCommand(power_usage_cmd);
		}
		case CS_MICROAPP_COMMAND_PRESENCE: {
			LOGd("Microapp presence command");
			microapp_presence_cmd_t* presence_cmd = reinterpret_cast<microapp_presence_cmd_t*>(cmd);
			return handleMicroappPresenceCommand(presence_cmd);
		}
		case CS_MICROAPP_COMMAND_MESH: {
			LOGd("Microapp mesh command");
			microapp_mesh_cmd_t* mesh_cmd = reinterpret_cast<microapp_mesh_cmd_t*>(cmd);
			return handleMicroappMeshCommand(mesh_cmd);
		}
		case CS_MICROAPP_COMMAND_SOFT_INTERRUPT_RECEIVED: {
			microapp_soft_interrupt_cmd_t* soft_interrupt_cmd = reinterpret_cast<microapp_soft_interrupt_cmd_t*>(cmd);
			uint8_t emptyInterruptSlots = soft_interrupt_cmd->emptyInterruptSlots;
			MicroappController& controller = MicroappController::getInstance();
			controller.setEmptySoftInterrupts(emptyInterruptSlots);
			LOGd("Soft interrupt received for %i [slots=%i]", (int)cmd->id, emptyInterruptSlots);
			break;
		}
		case CS_MICROAPP_COMMAND_SOFT_INTERRUPT_DROPPED: {
			microapp_soft_interrupt_cmd_t* soft_interrupt_cmd = reinterpret_cast<microapp_soft_interrupt_cmd_t*>(cmd);
			uint8_t emptyInterruptSlots = soft_interrupt_cmd->emptyInterruptSlots;
			MicroappController& controller = MicroappController::getInstance();
			controller.setEmptySoftInterrupts(emptyInterruptSlots);
			LOGd("Soft interrupt dropped for %i [slots=%i]", (int)cmd->id, emptyInterruptSlots);
			break;
		}
		case CS_MICROAPP_COMMAND_SOFT_INTERRUPT_ERROR: {
			// Empty slots cannot be used, it is old info from the interrupt's starting state
			LOGd("Soft interrupt error for %i", (int)cmd->id);
			break;
		}
		case CS_MICROAPP_COMMAND_SOFT_INTERRUPT_END: {
			// Empty slots cannot be used, it is old info from the interrupt's starting state
			LOGd("Soft interrupt end for %i", (int)cmd->id);
			break;
		}
		case CS_MICROAPP_COMMAND_SETUP_END: {
			LOGi("Setup end");
			break;
		}
		case CS_MICROAPP_COMMAND_LOOP_END: {
			// Only in debug mode
			LOGv("Loop end");
			break;
		}
		case CS_MICROAPP_COMMAND_NONE: {
			// ignore, nothing written
			break;
		}
		default: {
			_log(SERIAL_INFO, true, "Unknown command %u", command);
			return ERR_UNKNOWN_TYPE;
		}
	}
	return ERR_SUCCESS;
}

// TODO: establish a proper default log level for microapps
#define LOCAL_MICROAPP_LOG_LEVEL SERIAL_INFO

cs_ret_code_t MicroappCommandHandler::handleMicroappLogCommand(microapp_log_cmd_t* command) {

	__attribute__((unused)) bool newLine = false;
	if (command->option == CS_MICROAPP_COMMAND_LOG_NEWLINE) {
		newLine = true;
	}
	if (command->length == 0) {
		_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%s", "");
		return ERR_SUCCESS;
	}
	switch (command->type) {
		case CS_MICROAPP_COMMAND_LOG_CHAR: {
			[[maybe_unused]] microapp_log_char_cmd_t* cmd = reinterpret_cast<microapp_log_char_cmd_t*>(command);
			[[maybe_unused]] uint32_t val                 = cmd->value;
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i%s", val);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_SHORT: {
			[[maybe_unused]] microapp_log_short_cmd_t* cmd = reinterpret_cast<microapp_log_short_cmd_t*>(command);
			[[maybe_unused]] uint32_t val                  = cmd->value;
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i%s", val);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_UINT: {
			[[maybe_unused]] microapp_log_uint_cmd_t* cmd = reinterpret_cast<microapp_log_uint_cmd_t*>(command);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%u%s", cmd->value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_INT: {
			[[maybe_unused]] microapp_log_int_cmd_t* cmd = reinterpret_cast<microapp_log_int_cmd_t*>(command);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i%s", cmd->value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_FLOAT: {
			[[maybe_unused]] microapp_log_float_cmd_t* cmd = reinterpret_cast<microapp_log_float_cmd_t*>(command);
			[[maybe_unused]] uint32_t val                  = cmd->value;
			// We automatically cast to int because printf of floats is disabled due to size limitations
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i (cast to int) %s", val);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_DOUBLE: {
			[[maybe_unused]] microapp_log_double_cmd_t* cmd = reinterpret_cast<microapp_log_double_cmd_t*>(command);
			[[maybe_unused]] uint32_t val                   = cmd->value;
			// We automatically cast to int because printf of floats is disabled due to size limitations
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i (cast to int) %s", val);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_STR: {
			[[maybe_unused]] microapp_log_string_cmd_t* cmd = reinterpret_cast<microapp_log_string_cmd_t*>(command);
			// Enforce a zero-byte at the end before we log
			uint8_t zeroByteIndex = MAX_MICROAPP_STRING_LENGTH - 1;
			if (command->length < zeroByteIndex) {
				zeroByteIndex = command->length;
			}
			cmd->str[zeroByteIndex] = 0;
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%s", cmd->str);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_ARR: {
			[[maybe_unused]] microapp_log_array_cmd_t* cmd = reinterpret_cast<microapp_log_array_cmd_t*>(command);
			if (command->length >= MAX_MICROAPP_ARRAY_LENGTH) {
				// Truncate, but don't send an error
				command->length = MAX_MICROAPP_ARRAY_LENGTH;
			}
			for (uint8_t i = 0; i < command->length; ++i) {
				_log(LOCAL_MICROAPP_LOG_LEVEL, false, "0x%x ", (int)cmd->arr[i]);
			}
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "");
			break;
		}
		default: {
			LOGi("Unsupported microapp log type: %u", command->type);
			return ERR_UNKNOWN_TYPE;
		}
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappCommandHandler::handleMicroappDelayCommand(microapp_delay_cmd_t* delay_cmd) {
	// don't do anything
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappCommandHandler::handleMicroappPinCommand(microapp_pin_cmd_t* pin_cmd) {
	CommandMicroappPin pin = (CommandMicroappPin)pin_cmd->pin;
	if (pin > GPIO_INDEX_COUNT + BUTTON_COUNT + LED_COUNT) {
		LOGw("Pin %i out of range", pin); return ERR_UNKNOWN_TYPE;
	}
	CommandMicroappPinOpcode1 opcode1 = (CommandMicroappPinOpcode1)pin_cmd->opcode1;
	switch (opcode1) {
		case CS_MICROAPP_COMMAND_PIN_MODE: return handleMicroappPinSetModeCommand(pin_cmd);
		case CS_MICROAPP_COMMAND_PIN_ACTION: return handleMicroappPinActionCommand(pin_cmd);
		default: LOGw("Unknown opcode1"); return ERR_UNKNOWN_OP_CODE;
	}
}

cs_ret_code_t MicroappCommandHandler::handleMicroappPinSetModeCommand(microapp_pin_cmd_t* pin_cmd) {
	CommandMicroappPin pin = (CommandMicroappPin)pin_cmd->pin;
	TYPIFY(EVT_GPIO_INIT) gpio;
	gpio.pin_index                    = pin;
	gpio.pull                         = 0;
	CommandMicroappPinOpcode2 opcode2 = (CommandMicroappPinOpcode2)pin_cmd->opcode2;
	LOGi("Set mode %i for virtual pin %i", opcode2, pin);
	switch (opcode2) {
		case CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP:
			gpio.pull = 1;
			[[ fallthrough ]];
		case CS_MICROAPP_COMMAND_PIN_READ: {
			CommandMicroappPinValue val = (CommandMicroappPinValue)pin_cmd->value;
			switch (val) {
				case CS_MICROAPP_COMMAND_VALUE_RISING:
					gpio.direction = SENSE;
					gpio.polarity  = LOTOHI;
					break;
				case CS_MICROAPP_COMMAND_VALUE_FALLING:
					gpio.direction = SENSE;
					gpio.polarity  = HITOLO;
					break;
				case CS_MICROAPP_COMMAND_VALUE_CHANGE:
					gpio.direction = SENSE;
					gpio.polarity  = TOGGLE;
					break;
				default:
					gpio.direction = INPUT;
					gpio.polarity  = NONE;
					break;
			}
			event_t event(CS_TYPE::EVT_GPIO_INIT, &gpio, sizeof(gpio));
			EventDispatcher::getInstance().dispatch(event);
			break;
		}
		case CS_MICROAPP_COMMAND_PIN_WRITE: {
			gpio.direction = OUTPUT;
			event_t event(CS_TYPE::EVT_GPIO_INIT, &gpio, sizeof(gpio));
			EventDispatcher::getInstance().dispatch(event);
			break;
		}
		default: LOGw("Unknown opcode2: %i", pin_cmd->opcode2); return ERR_UNKNOWN_OP_CODE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappCommandHandler::handleMicroappPinActionCommand(microapp_pin_cmd_t* pin_cmd) {
	CommandMicroappPin pin = (CommandMicroappPin)pin_cmd->pin;
	LOGd("Read or write pin %i", pin);
	CommandMicroappPinOpcode2 opcode2 = (CommandMicroappPinOpcode2)pin_cmd->opcode2;
	switch (opcode2) {
		case CS_MICROAPP_COMMAND_PIN_WRITE: {
			TYPIFY(EVT_GPIO_WRITE) gpio;
			gpio.pin_index              = pin;
			CommandMicroappPinValue val = (CommandMicroappPinValue)pin_cmd->value;
			switch (val) {
				case CS_MICROAPP_COMMAND_VALUE_OFF: {
					LOGd("Clear GPIO pin");
					gpio.length = 1;
					uint8_t buf[1];
					buf[0]   = 0;
					gpio.buf = buf;
					event_t event(CS_TYPE::EVT_GPIO_WRITE, &gpio, sizeof(gpio));
					EventDispatcher::getInstance().dispatch(event);
					break;
				}
				case CS_MICROAPP_COMMAND_VALUE_ON: {
					LOGd("Set GPIO pin");
					gpio.length = 1;
					uint8_t buf[1];
					buf[0]   = 1;
					gpio.buf = buf;
					event_t event(CS_TYPE::EVT_GPIO_WRITE, &gpio, sizeof(gpio));
					EventDispatcher::getInstance().dispatch(event);
					break;
				}
				default: LOGw("Unknown microapp pin value command"); return ERR_UNKNOWN_TYPE;
			}
			break;
		}
		case CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP: // undefined, can only be used with opcode1 CS_MICROAPP_COMMAND_PIN_MODE
			LOGw("Input pullup undefined as a pin action command"); return ERR_UNSPECIFIED;
		case CS_MICROAPP_COMMAND_PIN_READ: {
			// TODO; (note that we do not handle event handler registration here but in SetMode above
			LOGw("Reading pins via the microapp is not implemented yet"); return ERR_NOT_IMPLEMENTED;
		}
		default: LOGw("Unknown opcode2: %i", pin_cmd->opcode2); return ERR_UNKNOWN_OP_CODE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappCommandHandler::handleMicroappDimmerSwitchCommand(microapp_dimmer_switch_cmd_t* dim_switch_cmd) {
	CommandMicroappDimmerSwitchOpcode opcode = (CommandMicroappDimmerSwitchOpcode)dim_switch_cmd->opcode;
	switch (opcode) {
		case CS_MICROAPP_COMMAND_SWITCH: {
			CommandMicroappSwitchValue val = (CommandMicroappSwitchValue)dim_switch_cmd->value;
			switch (val) {
				case CS_MICROAPP_COMMAND_SWITCH_OFF: {
					LOGi("Turn switch off");
					event_t event(CS_TYPE::CMD_SWITCH_OFF);
					EventDispatcher::getInstance().dispatch(event);
					break;
				}
				case CS_MICROAPP_COMMAND_SWITCH_ON: {
					LOGi("Turn switch on");
					event_t event(CS_TYPE::CMD_SWITCH_ON);
					EventDispatcher::getInstance().dispatch(event);
					break;
				}
				case CS_MICROAPP_COMMAND_SWITCH_TOGGLE: {
					LOGi("Toggle switch");
					event_t event(CS_TYPE::CMD_SWITCH_TOGGLE);
					EventDispatcher::getInstance().dispatch(event);
					break;
				}
				default: LOGw("Unknown switch command"); return ERR_UNKNOWN_TYPE;
			}
			break;
		}
		case CS_MICROAPP_COMMAND_DIMMER: {
			LOGi("Set dimmer to %i", dim_switch_cmd->value);
			TYPIFY(CMD_SET_DIMMER) eventData;
			eventData = dim_switch_cmd->value;
			event_t event(CS_TYPE::CMD_SET_DIMMER, &eventData, sizeof(eventData));
			event.dispatch();
			break;
		}
		default: LOGw("Unknown opcode: %i", opcode); return ERR_UNKNOWN_OP_CODE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappCommandHandler::handleMicroappServiceDataCommand(microapp_service_data_cmd_t* cmd) {

	if (cmd->body.dlen > MAX_COMMAND_SERVICE_DATA_LENGTH) {
		LOGi("Payload size incorrect");
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	TYPIFY(CMD_MICROAPP_ADVERTISE) eventData;
	eventData.version   = 0;  // TODO: define somewhere.
	eventData.type      = 0;  // TODO: define somewhere.
	eventData.appUuid   = cmd->body.appUuid;
	eventData.data.len  = cmd->body.dlen;
	eventData.data.data = cmd->body.data;
	event_t event(CS_TYPE::CMD_MICROAPP_ADVERTISE, &eventData, sizeof(eventData));
	event.dispatch();
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappCommandHandler::handleMicroappTwiCommand(microapp_twi_cmd_t* twi_cmd) {
	CommandMicroappTwiOpcode opcode = (CommandMicroappTwiOpcode)twi_cmd->opcode;
	switch (opcode) {
		case CS_MICROAPP_COMMAND_TWI_INIT: {
			LOGi("Init i2c");
			TYPIFY(EVT_TWI_INIT) twi;
			// no need to write twi.config (is not under control of microapp)
			event_t event(CS_TYPE::EVT_TWI_INIT, &twi, sizeof(twi));
			EventDispatcher::getInstance().dispatch(event);
			break;
		}
		case CS_MICROAPP_COMMAND_TWI_WRITE: {
			LOGd("Write over i2c to address: 0x%02x", twi_cmd->address);
			uint8_t bufSize = twi_cmd->length;
			TYPIFY(EVT_TWI_WRITE) twi;
			twi.address = twi_cmd->address;
			twi.buf     = twi_cmd->buf;
			twi.length  = bufSize;
			twi.stop    = twi_cmd->stop;
			event_t event(CS_TYPE::EVT_TWI_WRITE, &twi, sizeof(twi));
			EventDispatcher::getInstance().dispatch(event);
			break;
		}
		case CS_MICROAPP_COMMAND_TWI_READ: {
			LOGd("Read from i2c address: 0x%02x", twi_cmd->address);

			// Create a synchronous event to retrieve twi data
			uint8_t bufSize = twi_cmd->length;
			TYPIFY(EVT_TWI_READ) twi;
			twi.address = twi_cmd->address;
			twi.buf     = twi_cmd->buf;
			twi.length  = bufSize;
			twi.stop    = twi_cmd->stop;
			event_t event(CS_TYPE::EVT_TWI_READ, &twi, sizeof(twi));
			EventDispatcher::getInstance().dispatch(event);

			// Get data back and prepare for microapp
			twi_cmd->header.ack = event.result.returnCode;
			twi_cmd->length     = twi.length;
			break;
		}
		default: LOGw("Unknown i2c opcode: %i", twi_cmd->opcode); return ERR_UNKNOWN_OP_CODE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappCommandHandler::handleMicroappBleCommand(microapp_ble_cmd_t* ble_cmd) {
	switch (ble_cmd->opcode) {
		case CS_MICROAPP_COMMAND_BLE_SCAN_SET_HANDLER: {
			LOGi("Set scan event callback handler");
#if BUILD_MESHING == 0
			LOGi("Scanning is done within the mesh code. No scans will be received because mesh is disabled");
			// TODO: just return with ERR_NOT_AVAILABLE if nothing will happen anyway. Also for other scan_related commands
#endif
			// TYPIFY(EVT_MICROAPP_BLE_FILTER_INIT) ble;
			// ble.index = ble_cmd->id;
			// event_t event(CS_TYPE::EVT_MICROAPP_BLE_FILTER_INIT, &ble, sizeof(ble));
			// EventDispatcher::getInstance().dispatch(event);
			// bool success        = event.result.returnCode;
			// ble_cmd->header.ack = success;
			// return success ? ERR_SUCCESS : ERR_NO_SPACE;

			MicroappController& controller = MicroappController::getInstance();
			// what is the difference between the header id and the ble id?
			LOGi("Header id=%d, ble id=%d", ble_cmd->header.id, ble_cmd->id);
			controller.registerSoftInterruptSlotBle(ble_cmd->id);
			ble_cmd->header.ack = true;
			break;
		}
		case CS_MICROAPP_COMMAND_BLE_SCAN_START: {
			LOGi("Start scanning");
			MicroappController& controller = MicroappController::getInstance();
			controller.setScanning(true);
			ble_cmd->header.ack = true;
			break;
		}
		case CS_MICROAPP_COMMAND_BLE_SCAN_STOP: {
			LOGi("Stop scanning");
			MicroappController& controller = MicroappController::getInstance();
			controller.setScanning(false);
			ble_cmd->header.ack = true;
			break;
		}
		case CS_MICROAPP_COMMAND_BLE_CONNECT: {
			LOGi("Set up BLE connection");
			TYPIFY(CMD_BLE_CENTRAL_CONNECT) bleConnectCommand;
			std::reverse_copy(ble_cmd->addr, ble_cmd->addr + MAC_ADDRESS_LENGTH, bleConnectCommand.address.address);
			event_t event(CS_TYPE::CMD_BLE_CENTRAL_CONNECT, &bleConnectCommand, sizeof(bleConnectCommand));
			event.dispatch();
			ble_cmd->header.ack = true;
			LOGi("BLE command result: %u", event.result.returnCode);
			return event.result.returnCode;
		}
		default: {
			LOGi("Unknown microapp BLE command opcode: %u", ble_cmd->opcode);
			return ERR_UNKNOWN_OP_CODE;
		}
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappCommandHandler::handleMicroappPowerUsageCommand(microapp_power_usage_cmd_t* cmd) {
	microapp_power_usage_t* commandPayload = &cmd->powerUsage;

	TYPIFY(STATE_POWER_USAGE) powerUsage;
	State::getInstance().get(CS_TYPE::STATE_POWER_USAGE, &powerUsage, sizeof(powerUsage));

	commandPayload->powerUsage = powerUsage;

	return ERR_SUCCESS;
}

/*
 * This queries for a presence event.
 */
cs_ret_code_t MicroappCommandHandler::handleMicroappPresenceCommand(microapp_presence_cmd_t* cmd) {

	microapp_presence_t* microappPresence = &cmd->presence;
	if (microappPresence->profileId >= MAX_NUMBER_OF_PRESENCE_PROFILES) {
		LOGi("Incorrect profileId");
		return ERR_UNSAFE;
	}

	// Obtains presence array
	presence_t resultBuf;
	event_t event(CS_TYPE::CMD_GET_PRESENCE);
	event.result.buf = cs_data_t(reinterpret_cast<uint8_t*>(&resultBuf), sizeof(resultBuf));
	event.dispatch();
	if (event.result.returnCode != ERR_SUCCESS) {
		LOGi("No success, result code: %u", event.result.returnCode);
		return event.result.returnCode;
	}
	if (event.result.dataSize != sizeof(presence_t)) {
		LOGi("Result is of size %u expected size %u", event.result.dataSize, sizeof(presence_t));
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	microappPresence->presenceBitmask = resultBuf.presence[microappPresence->profileId];
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappCommandHandler::handleMicroappMeshCommand(microapp_mesh_cmd_t* command) {
#if BUILD_MESHING == 0
	LOGi("Mesh is not built in bluenet, so mesh-related microapp functionalities are disabled.");
	return ERR_NOT_AVAILABLE;
#endif
	switch (command->opcode) {
		case CS_MICROAPP_COMMAND_MESH_SEND: {
			auto cmd = reinterpret_cast<microapp_mesh_send_cmd_t*>(command);

			if (cmd->dlen == 0) {
				LOGi("No message.");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

			if (cmd->dlen > MICROAPP_MAX_MESH_MESSAGE_SIZE) {
				LOGi("Message too large: %u > %u", cmd->dlen, MICROAPP_MAX_MESH_MESSAGE_SIZE);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

			TYPIFY(CMD_SEND_MESH_MSG) eventData;
			bool broadcast = (cmd->stoneId == 0);
			if (!broadcast) {
				LOGv("Send mesh message to %i", cmd->stoneId);
				eventData.idCount   = 1;
				eventData.targetIds = &(cmd->stoneId);
			} else {
				LOGv("Broadcast mesh message");
			}
			eventData.flags.flags.broadcast   = broadcast;
			eventData.flags.flags.reliable    = !broadcast;
			eventData.flags.flags.useKnownIds = false;
			eventData.flags.flags.noHops      = false;
			eventData.type                    = CS_MESH_MODEL_TYPE_MICROAPP;
			eventData.payload                 = cmd->data;
			eventData.size                    = cmd->dlen;
			event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &eventData, sizeof(eventData));
			event.dispatch();
			if (event.result.returnCode != ERR_SUCCESS) {
				LOGi("Failed to send mesh message, return code: %u", event.result.returnCode);
				return event.result.returnCode;
			}
			break;
		}
		// case CS_MICROAPP_COMMAND_MESH_READ_AVAILABLE: {
		// 	auto cmd = reinterpret_cast<microapp_mesh_read_available_cmd_t*>(command);

		// 	if (!_meshMessageBuffer.isInitialized()) {
		// 		_meshMessageBuffer.init();
		// 	}

		// 	// TODO: This assumes nothing will overwrite the buffer
		// 	cmd->available = !_meshMessageBuffer.empty();
		// 	if (cmd->available) {
		// 		LOGv("Available mesh messages");
		// 	} else {
		// 		LOGv("No mesh messages available");
		// 	}

		// 	// TODO: One might want to call callMicroapp here (at least once).
		// 	// That would benefit from an ack "the other way around" (so microapp knows "available" is updated).
		// 	break;
		// }
		// case CS_MICROAPP_COMMAND_MESH_READ: {
		// 	auto cmd = reinterpret_cast<microapp_mesh_read_cmd_t*>(command);

		// 	if (!_meshMessageBuffer.isInitialized()) {
		// 		_meshMessageBuffer.init();
		// 	}
		// 	if (_meshMessageBuffer.empty()) {
		// 		LOGi("No message in buffer");
		// 		return ERR_WRONG_PAYLOAD_LENGTH;
		// 	}

		// 	auto message = _meshMessageBuffer.pop();
		// 	LOGv("Pop message for microapp");

		// 	// TODO: This assumes nothing will overwrite the buffer
		// 	cmd->stoneId = message.stoneId;
		// 	cmd->dlen    = message.messageSize;
		// 	if (message.messageSize > MICROAPP_MAX_MESH_MESSAGE_SIZE) {
		// 		LOGi("Message with wrong size in buffer");
		// 		return ERR_WRONG_PAYLOAD_LENGTH;
		// 	}
		// 	memcpy(cmd->data, message.message, message.messageSize);

		// 	// TODO: One might want to call callMicroapp here (at least once).
		// 	// That would benefit from an ack "the other way around" (so microapp knows "available" is updated).
		// 	break;
		// }
		case CS_MICROAPP_COMMAND_MESH_READ_SET_HANDLER: {
			MicroappController& controller = MicroappController::getInstance();
			command->header.ack = controller.registerSoftInterruptSlotMesh(command->header.id);
			break;
		}
		default: {
			LOGi("Unknown microapp mesh command opcode: %u", command->opcode);
			return ERR_UNKNOWN_OP_CODE;
		}
	}
	return ERR_SUCCESS;
}

/*
 * Called upon receiving mesh message, places mesh message in a buffer on the bluenet side.
 *
 * TODO: This should actually be implemented differently.
 */

// void MicroappCommandHandler::onMeshMessage(MeshMsgEvent event) {

// 	if (event.type != CS_MESH_MODEL_TYPE_MICROAPP) {
// 		LOGd("Mesh message received, but not for microapp");
// 		return;
// 	}
// 	if (_meshMessageBuffer.full()) {
// 		LOGi("Dropping message, buffer is full");
// 		return;
// 	}

// 	if (event.msg.len > MICROAPP_MAX_MESH_MESSAGE_SIZE) {
// 		LOGi("Message is too large: %u", event.msg.len);
// 		return;
// 	}

// 	LOGd("Mesh message received, store in buffer");
// 	microapp_buffered_mesh_message_t bufferedMessage;
// 	bufferedMessage.stoneId     = event.srcAddress;
// 	bufferedMessage.messageSize = event.msg.len;
// 	memcpy(bufferedMessage.message, event.msg.data, event.msg.len);
// 	_meshMessageBuffer.push(bufferedMessage);
// }

/**
 * Listen to events from bluenet in which the implement buffers those events and their contents on the bluenet side.
 *
 * TODO: It might be better to use an interrupt and store the buffer on the microapp side. Compare the implementation
 * of EVT_RECV_MESH_MSG with EVT_DEVICE_SCANNED (in cs_MicroappCommandHandler.cpp).
 */
// void MicroappCommandHandler::handleEvent(event_t& event) {
// 	switch (event.type) {
// 		case CS_TYPE::EVT_RECV_MESH_MSG: {
// 			auto msg = CS_TYPE_CAST(EVT_RECV_MESH_MSG, event.data);
// 			onMeshMessage(*msg);
// 			break;
// 		}
// 		default: break;
// 	}
// }
