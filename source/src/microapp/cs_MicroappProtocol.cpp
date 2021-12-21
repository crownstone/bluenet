/**
 *
 * Microapp protocol.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: April 4, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "nrf_fstorage_sd.h"

#include <ble/cs_UUID.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Config.h>
#include <common/cs_Types.h>
#include <cs_MicroappStructs.h>
#include <drivers/cs_Gpio.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <ipc/cs_IpcRamData.h>
#include <logging/cs_Logger.h>
#include <microapp/cs_MicroappProtocol.h>
#include <microapp/cs_MicroappStorage.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Hash.h>
#include <util/cs_Utils.h>
#include <algorithm>

extern "C" {

/*
 * This function is called by the microapp in the context of its stack. Even though it is possible to call a log or
 * other bluenet function, this is strongly discouraged. In contrast, yield control towards bluenet explicitly by
 * calling yield() and continue in `MicroappProtocol::callMicroapp()` retrieving the command sent by the microapp.
 *
 * @param[in] payload                            pointer to buffer with command for bluenet
 * @param[in] length                             length of command (not total buffer size)
 */
cs_ret_code_t microapp_callback(uint8_t* payload, uint16_t length) {
	if (length == 0 || length > MAX_PAYLOAD) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	microapp_cmd_t* cmd  = (microapp_cmd_t*)payload;
	uintptr_t coargs_ptr = cmd->coargs;
	coargs_t* args       = (coargs_t*)coargs_ptr;
	args->payload        = payload;
	args->payloadSize    = length;
	yield(args->coroutine);

	return ERR_SUCCESS;
}

/*
 * Jump into the microapp (this function should be called as a coroutine). It obtains the very first instruction from
 * the coroutine arguments. It then considers that instruction to be a method without arguments and calls it. An
 * incorrectly written microapp might crash the firmware here. Henceforth, before this moment it must be written to
 * flash that we try to start the microapp. If we get a reboot and see the "try to start" state, we can disable the
 * microapp forever. Alternatively, if the function never yields, it will trip the watchdog. If the watchdog is
 * triggered, we might presume that a microapp was the reason for it and disable it.
 *
 * @param[in] void *p                            pointer to a coargs_t struct
 */
void goIntoMicroapp(void* p) {
	coargs_t* coargs       = (coargs_t*)p;
	void (*microappMain)() = (void (*)())coargs->entry;
	(*microappMain)();
}

}  // extern C

/*
 * Helper functions.
 *
 * TODO: Move to appropriate files.
 */

/*
 * Checks flash boundaries (for single microapp).
 */
cs_ret_code_t checkFlashBoundaries(uintptr_t address) {
	if (address < g_FLASH_MICROAPP_BASE) {
		return ERR_UNSAFE;
	}
	if (address > (g_FLASH_MICROAPP_BASE + g_FLASH_MICROAPP_BASE * 0x1000)) {
		return ERR_UNSAFE;
	}
	return ERR_SUCCESS;
}

MicroappProtocol::MicroappProtocol() : EventListener(), _meshMessageBuffer(MAX_MESH_MESSAGES_BUFFERED) {

	EventDispatcher::getInstance().addListener(this);

	_booted = false;

	for (int i = 0; i < MAX_PIN_ISR_COUNT; ++i) {
		_pinIsr[i].pin      = 0;
		_pinIsr[i].callback = 0;
	}
	for (int i = 0; i < MAX_BLE_ISR_COUNT; ++i) {
		_bleIsr[i].type     = 0;
		_bleIsr[i].callback = 0;
	}

	_callbackData       = nullptr;
	_microappIsScanning = false;

	_coargs = new coargs_t();
}

/*
 * Set the microapp_callback in the IPC ram data bank. At a later time it can be used by the microapp to find the
 * address of microapp_callback to call back into the bluenet code.
 */
