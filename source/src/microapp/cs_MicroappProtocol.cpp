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
#include <cfg/cs_Config.h>
#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
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

enum ErrorCodesMicroapp {
	ERR_NO_PAYLOAD                   = 0x01,    // need at least an opcode in the payload
	ERR_TOO_LARGE                    = 0x02,
};

enum CommandMicroapp {
	CS_MICROAPP_COMMAND_LOG          = 0x01,
	CS_MICROAPP_COMMAND_DELAY        = 0x02,
	CS_MICROAPP_COMMAND_PIN          = 0x03,
};

enum CommandMicroappLog {
	CS_MICROAPP_COMMAND_LOG_CHAR     = 0x00,
	CS_MICROAPP_COMMAND_LOG_INT      = 0x01,
	CS_MICROAPP_COMMAND_LOG_STR      = 0x02,
};

enum CommandMicroappPin {
	CS_MICROAPP_COMMAND_PIN_SWITCH   = 0x01,
};

enum CommandMicroappSwitch {
	CS_MICROAPP_COMMAND_SWITCH_OFF   = 0x00,
	CS_MICROAPP_COMMAND_SWITCH_ON    = 0x01,
};

/*
 * The function forwardCommand is called from microapp_callback.
 */
void forwardCommand(char command, char *data, uint16_t length) {
	LOGi("Set command");
	switch(command) {
	case CS_MICROAPP_COMMAND_PIN: {
		int pin = data[0];
		if (pin != CS_MICROAPP_COMMAND_PIN_SWITCH) {
			LOGd("Unknown pin");
			return;
		}
		int mode = data[1];
		switch(mode) {
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
		}
	}
	}
}

extern "C" {

int microapp_callback(char *payload, uint16_t length) {
	if (length == 0) return ERR_NO_PAYLOAD;
	if (length > 255) return ERR_TOO_LARGE;

	uint8_t command = payload[0];

	switch(command) {
	case CS_MICROAPP_COMMAND_LOG: {
		char type = payload[1];
		switch(type) {
		case CS_MICROAPP_COMMAND_LOG_CHAR: {
			char value = payload[2];
			LOGi("%i", (int)value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_INT: {
			int value = (payload[2] << 8) + payload[3];
			LOGi("%i", value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_STR: {
			int str_length = length - 2;
			char *data = &(payload[2]);
			data[str_length] = 0;
			LOGi("%s", data);
			break;
		}
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
		forwardCommand(command, &payload[1], length - 1);
		break;
	}
	default:
		LOGi("Unknown command of length %i", length);
		int ml = length;
		if (length > 4) ml = 4;
		for (int i = 0; i < ml; i++) {
			LOGi("0x%i", payload[i]);
		}
	}

	return ERR_SUCCESS;
}

} // extern C

MicroappProtocol::MicroappProtocol() {
	_setup = 0;
	_loop = 0;
	_booted = false;
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
void MicroappProtocol::callApp() {
	static bool thumb_mode = true;

	// Check if we want to do this again
	//if (!isEnabled()) {
	//	LOGi("Microapp: app not enabled.");
	//	return;
	//} 
	
	TYPIFY(STATE_MICROAPP) state_microapp;
	cs_state_id_t app_id = 0;
	cs_state_data_t data(CS_TYPE::STATE_MICROAPP, app_id, (uint8_t*)&state_microapp, sizeof(state_microapp));
	State::getInstance().get(data);

	if (state_microapp.start_addr == 0x00) {
		LOGi("Module can't be run. Start address 0?");
		_booted = false;
		return;
	}

	initMemory();

	uintptr_t address = state_microapp.start_addr + state_microapp.offset;
	LOGi("Microapp: start at 0x%04x", address);

	if (thumb_mode) address += 1;
	LOGi("Check main code at %p", address);
	char *arr = (char*)address;
	if (arr[0] != 0xFF) {
		void (*microapp_main)() = (void (*)()) address;
		LOGi("Call function in module: %p", microapp_main);
		(*microapp_main)();
		LOGi("Module run.");
	}
	_booted = true;
}

uint16_t MicroappProtocol::initMemory() {
	// We allow an area of 0x2000B000 and then two pages for RAM. For now let us clear it to 0
	// This is actually incorrect (we should skip) and it probably can be done at once as well
	for (int i = 0; i < 1024 * 2; ++i) {
		uint32_t *const val = (uint32_t *)(uintptr_t)(RAM_MICROAPP_BASE + i);
		*val = 0;
	}

	// The above is fine for .bss (which is uninitialized) but not for .data. It needs to be copied
	// to the right addresses.
	return ERR_SUCCESS;
}


void MicroappProtocol::callSetupAndLoop() {

	static uint16_t counter = 0;
	if (_booted) {
		
		if (!_loaded) {
			uint16_t ret_code = interpretRamdata();
			if (ret_code == ERR_SUCCESS) {
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
				LOGi("Call setup");
				void (*setup_func)() = (void (*)()) _setup;
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
					LOGi("Call loop");
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

