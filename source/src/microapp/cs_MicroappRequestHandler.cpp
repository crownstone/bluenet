/**
 * Microapp command handler.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: March 16, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Stack.h>
#include <cfg/cs_Boards.h>
#include <common/cs_Types.h>
#include <cs_MicroappStructs.h>
#include <drivers/cs_Gpio.h>
#include <events/cs_Event.h>
#include <ipc/cs_IpcRamData.h>
#include <logging/cs_Logger.h>
#include <microapp/cs_MicroappController.h>
#include <microapp/cs_MicroappInterruptHandler.h>
#include <microapp/cs_MicroappRequestHandler.h>
#include <microapp/cs_MicroappSdkUtil.h>
#include <protocol/cs_CommandTypes.h>
#include <protocol/cs_ErrorCodes.h>
#include <protocol/cs_Packets.h>
#include <storage/cs_State.h>

#define LogMicroappRequestHandlerDebug LOGvv

/*
 * Forwards requests from the microapp to the relevant handler
 */
cs_ret_code_t MicroappRequestHandler::handleMicroappRequest(microapp_sdk_header_t* header) {
	LogMicroappRequestHandlerDebug("handleMicroappRequest: [messageType %u, ack %i]", header->messageType, header->ack);
	uint8_t type = header->messageType;
	switch (type) {
		case CS_MICROAPP_SDK_TYPE_NONE: {
			// This should not be used
			LOGw("Microapp yields without setting messageType");
			break;
		}
		case CS_MICROAPP_SDK_TYPE_LOG: {
			microapp_sdk_log_header_t* log = reinterpret_cast<microapp_sdk_log_header_t*>(header);
			return handleRequestLog(log);
		}
		case CS_MICROAPP_SDK_TYPE_PIN: {
			microapp_sdk_pin_t* pin = reinterpret_cast<microapp_sdk_pin_t*>(header);
			return handleRequestPin(pin);
		}
		case CS_MICROAPP_SDK_TYPE_SWITCH: {
			microapp_sdk_switch_t* switchRequest = reinterpret_cast<microapp_sdk_switch_t*>(header);
			return handleRequestSwitch(switchRequest);
		}
		case CS_MICROAPP_SDK_TYPE_SERVICE_DATA: {
			microapp_sdk_service_data_t* serviceData = reinterpret_cast<microapp_sdk_service_data_t*>(header);
			return handleRequestServiceData(serviceData);
		}
		case CS_MICROAPP_SDK_TYPE_TWI: {
			microapp_sdk_twi_t* twi = reinterpret_cast<microapp_sdk_twi_t*>(header);
			return handleRequestTwi(twi);
		}
		case CS_MICROAPP_SDK_TYPE_BLE: {
			microapp_sdk_ble_t* ble = reinterpret_cast<microapp_sdk_ble_t*>(header);
			return handleRequestBle(ble);
		}
		case CS_MICROAPP_SDK_TYPE_MESH: {
			microapp_sdk_mesh_t* mesh = reinterpret_cast<microapp_sdk_mesh_t*>(header);
			return handleRequestMesh(mesh);
		}
		case CS_MICROAPP_SDK_TYPE_POWER_USAGE: {
			microapp_sdk_power_usage_t* powerUsage = reinterpret_cast<microapp_sdk_power_usage_t*>(header);
			return handleRequestPowerUsage(powerUsage);
		}
		case CS_MICROAPP_SDK_TYPE_PRESENCE: {
			microapp_sdk_presence_t* presence = reinterpret_cast<microapp_sdk_presence_t*>(header);
			return handleRequestPresence(presence);
		}
		case CS_MICROAPP_SDK_TYPE_CONTROL_COMMAND: {
			microapp_sdk_control_command_t* controlCommand = reinterpret_cast<microapp_sdk_control_command_t*>(header);
			return handleRequestControlCommand(controlCommand);
		}
		case CS_MICROAPP_SDK_TYPE_YIELD: {
			microapp_sdk_yield_t* yield = reinterpret_cast<microapp_sdk_yield_t*>(header);
			return handleRequestYield(yield);
		}
		default: {
			_log(SERIAL_INFO, true, "Unknown command %u", type);
			// set ack field so microapp will know something went wrong
			header->ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
			return ERR_UNKNOWN_TYPE;
		}
	}
	return ERR_SUCCESS;
}

// TODO: establish a proper default log level for microapps
#define LOCAL_MICROAPP_LOG_LEVEL SERIAL_INFO