void MicroappProtocol::setIpcRam() {
	LOGi("Set IPC info for microapp");
	bluenet2microapp_ipcdata_t data;
	data.protocol          = 1;
	data.length            = sizeof(bluenet2microapp_ipcdata_t);
	data.microapp_callback = (uintptr_t)&microapp_callback;
	data.coargs_ptr        = (uintptr_t)&_coargs;

	[[maybe_unused]] uint32_t retCode = setRamData(IPC_INDEX_CROWNSTONE_APP, (uint8_t*)&data, data.length);
	LOGi("retCode=%u", retCode);
}

/*
 * Get the ram structure in which the microapp has registered some of its data. We only check if they expect the same
 * protocol version as we do.
 */
uint16_t MicroappProtocol::interpretRamdata() {
	LOGi("Get IPC info for microapp");
	uint8_t buf[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
	uint8_t readSize = 0;
	uint8_t retCode  = getRamData(IPC_INDEX_MICROAPP, buf, BLUENET_IPC_RAM_DATA_ITEM_SIZE, &readSize);

	if (retCode != IPC_RET_SUCCESS) {
		LOGi("Nothing found in RAM ret_code=%u", retCode);
		return ERR_NOT_FOUND;
	}
	LOGi("Found RAM for Microapp");

	microapp2bluenet_ipcdata_t& data = *(microapp2bluenet_ipcdata_t*)buf;

	LOGi("  protocol: %02x", data.protocol);

	const int accepted_protocol_version = 1;
	if (data.protocol != accepted_protocol_version) {
		return ERR_PROTOCOL_UNSUPPORTED;
	}
	return ERR_SUCCESS;
}

/*
 * Gets the first instruction for the microapp (this is written in its header). We correct for thumb and check its
 * boundaries. Then we call it from a coroutine context and expect it to yield.
 *
 * @param[in]                                    appIndex (currently ignored)
 */
void MicroappProtocol::callApp(uint8_t appIndex) {
	// Make thumb mode explicit: will always be true
	static bool thumbMode = true;

	initMemory();

	uintptr_t address = MicroappStorage::getInstance().getStartInstructionAddress(appIndex);
	LOGi("Microapp: start at 0x%08X", address);

	if (thumbMode) {
		address += 1;
		LOGi("Set address to 0x%08X (thumb mode)", address);
	}

	if (checkFlashBoundaries(address) != ERR_SUCCESS) {
		LOGe("Address not within microapp flash boundaries");
		return;
	}

	_coargs->entry = address;

	// Write coroutine as argument in the struct so we can yield from it in the context of the microapp stack
	_coargs->coroutine = &_coroutine;
	start(&_coroutine, goIntoMicroapp, &_coargs);
}

/*
 * This should not be done. Call the microapp before main where all copying towards the right sections takes place.
 * That will be fine for both .bss (uninitialized) and .data (initialized) data. If this is done before the very
 * first call to the microapp, it won't hurt though.
 *
 * TODO: Check and remove this function.
 */
uint16_t MicroappProtocol::initMemory() {
	LOGi("Init memory: clear 0x%p to 0x%p", g_RAM_MICROAPP_BASE, g_RAM_MICROAPP_BASE + g_RAM_MICROAPP_AMOUNT);
	for (uint32_t i = 0; i < g_RAM_MICROAPP_AMOUNT; ++i) {
		uint32_t* const val = (uint32_t*)(uintptr_t)(g_RAM_MICROAPP_BASE + i);
		*val                = 0;
	}

	// The above is fine for .bss (which is uninitialized) but not for .data. It needs to be copied
	// to the right addresses.
	return ERR_SUCCESS;
}

/*
 * Called from cs_Microapp every time tick. Only when _booted flag is set, will we actually call the microapp.
 * The _booted flag is reset when the microapp data can not be interpreted right.
 *
 * TODO: _booted etc are only for single microapp
 */
void MicroappProtocol::tickMicroapp(uint8_t appIndex) {

	if (!_booted) {
		return;
	}

	if (!_loaded) {
		LOGi("Start loading");
		uint16_t ret_code = interpretRamdata();
		if (ret_code != ERR_SUCCESS) {
			LOGw("Disable microapp. After boot not the right info available. ret_code=%u", ret_code);
			_booted = false;
			return;
		}

		LOGi("Set loaded to true");
		_loaded = true;
	}

	if (_loaded) {
		callMicroapp();
		retrieveCommand();
	}
}

/*
 * Retrieve command from the microapp.
 */
void MicroappProtocol::retrieveCommand() {
	// get payload from data
	uint8_t* payload = _coargs->payload;
	uint16_t length  = _coargs->payloadSize;
	handleMicroappCommand(payload, length);
}

/*
 * We resume the previously started coroutine.
 */
void MicroappProtocol::callMicroapp() {
	if (!_loaded) {
		LOGi("Not loaded");
		return;
	}
	if (next(&_coroutine)) {
		return;
	}
	// Should only happen if microapp actually ends (and does not yield anymore).
	LOGi("End of coroutine. Should not happen.")
}

/**
 * Required if we have to pass events through to the microapp.
 */
void MicroappProtocol::handleEvent(event_t& event) {
	switch (event.type) {
		// Listen to GPIO events, will be used later to implement attachInterrupt in microapp.
		case CS_TYPE::EVT_GPIO_INIT: {
			LOGi("Register GPIO event handler for microapp");
			TYPIFY(EVT_GPIO_INIT) gpio = *(TYPIFY(EVT_GPIO_INIT)*)event.data;

			// we will register the handler in the class for first empty slot
			for (int i = 0; i < MAX_PIN_ISR_COUNT; ++i) {
				if (_pinIsr[i].callback == 0) {
					LOGi("Register callback %x or %i for pin %i", gpio.callback, gpio.callback, gpio.pin_index);
					_pinIsr[i].callback = gpio.callback;
					_pinIsr[i].pin      = gpio.pin_index;
					break;
				}
			}
			break;
		}
		case CS_TYPE::EVT_GPIO_UPDATE: {
			TYPIFY(EVT_GPIO_UPDATE) gpio = *(TYPIFY(EVT_GPIO_UPDATE)*)event.data;
			LOGi("Get GPIO update for pin %i", gpio.pin_index);

			uintptr_t callback = 0;
			// get callback and call
			for (int i = 0; i < MAX_PIN_ISR_COUNT; ++i) {
				if (_pinIsr[i].pin == gpio.pin_index) {
					callback = _pinIsr[i].callback;
					break;
				}
			}
			LOGi("Call %x", callback);

			if (callback == 0) {
				LOGi("Callback not yet registered");
				break;
			}
			// we have to do this through another coroutine perhaps (not the same one as loop!), for
			// now stay on this stack
			void (*callback_func)() = (void (*)())callback;
			LOGi("Call callback at 0x%x", callback);
			callback_func();
			break;
		}
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			auto dev = CS_TYPE_CAST(EVT_DEVICE_SCANNED, event.data);
			onDeviceScanned(dev);
			break;
		}
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto msg = CS_TYPE_CAST(EVT_RECV_MESH_MSG, event.data);
			onMeshMessage(*msg);
			break;
		}
		default: break;
	}
}

