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

#include <algorithm>
#include <ble/cs_UUID.h>
#include <cs_MicroappStructs.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_AutoConfig.h>
#include <common/cs_Types.h>
#include <logging/cs_Logger.h>
#include <drivers/cs_Gpio.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventDispatcher.h>
#include <ipc/cs_IpcRamData.h>
#include <microapp/cs_MicroappProtocol.h>
#include <protocol/cs_ErrorCodes.h>
#include <storage/cs_State.h>
#include <storage/cs_StateData.h>
#include <util/cs_BleError.h>
#include <util/cs_Error.h>
#include <util/cs_Hash.h>
#include <util/cs_Utils.h>
#include <microapp/cs_MicroappStorage.h>

void handleSwitchCommand(pin_cmd_t* pin_cmd) {
	CommandMicroappPinOpcode2 mode = (CommandMicroappPinOpcode2)pin_cmd->opcode2;
	switch(mode) {
		case CS_MICROAPP_COMMAND_PIN_WRITE: {
			CommandMicroappPinValue val = (CommandMicroappPinValue)pin_cmd->value;
			switch(val) {
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
				default:
					LOGw("Unknown switch command");
			}
			break;
		}
		default:
			LOGw("Unknown pin mode / opcode: %i", pin_cmd->opcode2);
	}
}

/*
 * Set mode
 */