cs_ret_code_t MicroappRequestHandler::handleRequestLog(microapp_sdk_log_header_t* log) {
	__attribute__((unused)) bool newLine = false;
	if (log->flags & CS_MICROAPP_SDK_LOG_FLAG_NEWLINE) {
		newLine = true;
	}
	if (log->size == 0) {
		_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "");
		log->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
		return ERR_SUCCESS;
	}
	switch (log->type) {
		case CS_MICROAPP_SDK_LOG_CHAR: {
			[[maybe_unused]] microapp_sdk_log_char_t* logChar = reinterpret_cast<microapp_sdk_log_char_t*>(log);
			[[maybe_unused]] uint32_t val                     = logChar->value;
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i", val);
			break;
		}
		case CS_MICROAPP_SDK_LOG_SHORT: {
			[[maybe_unused]] microapp_sdk_log_short_t* logShort = reinterpret_cast<microapp_sdk_log_short_t*>(log);
			[[maybe_unused]] uint32_t val                       = logShort->value;
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i", val);
			break;
		}
		case CS_MICROAPP_SDK_LOG_UINT: {
			[[maybe_unused]] microapp_sdk_log_uint_t* logUint = reinterpret_cast<microapp_sdk_log_uint_t*>(log);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%u", logUint->value);
			break;
		}
		case CS_MICROAPP_SDK_LOG_INT: {
			[[maybe_unused]] microapp_sdk_log_int_t* logInt = reinterpret_cast<microapp_sdk_log_int_t*>(log);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i", logInt->value);
			break;
		}
		case CS_MICROAPP_SDK_LOG_FLOAT: {
			[[maybe_unused]] microapp_sdk_log_float_t* logFloat = reinterpret_cast<microapp_sdk_log_float_t*>(log);
			[[maybe_unused]] int32_t val                        = logFloat->value;
			[[maybe_unused]] int32_t decimal = abs(static_cast<int>(logFloat->value * 1000.0) % 1000);
			// We automatically cast to int because printf of floats is disabled due to size limitations
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i.%03i", val, decimal);
			break;
		}
		case CS_MICROAPP_SDK_LOG_DOUBLE: {
			[[maybe_unused]] microapp_sdk_log_double_t* logDouble = reinterpret_cast<microapp_sdk_log_double_t*>(log);
			[[maybe_unused]] int32_t val                          = logDouble->value;
			[[maybe_unused]] int32_t decimal = abs(static_cast<int>(logDouble->value * 1000.0) % 1000);
			// We automatically cast to int because printf of floats is disabled due to size limitations
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i.%03i", val);
			break;
		}
		case CS_MICROAPP_SDK_LOG_STR: {
			[[maybe_unused]] microapp_sdk_log_string_t* logString = reinterpret_cast<microapp_sdk_log_string_t*>(log);
			// Enforce a zero-byte at the end before we log
			uint8_t zeroByteIndex                                 = MICROAPP_SDK_MAX_STRING_LENGTH - 1;
			if (log->size < zeroByteIndex) {
				zeroByteIndex = log->size;
			}
			logString->str[zeroByteIndex] = 0;
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%s", logString->str);
			break;
		}
		case CS_MICROAPP_SDK_LOG_ARR: {
			[[maybe_unused]] microapp_sdk_log_array_t* logArray = reinterpret_cast<microapp_sdk_log_array_t*>(log);
			if (log->size >= MICROAPP_SDK_MAX_ARRAY_SIZE) {
				// Truncate, but don't send an error
				log->size = MICROAPP_SDK_MAX_ARRAY_SIZE;
			}
			_logArray(LOCAL_MICROAPP_LOG_LEVEL, newLine, reinterpret_cast<int8_t*>(&logArray->arr[0]), log->size);
			break;
		}
		default: {
			LOGi("Unsupported microapp log type: %u", log->type);
			log->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
			return ERR_UNKNOWN_TYPE;
		}
	}
	log->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappRequestHandler::handleRequestPin(microapp_sdk_pin_t* pin) {
	MicroappSdkPin pinIndex = (MicroappSdkPin)pin->pin;
	LogMicroappRequestHandlerDebug("handleMicroappPinRequest: [pin %i, type %i]", pinIndex, pin->type);
	if (pinIndex > GPIO_INDEX_COUNT + BUTTON_COUNT + LED_COUNT) {
		LOGi("Pin %i out of range", pinIndex);
		pin->header.ack = CS_MICROAPP_SDK_ACK_ERR_OUT_OF_RANGE;
		return ERR_NOT_FOUND;
	}
	MicroappSdkPinType type = (MicroappSdkPinType)pin->type;
	switch (type) {
		case CS_MICROAPP_SDK_PIN_INIT: {
			// Initializing a pin
			MicroappSdkPinDirection direction = (MicroappSdkPinDirection)pin->direction;
			MicroappSdkPinPolarity polarity   = (MicroappSdkPinPolarity)pin->polarity;

			TYPIFY(EVT_GPIO_INIT) gpio;
			gpio.pinIndex = MicroappSdkUtil::interruptToDigitalPin(pinIndex);
			gpio.pull     = (direction == CS_MICROAPP_SDK_PIN_INPUT_PULLUP) ? 1 : 0;
			LogMicroappRequestHandlerDebug(
					"Initializing GPIO pin %u with direction %u and polarity %u", gpio.pinIndex, direction, polarity);

			switch (direction) {
				case CS_MICROAPP_SDK_PIN_INPUT:
				case CS_MICROAPP_SDK_PIN_INPUT_PULLUP: {
					// Initializing a pin as input
					switch (polarity) {
						case CS_MICROAPP_SDK_PIN_NO_POLARITY: {
							gpio.direction = INPUT;
							gpio.polarity  = NONE;
							break;
						}
						case CS_MICROAPP_SDK_PIN_RISING: {
							gpio.direction = SENSE;
							gpio.polarity  = LOTOHI;
							break;
						}
						case CS_MICROAPP_SDK_PIN_FALLING: {
							gpio.direction = SENSE;
							gpio.polarity  = HITOLO;
							break;
						}
						case CS_MICROAPP_SDK_PIN_CHANGE: {
							gpio.direction = SENSE;
							gpio.polarity  = TOGGLE;
							break;
						}
						default: {
							LOGw("Unknown pin polarity: %u", polarity);
							pin->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
							return ERR_UNKNOWN_TYPE;
						}
					}
					break;
				}
				case CS_MICROAPP_SDK_PIN_OUTPUT: {
					// Initializing a pin as output
					gpio.direction = OUTPUT;
					break;
				}
				default: {
					LOGw("Unknown pin direction: %u", direction);
					pin->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
					return ERR_UNKNOWN_TYPE;
				}
			}
			event_t event(CS_TYPE::EVT_GPIO_INIT, &gpio, sizeof(gpio));
			event.dispatch();

			if (gpio.direction == SENSE) {
				MicroappController& controller = MicroappController::getInstance();
				cs_ret_code_t result           = controller.registerSoftInterrupt(CS_MICROAPP_SDK_TYPE_PIN, pinIndex);
				if (result != ERR_SUCCESS) {
					// Either already registered or no space
					pin->header.ack = CS_MICROAPP_SDK_ACK_ERROR;
					return ERR_UNSPECIFIED;
				}
			}
			break;
		}
		case CS_MICROAPP_SDK_PIN_ACTION: {
			// Performing a read or write on a pin
			MicroappSdkPinActionType action = (MicroappSdkPinActionType)pin->action;
			switch (action) {
				case CS_MICROAPP_SDK_PIN_READ: {
					LogMicroappControllerDebug("CS_MICROAPP_SDK_PIN_READ pin=%u", pin->pin);

					TYPIFY(EVT_GPIO_READ) gpio;

					// TODO: use result buffer for this.
					uint8_t buf[1];

					gpio.pinIndex = MicroappSdkUtil::interruptToDigitalPin(pinIndex);
					gpio.length = sizeof(buf);
					gpio.buf = buf;

					event_t event(CS_TYPE::EVT_GPIO_READ, &gpio, sizeof(gpio));
					event.dispatch();

					pin->value = buf[0];
					pin->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(event.result.returnCode);
					break;
				}
				case CS_MICROAPP_SDK_PIN_WRITE: {
					// Write to a pin
					TYPIFY(EVT_GPIO_WRITE) gpio;
					gpio.pinIndex             = MicroappSdkUtil::interruptToDigitalPin(pinIndex);
					MicroappSdkPinValue value = (MicroappSdkPinValue)pin->value;
					switch (value) {
						case CS_MICROAPP_SDK_PIN_ON: {
							LogMicroappRequestHandlerDebug("Setting GPIO pin %i", gpio.pinIndex);
							gpio.length = 1;
							uint8_t buf[1];
							buf[0]   = 1;
							gpio.buf = buf;
							event_t event(CS_TYPE::EVT_GPIO_WRITE, &gpio, sizeof(gpio));
							event.dispatch();
							break;
						}
						case CS_MICROAPP_SDK_PIN_OFF: {
							LogMicroappRequestHandlerDebug("Clearing GPIO pin %i", gpio.pinIndex);
							gpio.length = 1;
							uint8_t buf[1];
							buf[0]   = 0;
							gpio.buf = buf;
							event_t event(CS_TYPE::EVT_GPIO_WRITE, &gpio, sizeof(gpio));
							event.dispatch();
							break;
						}
						default: {
							LOGw("Unknown pin value: %u", value);
							pin->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
							return ERR_UNKNOWN_TYPE;
						}
					}
					break;
				}
				default: {
					LOGw("Unknown pin action: %u", action);
					pin->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
					return ERR_UNKNOWN_TYPE;
				}
			}
			break;
		}
		default: {
			LOGw("Unknown pin request type");
			pin->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
			return ERR_UNKNOWN_TYPE;
		}
	}
	pin->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappRequestHandler::handleRequestSwitch(microapp_sdk_switch_t* switchRequest) {
	MicroappSdkSwitchValue value = (MicroappSdkSwitchValue)switchRequest->value;
	LogMicroappRequestHandlerDebug("handleMicroappSwitchRequest: [value %i]", value);
	TYPIFY(CMD_SWITCH) switchCommand;
	switchCommand.switchCmd = value;
	cmd_source_with_counter_t source(CS_CMD_SOURCE_MICROAPP);
	event_t event(CS_TYPE::CMD_SWITCH, &switchCommand, sizeof(switchCommand), source);
	event.dispatch();
	switchRequest->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappRequestHandler::handleRequestServiceData(microapp_sdk_service_data_t* serviceData) {
	LogMicroappRequestHandlerDebug(
			"handleMicroappServiceDataRequest: [uuid %i, size %i]", serviceData->appUuid, serviceData->size);
	if (serviceData->size > MICROAPP_SDK_MAX_SERVICE_DATA_LENGTH) {
		LOGi("Payload size too large");
		serviceData->header.ack = CS_MICROAPP_SDK_ACK_ERR_TOO_LARGE;
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	TYPIFY(CMD_MICROAPP_ADVERTISE) eventData;
	eventData.version   = 0;  // TODO: define somewhere.
	eventData.type      = 0;  // TODO: define somewhere.
	eventData.appUuid   = serviceData->appUuid;
	eventData.data.len  = serviceData->size;
	eventData.data.data = serviceData->data;
	event_t event(CS_TYPE::CMD_MICROAPP_ADVERTISE, &eventData, sizeof(eventData));
	event.dispatch();
	serviceData->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappRequestHandler::handleRequestTwi(microapp_sdk_twi_t* twi) {
	MicroappSdkTwiType type = (MicroappSdkTwiType)twi->type;
	LogMicroappRequestHandlerDebug("handleMicroappTwiRequest: [type %i]", type);
	switch (type) {
		case CS_MICROAPP_SDK_TWI_INIT: {
			LogMicroappRequestHandlerDebug("Init i2c");
			TYPIFY(EVT_TWI_INIT) twiInit;
			// no need to write twi.config (is not under control of microapp)
			event_t event(CS_TYPE::EVT_TWI_INIT, &twiInit, sizeof(twiInit));
			event.dispatch();
			break;
		}
		case CS_MICROAPP_SDK_TWI_WRITE: {
			LogMicroappRequestHandlerDebug("Write over i2c to address: 0x%02x", twi->address);
			TYPIFY(EVT_TWI_WRITE) twiWrite;
			twiWrite.address = twi->address;
			twiWrite.buf     = twi->buf;
			twiWrite.length  = twi->size;
			twiWrite.stop    = (twi->flags & CS_MICROAPP_SDK_TWI_FLAG_STOP);
			event_t event(CS_TYPE::EVT_TWI_WRITE, &twiWrite, sizeof(twiWrite));
			event.dispatch();
			break;
		}
		case CS_MICROAPP_SDK_TWI_READ: {
			LogMicroappRequestHandlerDebug("Read from i2c address: 0x%02x", twi->address);
			// Create a synchronous event to retrieve twi data
			TYPIFY(EVT_TWI_READ) twiRead;
			twiRead.address = twi->address;
			twiRead.buf     = twi->buf;
			twiRead.length  = twi->size;
			twiRead.stop    = (twi->flags & CS_MICROAPP_SDK_TWI_FLAG_STOP);
			event_t event(CS_TYPE::EVT_TWI_READ, &twiRead, sizeof(twiRead));
			event.dispatch();

			// Get data back and prepare for microapp
			twi->header.ack = event.result.returnCode;
			twi->size       = twiRead.length;
			break;
		}
		default: {
			LOGw("Unknown TWI type: %i", type);
			twi->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
			return ERR_UNKNOWN_TYPE;
		}
	}
	twi->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappRequestHandler::handleRequestBle(microapp_sdk_ble_t* ble) {
	MicroappSdkBleType type = (MicroappSdkBleType)ble->type;
	LogMicroappRequestHandlerDebug("handleMicroappBleRequest: [type %i]", type);

#if BUILD_MESHING == 0
	if (type == CS_MICROAPP_SDK_BLE_SCAN) {
		LOGw("Scanning is done within the mesh code. No scans will be received because mesh is disabled");
		ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_DISABLED;
		return ERR_NOT_AVAILABLE;
	}
#endif

	switch (type) {
		case CS_MICROAPP_SDK_BLE_UUID_REGISTER: {
			LOGi("BLE UUID register");
			ble_uuid128_t uuid128;
			memcpy(uuid128.uuid128, ble->requestUuidRegister.customUuid, sizeof(uuid128.uuid128));
			UUID uuid;
			cs_ret_code_t result          = uuid.fromFullUuid(uuid128);
			ble->requestUuidRegister.uuid = MicroappSdkUtil::convertUuid(uuid);
			ble->header.ack               = MicroappSdkUtil::bluenetResultToMicroapp(result);
			return result;
		}
		case CS_MICROAPP_SDK_BLE_SCAN: {
			return handleRequestBleScan(ble);
		}
		case CS_MICROAPP_SDK_BLE_CENTRAL: {
			return handleRequestBleCentral(ble);
		}
		case CS_MICROAPP_SDK_BLE_PERIPHERAL: {
			return handleRequestBlePeripheral(ble);
		}
		default: {
			LOGi("Unknown BLE type: %u", type);
			ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
			return ERR_UNKNOWN_TYPE;
		}
	}
}

cs_ret_code_t MicroappRequestHandler::handleRequestBleScan(microapp_sdk_ble_t* ble) {
	switch (ble->scan.type) {
		case CS_MICROAPP_SDK_BLE_SCAN_REGISTER_INTERRUPT: {
			MicroappController& controller = MicroappController::getInstance();
			cs_ret_code_t result = controller.registerSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_SCAN);

			ble->header.ack      = MicroappSdkUtil::bluenetResultToMicroapp(result);
			return result;
		}
		case CS_MICROAPP_SDK_BLE_SCAN_START: {
			LOGv("Start scanning");
			MicroappController::getInstance().microappData.isScanning = true;
			ble->header.ack                                           = CS_MICROAPP_SDK_ACK_SUCCESS;
			return ERR_SUCCESS;
		}
		case CS_MICROAPP_SDK_BLE_SCAN_STOP: {
			LOGv("Stop scanning");
			MicroappController::getInstance().microappData.isScanning = false;
			ble->header.ack                                           = CS_MICROAPP_SDK_ACK_SUCCESS;
			return ERR_SUCCESS;
		}
		default: {
			LOGi("Unknown BLE scan type: %u", ble->scan.type);
			ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
			return ERR_UNKNOWN_TYPE;
		}
	}
}

cs_ret_code_t MicroappRequestHandler::handleRequestBleCentral(microapp_sdk_ble_t* ble) {
	switch (ble->central.type) {
		case CS_MICROAPP_SDK_BLE_CENTRAL_REGISTER_INTERRUPT: {
			MicroappController& controller = MicroappController::getInstance();
			cs_ret_code_t result =
					controller.registerSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_CENTRAL);

			ble->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(result);
			return result;
		}
		case CS_MICROAPP_SDK_BLE_CENTRAL_REQUEST_CONNECT: {
			LOGi("BLE central connect");
			TYPIFY(CMD_BLE_CENTRAL_CONNECT) bleConnectCommand;
			// TODO: don't reverse
			std::reverse_copy(
					ble->central.requestConnect.address.address,
					ble->central.requestConnect.address.address + MAC_ADDRESS_LENGTH,
					bleConnectCommand.address.address);
			event_t event(CS_TYPE::CMD_BLE_CENTRAL_CONNECT, &bleConnectCommand, sizeof(bleConnectCommand));
			event.dispatch();

			ble->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(event.result.returnCode);
			return event.result.returnCode;
		}
		case CS_MICROAPP_SDK_BLE_CENTRAL_REQUEST_DISCONNECT: {
			LOGi("BLE central disconnect");
			// Later we should check the connection handle.

			event_t event(CS_TYPE::CMD_BLE_CENTRAL_DISCONNECT);
			event.dispatch();

			ble->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(event.result.returnCode);
			return event.result.returnCode;
		}
		default: {
			LOGi("Unknown BLE central type: %u", ble->central.type);
			ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
			return ERR_UNKNOWN_TYPE;
		}
	}
}

cs_ret_code_t MicroappRequestHandler::handleRequestBlePeripheral(microapp_sdk_ble_t* ble) {
	switch (ble->peripheral.type) {
		case CS_MICROAPP_SDK_BLE_PERIPHERAL_REQUEST_REGISTER_INTERRUPT: {
			MicroappController& controller = MicroappController::getInstance();
			cs_ret_code_t result =
					controller.registerSoftInterrupt(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_PERIPHERAL);

			ble->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(result);
			return result;
		}
		case CS_MICROAPP_SDK_BLE_PERIPHERAL_REQUEST_ADD_SERVICE: {
			if (MicroappController::getInstance().microappData.service != nullptr) {
				ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_NO_SPACE;
				return ERR_NO_SPACE;
			}
			UUID uuid = MicroappSdkUtil::convertUuid(ble->peripheral.requestAddService.uuid);
			MicroappController::getInstance().microappData.service = new MicroappService(uuid);
			Stack::getInstance().addService(MicroappController::getInstance().microappData.service);
			MicroappController::getInstance().microappData.service->init(&Stack::getInstance());

			ble->peripheral.handle = MicroappController::getInstance().microappData.service->getHandle();
			ble->header.ack        = CS_MICROAPP_SDK_ACK_SUCCESS;
			return ERR_SUCCESS;
		}
		case CS_MICROAPP_SDK_BLE_PERIPHERAL_REQUEST_ADD_CHARACTERISTIC: {
			Service* service = MicroappController::getInstance().microappData.service;
			if (service == nullptr) {
				ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_NOT_FOUND;
				return ERR_WRONG_STATE;
			}

			if (ble->peripheral.requestAddCharacteristic.buffer == nullptr) {
				ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
				return ERR_BUFFER_UNASSIGNED;
			}

			if (ble->peripheral.requestAddCharacteristic.bufferSize == 0) {
				ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
				return ERR_BUFFER_TOO_SMALL;
			}

			if (ble->peripheral.requestAddCharacteristic.serviceHandle != service->getHandle()) {
				ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
				return ERR_WRONG_PARAMETER;
			}

			// Make sure to perform all checks before creating the characteristic on the heap.
			Characteristic<buffer_ptr_t>* characteristic = new Characteristic<buffer_ptr_t>();
			if (characteristic == nullptr) {
				return ERR_NO_SPACE;
			}

			characteristic_config_t config = {
					.read  = ble->peripheral.requestAddCharacteristic.options.read,
					.write = ble->peripheral.requestAddCharacteristic.options.write
							 || ble->peripheral.requestAddCharacteristic.options.writeNoResponse,
					.notify = ble->peripheral.requestAddCharacteristic.options.notify
							  || ble->peripheral.requestAddCharacteristic.options.indicate,
					.autoNotify          = ble->peripheral.requestAddCharacteristic.options.autoNotify,
					.notificationChunker = false,
					.encrypted           = false,
			};

			characteristic->setUuid(ble->peripheral.requestAddCharacteristic.uuid.uuid);
			characteristic->setName("microapp");
			characteristic->setConfig(config);
			characteristic->setValueBuffer(
					ble->peripheral.requestAddCharacteristic.buffer,
					ble->peripheral.requestAddCharacteristic.bufferSize);

			characteristic->setEventHandler(
					[&](CharacteristicEventType eventType,
						CharacteristicBase* characteristic,
						const EncryptionAccessLevel accessLevel) -> void {
						uint16_t connectionHandle = Stack::getInstance().getConnectionHandle();
						uint16_t handle           = characteristic->getValueHandle();
						switch (eventType) {
							case CHARACTERISTIC_EVENT_WRITE: {
								cs_data_t value = characteristic->getValue();
								MicroappInterruptHandler::getInstance().onBlePeripheralWrite(
										connectionHandle, handle, value);
								break;
							}
							case CHARACTERISTIC_EVENT_SUBSCRIPTION: {
								bool subscribed = characteristic->isSubscribedForNotifications();
								MicroappInterruptHandler::getInstance().onBlePeripheralSubscription(
										connectionHandle, handle, subscribed);
								break;
							}
							case CHARACTERISTIC_EVENT_NOTIFY_DONE: {
								MicroappInterruptHandler::getInstance().onBlePeripheralNotififyDone(
										connectionHandle, handle);
								break;
							}
							default: {
								break;
							}
						}
					});

			service->addCharacteristic(characteristic);

			ble->peripheral.handle = characteristic->getValueHandle();
			ble->header.ack        = CS_MICROAPP_SDK_ACK_SUCCESS;
			return ERR_SUCCESS;
		}
		case CS_MICROAPP_SDK_BLE_PERIPHERAL_REQUEST_DISCONNECT: {
			Stack::getInstance().disconnect();
			ble->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
			return ERR_SUCCESS;
		}
		case CS_MICROAPP_SDK_BLE_PERIPHERAL_REQUEST_VALUE_SET: {
			CharacteristicBase* characteristic = getCharacteristic(ble->peripheral.handle);
			if (characteristic == nullptr) {
				ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_NOT_FOUND;
				return ERR_WRONG_STATE;
			}

			cs_ret_code_t result = characteristic->updateValue(ble->peripheral.requestValueSet.size);
			ble->header.ack      = MicroappSdkUtil::bluenetResultToMicroapp(result);
			return result;
		}
		case CS_MICROAPP_SDK_BLE_PERIPHERAL_REQUEST_NOTIFY: {
			CharacteristicBase* characteristic = getCharacteristic(ble->peripheral.handle);
			if (characteristic == nullptr) {
				ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_NOT_FOUND;
				return ERR_WRONG_STATE;
			}

			// Right now, we ignore offset and size.
			cs_ret_code_t result =
					characteristic->notify(ble->peripheral.requestNotify.offset, ble->peripheral.requestNotify.size);
			ble->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(result);
			return result;
		}
		default: {
			LOGi("Unknown BLE peripheral type: %u", ble->peripheral.type);
			ble->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
			return ERR_UNKNOWN_TYPE;
		}
	}
}

CharacteristicBase* MicroappRequestHandler::getCharacteristic(uint16_t handle) {
	Service* service = MicroappController::getInstance().microappData.service;
	if (service == nullptr) {
		return nullptr;
	}
	for (CharacteristicBase* characteristic : service->getCharacteristics()) {
		if (characteristic->getValueHandle() == handle) {
			return characteristic;
		}
	}
	return nullptr;
}

cs_ret_code_t MicroappRequestHandler::handleRequestMesh(microapp_sdk_mesh_t* mesh) {

#if BUILD_MESHING == 0
	LOGw("Mesh is disabled. Mesh-related microapp requests are ignored.");
	mesh->header.ack = CS_MICROAPP_SDK_ACK_ERR_DISABLED;
	return ERR_NOT_AVAILABLE;
#endif

	MicroappSdkMeshType type = static_cast<MicroappSdkMeshType>(mesh->type);
	LogMicroappRequestHandlerDebug("handleMicroappMeshRequest: [type %i]", type);
	switch (type) {
		case CS_MICROAPP_SDK_MESH_SEND: {
			if (mesh->size == 0) {
				LOGi("No message");
				mesh->header.ack = CS_MICROAPP_SDK_ACK_ERR_EMPTY;
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

			if (mesh->size > MAX_MICROAPP_MESH_PAYLOAD_SIZE) {
				LOGi("Message too large: %u > %u", mesh->size, MAX_MICROAPP_MESH_PAYLOAD_SIZE);
				mesh->header.ack = CS_MICROAPP_SDK_ACK_ERR_TOO_LARGE;
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

			TYPIFY(CMD_SEND_MESH_MSG) eventData;
			bool broadcast = (mesh->stoneId == 0);
			if (!broadcast) {
				LogMicroappRequestHandlerDebug("Send mesh message to %i", mesh->stoneId);
				eventData.idCount   = 1;
				eventData.targetIds = &(mesh->stoneId);
			}
			else {
				LogMicroappRequestHandlerDebug("Broadcast mesh message");
			}
			eventData.flags.flags.broadcast   = broadcast;
			eventData.flags.flags.acked       = !broadcast;
			eventData.flags.flags.useKnownIds = false;
			eventData.flags.flags.doNotRelay  = false;
			eventData.type                    = CS_MESH_MODEL_TYPE_MICROAPP;
			eventData.payload                 = mesh->data;
			eventData.size                    = mesh->size;
			event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &eventData, sizeof(eventData));
			event.dispatch();

			mesh->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(event.result.returnCode);
			return event.result.returnCode;
		}
		case CS_MICROAPP_SDK_MESH_LISTEN: {
			LOGi("Start listening for microapp mesh messages");
			MicroappController& controller = MicroappController::getInstance();
			cs_ret_code_t result =
					controller.registerSoftInterrupt(CS_MICROAPP_SDK_TYPE_MESH, CS_MICROAPP_SDK_MESH_READ);

			mesh->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(result);
			return result;
		}
		case CS_MICROAPP_SDK_MESH_READ_CONFIG: {
			LogMicroappRequestHandlerDebug("Microapp requesting mesh info");
			TYPIFY(CONFIG_CROWNSTONE_ID) id;
			State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &id, sizeof(id));
			mesh->stoneId    = id;
			mesh->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
			return ERR_SUCCESS;
		}
		default: {
			LOGi("Unknown mesh type: %u", type);
			mesh->header.ack = CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
			return ERR_UNKNOWN_TYPE;
		}
	}
}

cs_ret_code_t MicroappRequestHandler::handleRequestPowerUsage(microapp_sdk_power_usage_t* powerUsage) {
	TYPIFY(STATE_POWER_USAGE) powerUsageState;
	State::getInstance().get(CS_TYPE::STATE_POWER_USAGE, &powerUsageState, sizeof(powerUsageState));
	powerUsage->powerUsage = powerUsageState;
	powerUsage->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappRequestHandler::handleRequestPresence(microapp_sdk_presence_t* presence) {
	if (presence->profileId >= MAX_NUMBER_OF_PRESENCE_PROFILES) {
		LOGi("Incorrect profileId");
		presence->header.ack = CS_MICROAPP_SDK_ACK_ERR_OUT_OF_RANGE;
		return ERR_NOT_FOUND;
	}

	presence_t resultBuf;
	event_t event(CS_TYPE::CMD_GET_PRESENCE);
	event.result.buf = cs_data_t(reinterpret_cast<uint8_t*>(&resultBuf), sizeof(resultBuf));
	event.dispatch();

	if (event.result.returnCode != ERR_SUCCESS) {
		LOGi("No success, result code: %u", event.result.returnCode);
		presence->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(event.result.returnCode);
		return event.result.returnCode;
	}
	if (event.result.dataSize != sizeof(presence_t)) {
		LOGi("Result is of size %u, expected size %u", event.result.dataSize, sizeof(presence_t));
		presence->header.ack = CS_MICROAPP_SDK_ACK_ERROR;
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	presence->presenceBitmask = resultBuf.presence[presence->profileId];
	presence->header.ack      = CS_MICROAPP_SDK_ACK_SUCCESS;
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappRequestHandler::handleRequestControlCommand(microapp_sdk_control_command_t* controlCommand) {
	if (controlCommand->size == 0) {
		LOGi("No control command");
		controlCommand->header.ack = CS_MICROAPP_SDK_ACK_ERR_EMPTY;
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	if (controlCommand->size > MICROAPP_SDK_MAX_CONTROL_COMMAND_PAYLOAD_SIZE) {
		LOGi("Control command too large: %u > %u", controlCommand->size, MICROAPP_SDK_MAX_CONTROL_COMMAND_PAYLOAD_SIZE);
		controlCommand->header.ack = CS_MICROAPP_SDK_ACK_ERR_TOO_LARGE;
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	LogMicroappRequestHandlerDebug("Dispatching control command of type %u", controlCommand->type);
	TYPIFY(CMD_CONTROL_CMD) eventData;
	eventData.protocolVersion = controlCommand->protocol;
	eventData.data            = controlCommand->payload;
	eventData.size            = controlCommand->size;
	eventData.type            = static_cast<CommandHandlerTypes>(controlCommand->type);
	eventData.accessLevel     = EncryptionAccessLevel::MEMBER;
	cmd_source_with_counter_t source(CS_CMD_SOURCE_MICROAPP);
	event_t event(CS_TYPE::CMD_CONTROL_CMD, &eventData, sizeof(eventData), source);
	event.dispatch();

	controlCommand->header.ack = MicroappSdkUtil::bluenetResultToMicroapp(event.result.returnCode);
	return event.result.returnCode;
}

cs_ret_code_t MicroappRequestHandler::handleRequestYield(microapp_sdk_yield_t* yield) {
	LogMicroappRequestHandlerDebug(
			"handleMicroappYieldRequest: [type %u, emptySlots %u]", yield->type, yield->emptyInterruptSlots);
	// Update number of empty interrupt slots the microapp has
	MicroappController& controller = MicroappController::getInstance();
	controller.setEmptySoftInterruptSlots(yield->emptyInterruptSlots);
	yield->header.ack = CS_MICROAPP_SDK_ACK_SUCCESS;
	return ERR_SUCCESS;
}