/*
 * Called upon receiving mesh message, places mesh message in buffer
 */
void MicroappProtocol::onMeshMessage(MeshMsgEvent event) {
	if (event.type != CS_MESH_MODEL_TYPE_MICROAPP) {
		return;
	}

	if (_meshMessageBuffer.full()) {
		LOGi("Dropping message, buffer is full");
		return;
	}

	if (event.msg.len > MICROAPP_MAX_MESH_MESSAGE_SIZE) {
		LOGi("Message is too large: %u", event.msg.len);
		return;
	}

	microapp_buffered_mesh_message_t bufferedMessage;
	bufferedMessage.stoneId     = event.srcAddress;
	bufferedMessage.messageSize = event.msg.len;
	memcpy(bufferedMessage.message, event.msg.data, event.msg.len);
	_meshMessageBuffer.push(bufferedMessage);
}

/*
 * Called upon receiving scanned BLE device, calls microapp callback if registered
 */
void MicroappProtocol::onDeviceScanned(scanned_device_t* dev) {
	if (!_microappIsScanning) {
		LOGv("Microapp not scanning, so not forwarding scanned device");
		return;
	}

	// copy scanned device info to microapp_ble_device_t struct
	microapp_ble_device_t ble_dev;
	ble_dev.addr_type = dev->addressType;
	// convert from little endian to big endian
	std::reverse_copy(dev->address, dev->address + MAC_ADDRESS_LENGTH, ble_dev.addr);
	ble_dev.rssi = dev->rssi;

	if (dev->dataSize > sizeof(ble_dev.data)) {
		LOGw("BLE advertisement data too large");
		return;
	}
	ble_dev.dlen = dev->dataSize;
	memcpy(ble_dev.data, dev->data, dev->dataSize);

	uint16_t type      = (uint16_t)CS_TYPE::EVT_DEVICE_SCANNED;
	uintptr_t callback = 0;
	// get callback
	for (int i = 0; i < MAX_BLE_ISR_COUNT; ++i) {
		if (_bleIsr[i].type == type) {
			callback = _bleIsr[i].callback;
			break;
		}
	}

	if (callback == 0) {
		LOGw("Callback not yet registered");
		return;
	}
	// call callback
	LOGd("Call callback at 0x%x", callback);
	// we have to do this through another coroutine perhaps (not the same one as loop!), for
	// now stay on this stack
	void (*callback_func)(microapp_ble_device_t) = (void (*)(microapp_ble_device_t))callback;
	callback_func(ble_dev);
}