void handlePinSetModeCommand(pin_cmd_t* pin_cmd) {
	CommandMicroappPin pin = (CommandMicroappPin)pin_cmd->pin;
	TYPIFY(EVT_GPIO_INIT) gpio;
	gpio.pin_index = pin;
	gpio.pull = 0;
	gpio.callback = 0;
	CommandMicroappPinOpcode2 opcode2 = (CommandMicroappPinOpcode2)pin_cmd->opcode2;
	LOGi("Set mode %i for virtual pin %i", opcode2, pin);
	switch(opcode2) {
		case CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP:
			gpio.pull = 1;
		case CS_MICROAPP_COMMAND_PIN_READ: {
			CommandMicroappPinValue val = (CommandMicroappPinValue)pin_cmd->value;
			switch(val) {
				case CS_MICROAPP_COMMAND_VALUE_RISING:
					gpio.direction = SENSE;
					gpio.polarity = LOTOHI;
					gpio.callback = pin_cmd->callback;
					break;
				case CS_MICROAPP_COMMAND_VALUE_FALLING:
					gpio.direction = SENSE;
					gpio.polarity = HITOLO;
					gpio.callback = pin_cmd->callback;
					break;
				case CS_MICROAPP_COMMAND_VALUE_CHANGE:
					gpio.direction = SENSE;
					gpio.polarity = TOGGLE;
					gpio.callback = pin_cmd->callback;
					break;
				default:
					gpio.direction = INPUT;
					gpio.polarity = NONE;
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
		default:
			LOGw("Unknown mode");
			return;
	}
}

void handlePinActionCommand(pin_cmd_t* pin_cmd) {
	CommandMicroappPin pin = (CommandMicroappPin)pin_cmd->pin;
	LOGi("Clear, set or configure pin %i", pin);
	CommandMicroappPinOpcode2 opcode2 = (CommandMicroappPinOpcode2)pin_cmd->opcode2;
	switch(opcode2) {
		case CS_MICROAPP_COMMAND_PIN_WRITE: {
			TYPIFY(EVT_GPIO_WRITE) gpio;
			gpio.pin_index = pin;
			CommandMicroappPinValue val = (CommandMicroappPinValue)pin_cmd->value;
			switch(val) {
				case CS_MICROAPP_COMMAND_VALUE_OFF: {
					LOGi("Clear GPIO pin");
					gpio.length = 1;
					uint8_t buf[1];
					buf[0] = 0;
					gpio.buf = buf;
					event_t event(CS_TYPE::EVT_GPIO_WRITE, &gpio, sizeof(gpio));
					EventDispatcher::getInstance().dispatch(event);
					break;
				}
				case CS_MICROAPP_COMMAND_VALUE_ON: {
					LOGi("Set GPIO pin");
					gpio.length = 1;
					uint8_t buf[1];
					buf[0] = 1;
					gpio.buf = buf;
					event_t event(CS_TYPE::EVT_GPIO_WRITE, &gpio, sizeof(gpio));
					EventDispatcher::getInstance().dispatch(event);
					break;
				}
				default:
					LOGw("Unknown switch command");
					return;
			}
			break;
		}
		case CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP:
		case CS_MICROAPP_COMMAND_PIN_READ: {
			// TODO; (not that we do not handle event handler registration here but in SetMode above
			break;
		}
		default:
			LOGw("Unknown pin mode / opcode: %i", pin_cmd->opcode2);
			return;
	}
}

/*
 * Handle pin commands
 */
void handlePinCommand(pin_cmd_t* pin_cmd) {
	CommandMicroappPin pin = (CommandMicroappPin)pin_cmd->pin;
	switch(pin) {
		case CS_MICROAPP_COMMAND_PIN_SWITCH: { // same as DIMMER
			handleSwitchCommand(pin_cmd);
			break;
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
			switch(opcode1) {
				case CS_MICROAPP_COMMAND_PIN_MODE:
					handlePinSetModeCommand(pin_cmd);
					break;
				case CS_MICROAPP_COMMAND_PIN_ACTION:
					handlePinActionCommand(pin_cmd);
					break;
				default:
					LOGw("Unknown opcode1");
					return;
			}
			break;
		}
		default:
			LOGw("Unknown pin: %i", pin_cmd->pin);
	}
}

/*
 * Handle TWI commands
 */
void handleTwiCommand(twi_cmd_t* twi_cmd) {
	CommandMicroappTwiOpcode opcode = (CommandMicroappTwiOpcode)twi_cmd->opcode;
	switch(opcode) {
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
			twi.buf = twi_cmd->buf;
			twi.length = bufSize;
			twi.stop = twi_cmd->stop;
			event_t event(CS_TYPE::EVT_TWI_WRITE, &twi, sizeof(twi));
			EventDispatcher::getInstance().dispatch(event);
			break;
		}
		case CS_MICROAPP_COMMAND_TWI_READ: {
			LOGd("Read from i2c address: 0x%02x", twi_cmd->address);
			uint8_t bufSize = twi_cmd->length;
			TYPIFY(EVT_TWI_READ) twi;
			twi.address = twi_cmd->address;
			twi.buf = twi_cmd->buf;
			twi.length = bufSize;
			twi.stop = twi_cmd->stop;
			event_t event(CS_TYPE::EVT_TWI_READ, &twi, sizeof(twi));
			EventDispatcher::getInstance().dispatch(event);
			twi_cmd->ack = event.result.returnCode;
			twi_cmd->length = twi.length;
			//LOGi("Return value of length: %i", twi_cmd->length);
			//for (int i = 0; i < (int)twi_cmd->length; ++i) {
			//	LOGi("Read: 0x%02x", twi.buf[i]);
			//}
			break;
		}
		default:
			LOGw("Unknown i2c opcode: %i", twi_cmd->opcode);
	}
}

int handleCommand(uint8_t* payload, uint16_t length) {
	//LOGd("Incoming message: [%i, %i, %i, ...]", payload[0], payload[1], payload[2]);
	uint8_t command = payload[0];
	switch(command) {
		case CS_MICROAPP_COMMAND_LOG: {
			char type = payload[1];
			char option = payload[2];
			__attribute__((unused)) bool newLine = false;
			if (option == CS_MICROAPP_COMMAND_LOG_NEWLINE) {
				newLine = true;
			}
			switch(type) {
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
//					__attribute__((unused)) int value = (payload[3] << 8) + payload[3];
					__attribute__((unused)) int value = *(int*)&payload[3];
					_log(SERIAL_INFO, newLine, "%i%s", (int)value);
					break;
				}
				case CS_MICROAPP_COMMAND_LOG_FLOAT: {
					// TODO: We get into trouble when printing using %f
					__attribute__((unused)) int value = *(int*)&payload[3];
					__attribute__((unused)) int before = value / 10000;
					__attribute__((unused)) int after = value - (before * 10000);
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
					__attribute__((unused)) int str_length = length - 3; // Check if length <= max_length - 1, for null terminator.
					__attribute__((unused)) char *data = reinterpret_cast<char*>(&(payload[3]));
					data[str_length] = 0;
					_log(SERIAL_INFO, newLine, "%s", data);
					break;
				}
				default:
					LOGi("Unsupported type (for now): %i", type);
			}
			break;
		}
		case CS_MICROAPP_COMMAND_DELAY: {
			int delay = (payload[1] << 8) + payload[2];
			LOGd("Microapp delay of %i", delay);
			uintptr_t coargs_ptr = (payload[3] << 24) + (payload[4] << 16) + (payload[5] << 8) + payload[6];
			// cast to coroutine args struct
			coargs* args = (coargs*) coargs_ptr;
			args->cntr++;
			args->delay = delay;
			yield(args->c);
			break;
		}
		case CS_MICROAPP_COMMAND_PIN: {
			pin_cmd_t *pin_cmd = (pin_cmd_t*)payload;
			handlePinCommand(pin_cmd);
			break;
		}
		case CS_MICROAPP_COMMAND_TWI: {
			twi_cmd_t *twi_cmd = (twi_cmd_t*)payload;
			handleTwiCommand(twi_cmd);
			break;
		}
		case CS_MICROAPP_COMMAND_SERVICE_DATA: {
			LOGd("Service data:");
			_logArray(SERIAL_DEBUG, true, payload, length);

			uint16_t commandDataSize = length - 3; // byte 0 is command, byte 1 is type, byte 2 is ??.
			TYPIFY(CMD_MICROAPP_ADVERTISE) eventData;
			if (commandDataSize < sizeof(eventData.appUuid)) {
				LOGi("payload too small");
				break;
			}
			eventData.version = 0; // TODO: define somewhere.
			eventData.type = 0; // TODO: define somewhere.
			eventData.appUuid = (payload[3] << 8) + payload[4];
			eventData.data.len = commandDataSize - sizeof(eventData.appUuid);
			eventData.data.data = &(payload[5]);
			//			BLEutil::printArray(eventData.data.data, eventData.data.len, SERIAL_INFO);
			event_t event(CS_TYPE::CMD_MICROAPP_ADVERTISE, &eventData, sizeof(eventData));
			event.dispatch();
			break;
		}
		default:
			_log(SERIAL_INFO, false, "Unknown command %i of length %i: [", command, length);
			// print first few bytes
			int maxl = length;
			if (length > 4) maxl = 4;
			for (int i = 0; i < maxl; i++) {
				_log(SERIAL_INFO, false, "0x%i ", payload[i]);
			}
			if (length > 4) {
				_log(SERIAL_INFO, true, "...,...]");
			} else {
				_log(SERIAL_INFO, true, "]");
			}
	}

	return ERR_SUCCESS;
}

extern "C" {

	int microapp_callback(uint8_t *payload, uint16_t length) {
		if (length == 0) return ERR_NO_PAYLOAD;
		if (length > 255) return ERR_TOO_LARGE;

		return handleCommand(payload, length);
	}

} // extern C

MicroappProtocol::MicroappProtocol(): EventListener() {

	EventDispatcher::getInstance().addListener(this);

	_setup = 0;
	_loop = 0;
	_booted = false;

	for (int i = 0; i < MAX_ISR_COUNT; ++i) {
		_isr[i].pin = 0;
		_isr[i].callback = 0;
	}

	_callbackData = NULL;
}

/*
 * Set the microapp_callback in the IPC ram data bank. It can later on be used by the microapp to find the address
 * of that function to call back into the bluenet code.
 */
void MicroappProtocol::setIpcRam() {
	LOGi("Set IPC info for microapp");
	uint8_t buf[BLUENET_IPC_RAM_DATA_ITEM_SIZE];

	// protocol version
	const char protocol_version = 0;
	buf[0] = protocol_version;
	uint8_t len = 1;

	// address of callback() function (is a C function)
	uintptr_t address = (uintptr_t)&microapp_callback;
	for (uint16_t i = 0; i < sizeof(uintptr_t); ++i) {
		buf[i+len] = (uint8_t)(0xFF & (address >> (i*8)));
	}
	len += sizeof(uintptr_t);

	// truncate (rather than assert)
	if (len > BLUENET_IPC_RAM_DATA_ITEM_SIZE) {
		len = BLUENET_IPC_RAM_DATA_ITEM_SIZE;
	}

	// set buffer in RAM
	setRamData(IPC_INDEX_CROWNSTONE_APP, buf, len);
}