/*
 * Forwards commands from the microapp to the relevant handler
 */
cs_ret_code_t MicroappProtocol::handleMicroappCommand(uint8_t* payload, uint16_t length) {
	_logArray(SERIAL_DEBUG, true, payload, length);
	uint8_t command = payload[0];
	switch (command) {
		case CS_MICROAPP_COMMAND_LOG: {
			LOGd("Microapp log command");
			return handleMicroappLogCommand(payload, length);
		}
		case CS_MICROAPP_COMMAND_DELAY: {
			LOGd("Microapp delay command");
			if (length != sizeof(microapp_delay_cmd_t)) {
				LOGi("Wrong payload length");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			microapp_delay_cmd_t* delay_cmd = reinterpret_cast<microapp_delay_cmd_t*>(payload);
			return handleMicroappDelayCommand(delay_cmd);
		}
		case CS_MICROAPP_COMMAND_PIN: {
			LOGd("Microapp pin command");
			microapp_pin_cmd_t* pin_cmd = reinterpret_cast<microapp_pin_cmd_t*>(payload);
			return handleMicroappPinCommand(pin_cmd);
		}
		case CS_MICROAPP_COMMAND_SERVICE_DATA: {
			LOGd("Microapp service data command");
			return handleMicroappServiceDataCommand(payload, length);
		}
		case CS_MICROAPP_COMMAND_TWI: {
			LOGd("Microapp TWI command");
			microapp_twi_cmd_t* twi_cmd = reinterpret_cast<microapp_twi_cmd_t*>(payload);
			return handleMicroappTwiCommand(twi_cmd);
		}
		case CS_MICROAPP_COMMAND_BLE: {
			LOGd("Microapp BLE command");
			if (length != sizeof(microapp_ble_cmd_t)) {
				LOGi("Wrong payload length");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			microapp_ble_cmd_t* ble_cmd = reinterpret_cast<microapp_ble_cmd_t*>(payload);
			return handleMicroappBleCommand(ble_cmd);
		}
		case CS_MICROAPP_COMMAND_POWER_USAGE: {
			LOGd("Microapp power usage command");
			return handleMicroappPowerUsageCommand(payload, length);
		}
		case CS_MICROAPP_COMMAND_PRESENCE: {
			LOGd("Microapp presence command");
			return handleMicroappPresenceCommand(payload, length);
		}
		case CS_MICROAPP_COMMAND_MESH: {
			LOGd("Microapp mesh command");
			if (length < sizeof(command) + sizeof(microapp_mesh_header_t)) {
				LOGi("Payload too small");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			microapp_mesh_header_t* meshCommand = reinterpret_cast<microapp_mesh_header_t*>(&(payload[1]));

			size_t headerSize        = sizeof(command) + sizeof(microapp_mesh_header_t);
			uint8_t* meshPayload     = payload + headerSize;
			size_t meshPayloadLength = length - headerSize;

			return handleMicroappMeshCommand(meshCommand, meshPayload, meshPayloadLength);
		}
		default: {
			_log(SERIAL_INFO, true, "Unknown command %u of length %u:", command, length);
			_logArray(SERIAL_INFO, true, payload, length);
			return ERR_UNKNOWN_TYPE;
		}
	}
}

cs_ret_code_t MicroappProtocol::handleMicroappLogCommand(uint8_t* payload, uint16_t length) {
	char type                            = payload[1];
	char option                          = payload[2];
	__attribute__((unused)) bool newLine = false;
	if (option == CS_MICROAPP_COMMAND_LOG_NEWLINE) {
		newLine = true;
	}
	switch (type) {
		case CS_MICROAPP_COMMAND_LOG_CHAR: {
			__attribute__((unused)) char value = payload[3];
			_log(SERIAL_INFO, newLine, "%i%s", (int)value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_SHORT: {
			__attribute__((unused)) uint16_t value = *(uint16_t*)&payload[3];
			_log(SERIAL_INFO, newLine, "%i%s", (int)value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_UINT:
		case CS_MICROAPP_COMMAND_LOG_INT: {
			__attribute__((unused)) int value = *(int*)&payload[3];
			_log(SERIAL_INFO, newLine, "%i%s", (int)value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_FLOAT: {
			// TODO: We get into trouble when printing using %f
			__attribute__((unused)) int value  = *(int*)&payload[3];
			__attribute__((unused)) int before = value / 10000;
			__attribute__((unused)) int after  = value - (before * 10000);
			_log(SERIAL_INFO, newLine, "%i.%04i", before, after);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_DOUBLE: {
			// TODO: This will fail (see float for workaround)
			__attribute__((unused)) double value = *(double*)&payload[3];
			_log(SERIAL_INFO, newLine, "%f%s", value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_STR: {
			__attribute__((unused)) int str_length =
					length - 3;  // Check if length <= max_length - 1, for null terminator.
			__attribute__((unused)) char* data = reinterpret_cast<char*>(&(payload[3]));
			data[str_length]                   = 0;
			_log(SERIAL_INFO, newLine, "%s", data);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_ARR: {
			// for now, print uint8_t array in hex format
			uint8_t* data = reinterpret_cast<uint8_t*>(&(payload[3]));
			int len       = length - 3;
			// convert to hex string using sprintf
			char buf[(MAX_PAYLOAD - 3) * 2 + 1];  // does not allow variable length
			char* dest   = buf;
			uint8_t* src = data;
			for (uint8_t i = 0; i < len; i++) {
				sprintf((dest + 2 * i), "%02X", *(src + i));
			}
			// print as a string
			_log(SERIAL_INFO, newLine, "0x%s", buf);
			break;
		}
		default: {
			LOGi("Unsupported microapp log type: %u", type);
			return ERR_UNKNOWN_TYPE;
		}
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappDelayCommand(microapp_delay_cmd_t* delay_cmd) {
	// don't do anything
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappPinCommand(microapp_pin_cmd_t* pin_cmd) {
	CommandMicroappPin pin = (CommandMicroappPin)pin_cmd->pin;
	switch (pin) {
		case CS_MICROAPP_COMMAND_PIN_SWITCH: {  // same as DIMMER
			return handleMicroappPinSwitchCommand(pin_cmd);
		}
		case CS_MICROAPP_COMMAND_PIN_GPIO1:
		case CS_MICROAPP_COMMAND_PIN_GPIO2:
		case CS_MICROAPP_COMMAND_PIN_GPIO3:
		case CS_MICROAPP_COMMAND_PIN_GPIO4:
		case CS_MICROAPP_COMMAND_PIN_BUTTON1:
		case CS_MICROAPP_COMMAND_PIN_BUTTON2:
		case CS_MICROAPP_COMMAND_PIN_BUTTON3:
		case CS_MICROAPP_COMMAND_PIN_BUTTON4:
		case CS_MICROAPP_COMMAND_PIN_LED1:
		case CS_MICROAPP_COMMAND_PIN_LED2:
		case CS_MICROAPP_COMMAND_PIN_LED3:
		case CS_MICROAPP_COMMAND_PIN_LED4: {
			CommandMicroappPinOpcode1 opcode1 = (CommandMicroappPinOpcode1)pin_cmd->opcode1;
			// We have one virtual device at location 0, so we have to decrement the pin number by one to map
			// to the array in the GPIO module
			pin_cmd->pin--;
			switch (opcode1) {
				case CS_MICROAPP_COMMAND_PIN_MODE: return handleMicroappPinSetModeCommand(pin_cmd);
				case CS_MICROAPP_COMMAND_PIN_ACTION: return handleMicroappPinActionCommand(pin_cmd);
				default: LOGw("Unknown opcode1"); return ERR_UNKNOWN_OP_CODE;
			}
			break;
		}
		default: {
			LOGw("Unknown pin: %i", pin_cmd->pin);
			return ERR_UNKNOWN_TYPE;
		}
	}
}

cs_ret_code_t MicroappProtocol::handleMicroappPinSwitchCommand(microapp_pin_cmd_t* pin_cmd) {
	CommandMicroappPinOpcode2 mode = (CommandMicroappPinOpcode2)pin_cmd->opcode2;
	switch (mode) {
		case CS_MICROAPP_COMMAND_PIN_WRITE: {
			CommandMicroappPinValue val = (CommandMicroappPinValue)pin_cmd->value;
			switch (val) {
				case CS_MICROAPP_COMMAND_VALUE_OFF: {
					LOGi("Turn switch off");
					event_t event(CS_TYPE::CMD_SWITCH_OFF);
					EventDispatcher::getInstance().dispatch(event);
					break;
				}
				case CS_MICROAPP_COMMAND_VALUE_ON: {
					LOGi("Turn switch on");
					event_t event(CS_TYPE::CMD_SWITCH_ON);
					EventDispatcher::getInstance().dispatch(event);
					break;
				}
				default: {
					LOGw("Unknown switch command");
					return ERR_UNKNOWN_TYPE;
				}
			}
			break;
		}
		default: LOGw("Unknown pin mode / opcode: %u", pin_cmd->opcode2); return ERR_UNKNOWN_OP_CODE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappPinSetModeCommand(microapp_pin_cmd_t* pin_cmd) {
	CommandMicroappPin pin = (CommandMicroappPin)pin_cmd->pin;
	TYPIFY(EVT_GPIO_INIT) gpio;
	gpio.pin_index                    = pin;
	gpio.pull                         = 0;
	gpio.callback                     = 0;
	CommandMicroappPinOpcode2 opcode2 = (CommandMicroappPinOpcode2)pin_cmd->opcode2;
	LOGi("Set mode %i for virtual pin %i", opcode2, pin);
	switch (opcode2) {
		case CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP: gpio.pull = 1;
		case CS_MICROAPP_COMMAND_PIN_READ: {
			CommandMicroappPinValue val = (CommandMicroappPinValue)pin_cmd->value;
			switch (val) {
				case CS_MICROAPP_COMMAND_VALUE_RISING:
					gpio.direction = SENSE;
					gpio.polarity  = LOTOHI;
					gpio.callback  = pin_cmd->callback;
					break;
				case CS_MICROAPP_COMMAND_VALUE_FALLING:
					gpio.direction = SENSE;
					gpio.polarity  = HITOLO;
					gpio.callback  = pin_cmd->callback;
					break;
				case CS_MICROAPP_COMMAND_VALUE_CHANGE:
					gpio.direction = SENSE;
					gpio.polarity  = TOGGLE;
					gpio.callback  = pin_cmd->callback;
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
		default: LOGw("Unknown mode"); return ERR_UNKNOWN_TYPE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappPinActionCommand(microapp_pin_cmd_t* pin_cmd) {
	CommandMicroappPin pin = (CommandMicroappPin)pin_cmd->pin;
	LOGd("Clear, set or configure pin %i", pin);
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
				default: LOGw("Unknown switch command"); return ERR_UNKNOWN_TYPE;
			}
			break;
		}
		case CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP:
		case CS_MICROAPP_COMMAND_PIN_READ: {
			// TODO; (note that we do not handle event handler registration here but in SetMode above
			break;
		}
		default: LOGw("Unknown pin mode / opcode: %i", pin_cmd->opcode2); return ERR_UNKNOWN_OP_CODE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappServiceDataCommand(uint8_t* payload, uint16_t length) {

	_logArray(SERIAL_DEBUG, true, payload, length);

	uint16_t commandDataSize = length - 3;  // byte 0 is command, byte 1 is type, byte 2 is option.
	TYPIFY(CMD_MICROAPP_ADVERTISE) eventData;
	if (commandDataSize < sizeof(eventData.appUuid)) {
		LOGi("Payload too small");
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	eventData.version   = 0;  // TODO: define somewhere.
	eventData.type      = 0;  // TODO: define somewhere.
	eventData.appUuid   = (payload[3] << 8) + payload[4];
	eventData.data.len  = commandDataSize - sizeof(eventData.appUuid);
	eventData.data.data = &(payload[5]);
	event_t event(CS_TYPE::CMD_MICROAPP_ADVERTISE, &eventData, sizeof(eventData));
	event.dispatch();
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappTwiCommand(microapp_twi_cmd_t* twi_cmd) {
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
			uint8_t bufSize = twi_cmd->length;
			TYPIFY(EVT_TWI_READ) twi;
			twi.address = twi_cmd->address;
			twi.buf     = twi_cmd->buf;
			twi.length  = bufSize;
			twi.stop    = twi_cmd->stop;
			event_t event(CS_TYPE::EVT_TWI_READ, &twi, sizeof(twi));
			EventDispatcher::getInstance().dispatch(event);
			twi_cmd->ack    = event.result.returnCode;
			twi_cmd->length = twi.length;
			// LOGi("Return value of length: %i", twi_cmd->length);
			// for (int i = 0; i < (int)twi_cmd->length; ++i) {
			//	LOGi("Read: 0x%02x", twi.buf[i]);
			//}
			break;
		}
		default: LOGw("Unknown i2c opcode: %i", twi_cmd->opcode); return ERR_UNKNOWN_OP_CODE;
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappBleCommand(microapp_ble_cmd_t* ble_cmd) {
	switch (ble_cmd->opcode) {
		case CS_MICROAPP_COMMAND_BLE_SCAN_SET_HANDLER: {
			uint16_t type = static_cast<uint16_t>(CS_TYPE::EVT_DEVICE_SCANNED);
			// we will register the handler in the class for first empty slot
			for (int i = 0; i < MAX_BLE_ISR_COUNT; ++i) {
				if (_bleIsr[i].callback == 0) {
					LOGi("Register callback %x", ble_cmd->callback);
					_bleIsr[i].callback = ble_cmd->callback;
					_bleIsr[i].type     = type;
					return ERR_SUCCESS;
				}
			}
			// if no empty isr slot available
			return ERR_NO_SPACE;
		}
		case CS_MICROAPP_COMMAND_BLE_SCAN_START: {
			_microappIsScanning = true;
			break;
		}
		case CS_MICROAPP_COMMAND_BLE_SCAN_STOP: {
			_microappIsScanning = false;
			break;
		}
		default: {
			LOGi("Unknown microapp BLE command opcode: %u", ble_cmd->opcode);
			return ERR_UNKNOWN_OP_CODE;
		}
	}
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappPowerUsageCommand(uint8_t* payload, uint16_t length) {
	if (length < sizeof(payload[0]) + sizeof(microapp_power_usage_t)) {
		LOGi("Payload too small");
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	microapp_power_usage_t* commandPayload = reinterpret_cast<microapp_power_usage_t*>(&(payload[1]));

	TYPIFY(STATE_POWER_USAGE) powerUsage;
	State::getInstance().get(CS_TYPE::STATE_POWER_USAGE, &powerUsage, sizeof(powerUsage));

	commandPayload->powerUsage = powerUsage;

	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappPresenceCommand(uint8_t* payload, uint16_t length) {
	if (length < sizeof(payload[0]) + sizeof(microapp_presence_t)) {
		LOGi("Payload too small");
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	microapp_presence_t* commandPayload = reinterpret_cast<microapp_presence_t*>(&(payload[1]));

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

	commandPayload->presenceBitmask = resultBuf.presence[commandPayload->profileId];
	return ERR_SUCCESS;
}

cs_ret_code_t MicroappProtocol::handleMicroappMeshCommand(
		microapp_mesh_header_t* meshCommand, uint8_t* payload, size_t payloadSize) {
	switch (meshCommand->opcode) {
		case CS_MICROAPP_COMMAND_MESH_SEND: {
			if (payloadSize < sizeof(microapp_mesh_send_header_t)) {
				LOGi("Payload too small");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			auto commandData = reinterpret_cast<microapp_mesh_send_header_t*>(payload);

			TYPIFY(CMD_SEND_MESH_MSG) eventData;
			if (commandData->stoneId == 0) {
				// Broadcast message.
				eventData.flags.flags.broadcast   = true;
				eventData.flags.flags.noHops      = false;
				eventData.flags.flags.reliable    = false;
				eventData.flags.flags.useKnownIds = false;
			}
			else {
				// Targeted message.
				eventData.flags.flags.broadcast   = true;
				eventData.flags.flags.noHops      = false;
				eventData.flags.flags.reliable    = true;
				eventData.flags.flags.useKnownIds = false;
				eventData.idCount                 = 1;
				eventData.targetIds               = &(commandData->stoneId);
			}
			eventData.type    = CS_MESH_MODEL_TYPE_MICROAPP;
			eventData.payload = payload + sizeof(commandData);
			eventData.size    = payloadSize - sizeof(commandData);
			if (eventData.size == 0) {
				LOGi("No message.");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			if (eventData.size > MICROAPP_MAX_MESH_MESSAGE_SIZE) {
				LOGi("Message too large: %u > %u", eventData.size, MICROAPP_MAX_MESH_MESSAGE_SIZE);
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

			event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &eventData, sizeof(eventData));
			event.dispatch();
			if (event.result.returnCode != ERR_SUCCESS) {
				LOGi("Failed to send message, return code: %u", event.result.returnCode);
				return event.result.returnCode;
			}
			break;
		}
		case CS_MICROAPP_COMMAND_MESH_READ_AVAILABLE: {
			if (payloadSize < sizeof(microapp_mesh_read_available_t)) {
				LOGi("Payload too small");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			if (!_meshMessageBuffer.isInitialized()) {
				_meshMessageBuffer.init();
			}

			auto commandData       = reinterpret_cast<microapp_mesh_read_available_t*>(payload);
			commandData->available = !_meshMessageBuffer.empty();
			break;
		}
		case CS_MICROAPP_COMMAND_MESH_READ: {
			if (payloadSize < sizeof(microapp_mesh_read_t)) {
				LOGi("Payload too small");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			if (!_meshMessageBuffer.isInitialized()) {
				_meshMessageBuffer.init();
			}
			if (_meshMessageBuffer.empty()) {
				LOGi("No message in buffer");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

			auto commandData         = reinterpret_cast<microapp_mesh_read_t*>(payload);
			auto message             = _meshMessageBuffer.pop();
			commandData->stoneId     = message.stoneId;
			commandData->messageSize = message.messageSize;
			memcpy(commandData->message, message.message, message.messageSize);
			break;
		}

		default: {
			LOGi("Unknown microapp mesh command opcode: %u", meshCommand->opcode);
			return ERR_UNKNOWN_OP_CODE;
		}
	}
	return ERR_SUCCESS;
}