/**
 * Get the ram structure in which loop and setup of the microapp are stored.
 */
uint16_t MicroappProtocol::interpretRamdata() {
	LOGi("Get IPC info for microapp");
	uint8_t buf[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
	for (int i = 0; i < BLUENET_IPC_RAM_DATA_ITEM_SIZE; ++i) {
		buf[i] = 0;
	}
	uint8_t rd_size = 0;
	uint8_t ret_code = getRamData(IPC_INDEX_MICROAPP, buf, BLUENET_IPC_RAM_DATA_ITEM_SIZE, &rd_size);

	bluenet_ipc_ram_data_item_t *ramStr = getRamStruct(IPC_INDEX_MICROAPP);
	if (ramStr != NULL) {
		LOGi("Get microapp info from address: %p", ramStr);
	}

	if (_debug) {
		LOGi("Return code: %i", ret_code);
		LOGi("Return length: %i", rd_size);
		LOGi("  protocol: %02x", buf[0]);
		LOGi("  setup():  %02x %02x %02x %02x", buf[1], buf[2], buf[3], buf[4]);
		LOGi("  loop():   %02x %02x %02x %02x", buf[5], buf[6], buf[7], buf[8]);
	}

	if (ret_code == IPC_RET_SUCCESS) {
		LOGi("Found RAM for Microapp");
		LOGi("  protocol: %02x", buf[0]);
		LOGi("  setup():  %02x %02x %02x %02x", buf[1], buf[2], buf[3], buf[4]);
		LOGi("  loop():   %02x %02x %02x %02x", buf[5], buf[6], buf[7], buf[8]);

		uint8_t protocol = buf[0];
		if (protocol == 0) {
			_setup = 0, _loop =0;
			uint8_t offset = 1;
			for (int i = 0; i < 4; ++i) {
				_setup = _setup | ( (uintptr_t)(buf[i+offset]) << (i*8));
			}
			offset = 5;
			for (int i = 0; i < 4; ++i) {
				_loop = _loop | ( (uintptr_t)(buf[i+offset]) << (i*8));
			}
		}

		LOGi("Found setup at %p", _setup);
		LOGi("Found loop at %p", _loop);
		if (_loop && _setup) {
			return ERR_SUCCESS;
		}
	} else {
		LOGi("Nothing found in RAM");
		return ERR_NOT_FOUND;
	}
	return ERR_UNSPECIFIED;
}

/**
 * Call the app, boot it.
 *
 * TODO: Return setup and loop addresses
 */
void MicroappProtocol::callApp(uint8_t appIndex) {
	static bool thumbMode = true;

	initMemory();

	uintptr_t address = MicroappStorage::getInstance().getStartInstructionAddress(appIndex);
	LOGi("Microapp: start at 0x%08X", address);

	if (thumbMode) {
		address += 1;
	}
	LOGi("Check main code at 0x%08X", address);
	char *arr = (char*)address;
	if (arr[0] != 0xFF) {
		void (*microappMain)() = (void (*)()) address;
		LOGi("Call function in module: 0x%p", microappMain);
		(*microappMain)();
		LOGi("Module did run.");
	}
	_booted = true;
	LOGi("Booted is at address: 0x%p", &_booted);
}

uint16_t MicroappProtocol::initMemory() {
	// We have a reserved area of RAM. For now let us clear it to 0
	// This is actually incorrect (we should skip) and it probably can be done at once as well
	LOGi("Init memory: clear 0x%p to 0x%p", g_RAM_MICROAPP_BASE, g_RAM_MICROAPP_BASE + g_RAM_MICROAPP_AMOUNT);
	for (uint32_t i = 0; i < g_RAM_MICROAPP_AMOUNT; ++i) {
		uint32_t *const val = (uint32_t *)(uintptr_t)(g_RAM_MICROAPP_BASE + i);
		*val = 0;
	}

	// The above is fine for .bss (which is uninitialized) but not for .data. It needs to be copied
	// to the right addresses.
	return ERR_SUCCESS;
}

/*
 * Called from cs_Microapp every time tick. Only when _booted gets up will this function become active.
 */
void MicroappProtocol::callSetupAndLoop(uint8_t appIndex) {

	static uint16_t counter = 0;
	if (_booted) {

		if (!_loaded) {
			LOGi("Start loading");
			uint16_t ret_code = interpretRamdata();
			if (ret_code == ERR_SUCCESS) {
				LOGi("Set loaded to true");
				_loaded = true;
			} else {
				LOGw("Disable microapp. After boot not the right info available");
				// Apparently, something went wrong?
				_booted = false;
			}
		}

		if (_loaded) {
			if (counter == 0) {
				// TODO: we cannot call delay in setup in this way...
				void (*setup_func)() = (void (*)()) _setup;
				LOGi("Call setup at 0x%x", _setup);
				setup_func();
				LOGi("Setup done");
				_cocounter = 0;
				_coskip = 0;
			}
			counter++;
			if (counter % MICROAPP_LOOP_FREQUENCY == 0) {
				if (_coskip > 0) {
					// Skip (due to by the microapp communicated delay)
					_coskip--;
				} else {
					//LOGd("Call loop");
					callLoop(_cocounter, _coskip);
					if (_cocounter == -1) {
						// Done, reset flags
						_cocounter = 0;
						_coskip = 0;
					}
				}
			}
			// Loop around, but do not hit 0 again
			if (counter == 0) {
				counter = 1;
			}
		}
	}
}

/**
 * Call loop (internally).
 *
 * This function can be improved in clearity for the developer. It now works as follows:
 *   - when cntr = 0 we start a new loop
 *   - we call next and set skip if we actually were "yielded"
 */
void MicroappProtocol::callLoop(int & cntr, int & skip) {
	if (cntr == 0) {
		// start a new loop
		_coargs = {&_coroutine, 1, 0};
		void (*loop_func)(void*) = (void (*)(void*)) _loop;
		start(&_coroutine, loop_func, &_coargs);
	}

	if (next(&_coroutine)) {
		// here we come only on yield
		cntr = _coargs.cntr;
		skip = _coargs.delay / MICROAPP_LOOP_INTERVAL_MS;
		return;
	}

	// indicate that we are done
	cntr = -1;
}

/**
 * Required if we have to pass events through to the microapp.
 */
void MicroappProtocol::handleEvent(event_t & event) {
	switch(event.type) {
		// Listen to GPIO events, will be used later to implement attachInterrupt in microapp.
		case CS_TYPE::EVT_GPIO_INIT: {
			LOGi("Register GPIO event handler for microapp");
			TYPIFY(EVT_GPIO_INIT) gpio = *(TYPIFY(EVT_GPIO_INIT)*)event.data;
			
			// we will register the handler in the class for first empty slot
			for (int i = 0; i < MAX_ISR_COUNT; ++i) {
				if (_isr[i].callback == 0) {
					LOGi("Register callback %x or %i for pin %i", gpio.callback, gpio.callback, gpio.pin_index);
					_isr[i].callback = gpio.callback;
					_isr[i].pin = gpio.pin_index;
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
			for (int i = 0; i < MAX_ISR_COUNT; ++i) {
				if (_isr[i].pin == gpio.pin_index) {
					callback = _isr[i].callback;
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
			void (*callback_func)() = (void (*)()) callback;
			LOGi("Call callback at 0x%x", callback);
			callback_func();
			break;
		}
		default:
			break;
	}
}
