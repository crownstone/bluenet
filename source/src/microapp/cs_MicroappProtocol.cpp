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

/**
 * Arguments to the coroutine.
 */
coargs_t g_coargs;

/*
 * This function is called by the microapp in the context of its stack. Even though it is possible to call a log or
 * other bluenet function, this is strongly discouraged. In contrast, yield control towards bluenet explicitly by
 * calling yield() and continue in `MicroappProtocol::callMicroapp()` retrieving the command sent by the microapp.
 *
 * Rather than expecting code within the microapp to call getRamData and set the coargs arguments as parameter or write
 * it to the buffer, we do this here. We can optimize this later (as long as we actually store this in an area that
 * is accessible by the microapp).
 *
 * @param[in] payload                            pointer to buffer with command for bluenet
 * @param[in] length                             length of command (not total buffer size)
 */
cs_ret_code_t microapp_callback(uint8_t* payload, uint16_t length) {
	if (length == 0 || length > MAX_PAYLOAD) {
		return ERR_WRONG_PAYLOAD_LENGTH;
	}

	uint8_t buf[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
	uint8_t readSize = 0;
	uint8_t retCode  = getRamData(IPC_INDEX_CROWNSTONE_APP, buf, BLUENET_IPC_RAM_DATA_ITEM_SIZE, &readSize);

	if (retCode != IPC_RET_SUCCESS) {
		// Dubious: we are here in the microapp coroutine and shouldn't log (even though we can).
		LOGi("Nothing found in RAM ret_code=%u", retCode);
		return ERR_NOT_FOUND;
	}

	// We obtain coargs from bluenet2microapp upon which we set microapp2bluenet, this is no error.
	// The reason is that we do not want to pass the coargs towards the microapp and then pass it back again.
	bluenet2microapp_ipcdata_t& data = *(bluenet2microapp_ipcdata_t*)buf;
	uintptr_t coargs_ptr             = data.coargs_ptr;
	coargs_t* args                   = (coargs_t*)coargs_ptr;
	args->microapp2bluenet.data      = payload;
	args->microapp2bluenet.size      = length;
	yieldCoroutine(args->coroutine);

	return ERR_SUCCESS;
}

#ifdef DUMMY_CALLBACK
/*
 * Helper function that does not go back and forth to the microapp and can be used to test if the coroutines and
 * callbacks are working properly without a microapp present.
 */
void microapp_callback_dummy() {
	static int first_time = true;
	while (1) {
		// just some random data array
		uint8_t data[10];
		uint8_t data_len = 10;
		// fake setup and loop commands
		if (first_time) {
			data[0]    = CS_MICROAPP_COMMAND_SETUP_END;
			first_time = false;
		}
		else {
			data[0] = CS_MICROAPP_COMMAND_LOOP_END;
		}
		// Get the ram data of ourselves (IPC_INDEX_CROWNSTONE_APP).
		uint8_t rd_size = 0;
		bluenet2microapp_ipcdata_t ipc_data;
		getRamData(IPC_INDEX_CROWNSTONE_APP, (uint8_t*)&ipc_data, sizeof(bluenet2microapp_ipcdata_t), &rd_size);

		// Perform the actual callback. Should call microapp_callback and in the end yield.
		int (*callback_func)(uint8_t*, uint16_t) = (int (*)(uint8_t*, uint16_t))ipc_data.microapp_callback;
		callback_func(data, data_len);
	}
}
#endif

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

#ifdef DUMMY_CALLBACK
	LOGi("Call main: %p", &microapp_callback_dummy);
	microapp_callback_dummy();
#else
	coargs_t* coargs = (coargs_t*)p;
	LOGd("Call main: %p", coargs->entry);

	// Rather than working with raw addresses (adding 1 for thumb mode etc.) just use "goto".
	void* ptr = (void*)coargs->entry;
	goto* ptr;
#endif
	// The coroutine should never return. Incorrectly written microapp.
	LOGe("We should not come here!");
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

/*
 * MicroappProtocol constructor zero-initializes most fields and makes sure the instance can receive messages through
 * deriving from EventListener and adding itself to the EventDispatcher as listener.
 */
MicroappProtocol::MicroappProtocol()
		: EventListener()
		, _callbackData(nullptr)
		, _microappIsScanning(false)
		, _meshMessageBuffer(MAX_MESH_MESSAGES_BUFFERED) {

	EventDispatcher::getInstance().addListener(this);

	for (int i = 0; i < MAX_PIN_ISR_COUNT; ++i) {
		_pinIsr[i].pin = 0;
	}
	for (int i = 0; i < MAX_BLE_ISR_COUNT; ++i) {
		_bleIsr[i].id = 0;
	}
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
	data.coargs_ptr        = (uintptr_t)&g_coargs;

	LOGi("Set callback to %p", data.microapp_callback);

	[[maybe_unused]] uint32_t retCode = setRamData(IPC_INDEX_CROWNSTONE_APP, (uint8_t*)&data, data.length);
	LOGi("Set ram data for microapp, retCode=%u", retCode);
}

/*
 * Gets the first instruction for the microapp (this is written in its header). We correct for thumb and check its
 * boundaries. Then we call it from a coroutine context and expect it to yield.
 *
 * @param[in]                                    appIndex (currently ignored)
 */
void MicroappProtocol::callApp(uint8_t appIndex) {
	LOGi("Call microapp #%i", appIndex);

	//	initMemory();

	uintptr_t address = MicroappStorage::getInstance().getStartInstructionAddress(appIndex);
	LOGi("Microapp: start at 0x%08X", address);

	if (checkFlashBoundaries(address) != ERR_SUCCESS) {
		LOGe("Address not within microapp flash boundaries");
		return;
	}

	// The entry function is this immediate address (no correction for thumb mode)
	g_coargs.entry = address;

	// Write coroutine as argument in the struct so we can yield from it in the context of the microapp stack
	g_coargs.coroutine = &_coroutine;
	startCoroutine(&_coroutine, goIntoMicroapp, &g_coargs);
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
 * Called from cs_Microapp every time tick. The microapp does not set anything in RAM but will only read from RAM and
 * call a handler.
 *
 * There's no load failure detection. When the call fails bluenet hangs and should reboot.
 */
void MicroappProtocol::tickMicroapp(uint8_t appIndex) {
	bool call_again = false;
	do {
		callMicroapp();
		call_again = retrieveCommand();
	} while (call_again);
}

/*
 * Overwrite the command field so it is not performed twice.
 */
void MicroappProtocol::setAsProcessed(microapp_cmd_t* cmd) {
	cmd->cmd = CS_MICROAPP_COMMAND_NONE;
}

/*
 * Retrieve command from the microapp.
 */
bool MicroappProtocol::retrieveCommand() {
	// get payload from data
	uint8_t* payload = g_coargs.microapp2bluenet.data;
	uint16_t length  = g_coargs.microapp2bluenet.size;
	handleMicroappCommand(payload, length);
	bool call_again = !stopAfterMicroappCommand(payload, length);
	setAsProcessed(reinterpret_cast<microapp_cmd_t*>(payload));
	return call_again;
}

/*
 * A GPIO callback towards the microapp.
 */
void MicroappProtocol::performCallbackGpio(uint8_t pin) {
	// Write pin command into the buffer.
	microapp_pin_cmd_t* cmd = (microapp_pin_cmd_t*)g_coargs.bluenet2microapp.data;
	cmd->pin                = pin;
	cmd->value              = CS_MICROAPP_COMMAND_VALUE_CHANGE;
	cmd->ack                = false;
	// Resume microapp so it can pick up this command.
	writeCallback();
}

/*
 * Communicate mesh message towards microapp.
 */
void MicroappProtocol::performCallbackMeshMessage(scanned_device_t* dev) {
	// Write bluetooth device to buffer
	microapp_ble_device_t* buf = (microapp_ble_device_t*)g_coargs.bluenet2microapp.data;

	bool found                     = false;
	uint8_t id                     = 0;
	enum MicroappBleEventType type = BleEventDeviceScanned;
	for (int i = 0; i < MAX_BLE_ISR_COUNT; ++i) {
		if (_bleIsr[i].type == type) {
			id    = _bleIsr[i].id;
			found = true;
			break;
		}
	}
	if (!found) {
		LOGw("No callback registered");
		return;
	}

	if (dev->dataSize > sizeof(buf->data)) {
		LOGw("BLE advertisement data too large");
		return;
	}

	buf->cmd = CS_MICROAPP_COMMAND_BLE_DEVICE;

	// Allow microapp to map to handler with given id
	buf->id   = id;
	buf->type = type;

	// Copy address and at the same time convert from little endian to big endian
	std::reverse_copy(dev->address, dev->address + MAC_ADDRESS_LENGTH, buf->addr);
	buf->rssi = dev->rssi;

	// Copy the data itself
	buf->dlen = dev->dataSize;
	memcpy(buf->data, dev->data, dev->dataSize);

	writeCallback();
}

/*
 * Write the callback to the microapp and have it execute it, hopefully. Try it more than once.
 */
void MicroappProtocol::writeCallback() {
	const uint8_t num_retries = 2;
	microapp_acked_cmd_t* buf = (microapp_acked_cmd_t*)g_coargs.bluenet2microapp.data;

	for (int i = 0; i < num_retries; ++i) {
		callMicroapp();

		if (buf->ack) {
			// Successful execution, resume bluenet.
			return;
		}
		LOGi("Command not acked (try %i)", i);
	}
}

/*
 * We resume the previously started coroutine.
 */
void MicroappProtocol::callMicroapp() {
	if (nextCoroutine(&_coroutine)) {
		return;
	}
	// Should only happen if microapp actually ends (and does not yield anymore).
	LOGi("End of coroutine. Should not happen.")
}

/*
 * Register GPIO pin
 */
bool MicroappProtocol::registerGpio(uint8_t pin) {
	// We register the pin at the first empty slot
	for (int i = 0; i < MAX_PIN_ISR_COUNT; ++i) {
		if (!_pinIsr[i].registered) {
			_pinIsr[i].registered = true;
			_pinIsr[i].pin        = pin;
			LOGi("Register callback for pin %i", pin);
			return true;
		}
	}
	LOGw("Could not register callback for pin %i", pin);
	return false;
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
			registerGpio(gpio.pin_index);
			break;
		}
		case CS_TYPE::EVT_GPIO_UPDATE: {
			TYPIFY(EVT_GPIO_UPDATE) gpio = *(TYPIFY(EVT_GPIO_UPDATE)*)event.data;
			LOGi("GPIO callback for pin %i", gpio.pin_index);
			performCallbackGpio(gpio.pin_index);
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

	performCallbackMeshMessage(dev);
}

/*
 * Forwards commands from the microapp to the relevant handler
 */
cs_ret_code_t MicroappProtocol::handleMicroappCommand(uint8_t* payload, uint16_t length) {
	_logArray(SERIAL_DEBUG, true, payload, length);
	uint8_t command = reinterpret_cast<microapp_cmd_t*>(payload)->cmd;
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
			microapp_mesh_cmd_t* mesh_cmd = reinterpret_cast<microapp_mesh_cmd_t*>(payload);
			return handleMicroappMeshCommand(mesh_cmd);
		}
		case CS_MICROAPP_COMMAND_SETUP_END: {
			// LOGi("Setup end");
			break;
		}
		case CS_MICROAPP_COMMAND_LOOP_END: {
			// LOGi("Loop end");
			break;
		}
		case CS_MICROAPP_COMMAND_NONE: {
			// ignore, nothing written
			break;
		}
		default: {
			_log(SERIAL_INFO, true, "Unknown command %u of length %u:", command, length);
			_logArray(SERIAL_INFO, true, payload, length);
			return ERR_UNKNOWN_TYPE;
		}
	}
	return ERR_SUCCESS;
}

bool MicroappProtocol::stopAfterMicroappCommand(uint8_t* payload, uint16_t length) {
	uint8_t command = reinterpret_cast<microapp_cmd_t*>(payload)->cmd;
	switch (command) {
		case CS_MICROAPP_COMMAND_PIN:
		case CS_MICROAPP_COMMAND_LOG:
		case CS_MICROAPP_COMMAND_SERVICE_DATA:
		case CS_MICROAPP_COMMAND_TWI:
		case CS_MICROAPP_COMMAND_BLE:
		case CS_MICROAPP_COMMAND_POWER_USAGE:
		case CS_MICROAPP_COMMAND_PRESENCE:
		case CS_MICROAPP_COMMAND_MESH: return false;
		case CS_MICROAPP_COMMAND_DELAY:
		case CS_MICROAPP_COMMAND_SETUP_END:
		case CS_MICROAPP_COMMAND_LOOP_END:
		case CS_MICROAPP_COMMAND_NONE:
		default: return true;
	}
	return true;
}

// TODO: establish a proper default log level for microapps
#define LOCAL_MICROAPP_LOG_LEVEL SERIAL_INFO

cs_ret_code_t MicroappProtocol::handleMicroappLogCommand(uint8_t* payload, uint16_t length) {

	microapp_log_cmd_t* command = reinterpret_cast<microapp_log_cmd_t*>(payload);

	__attribute__((unused)) bool newLine = false;
	if (command->option == CS_MICROAPP_COMMAND_LOG_NEWLINE) {
		newLine = true;
	}
	switch (command->type) {
		case CS_MICROAPP_COMMAND_LOG_CHAR: {
			[[maybe_unused]] microapp_log_char_cmd_t* cmd = reinterpret_cast<microapp_log_char_cmd_t*>(payload);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i%s", (int)cmd->value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_SHORT: {
			[[maybe_unused]] microapp_log_short_cmd_t* cmd = reinterpret_cast<microapp_log_short_cmd_t*>(payload);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i%s", (int)cmd->value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_UINT: {
			[[maybe_unused]] microapp_log_uint_cmd_t* cmd = reinterpret_cast<microapp_log_uint_cmd_t*>(payload);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%u%s", cmd->value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_INT: {
			[[maybe_unused]] microapp_log_int_cmd_t* cmd = reinterpret_cast<microapp_log_int_cmd_t*>(payload);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i%s", cmd->value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_FLOAT: {
			[[maybe_unused]] microapp_log_float_cmd_t* cmd = reinterpret_cast<microapp_log_float_cmd_t*>(payload);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%f%s", cmd->value);

			// TODO: We get into trouble when printing using %f
			//__attribute__((unused)) int value  = *(int*)&payload[3];
			//__attribute__((unused)) int before = value / 10000;
			//__attribute__((unused)) int after  = value - (before * 10000);
			//_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%i.%04i", before, after);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_DOUBLE: {
			[[maybe_unused]] microapp_log_double_cmd_t* cmd = reinterpret_cast<microapp_log_double_cmd_t*>(payload);
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%f%s", cmd->value);
			// TODO: This will fail (see float for workaround)
			//__attribute__((unused)) double value = *(double*)&payload[3];
			//_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%f%s", value);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_STR: {
			[[maybe_unused]] microapp_log_string_cmd_t* cmd = reinterpret_cast<microapp_log_string_cmd_t*>(payload);
			// Enforce a zero-byte at the end before we log
			uint8_t zeroByteIndex = MAX_MICROAPP_STRING_LENGTH - 1;
			if (cmd->length < zeroByteIndex) {
				zeroByteIndex = cmd->length;
			}
			cmd->str[zeroByteIndex] = 0;
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%s", cmd->str);

			//__attribute__((unused)) int str_length = length - 3;
			// TODO: Check if length <= max_length - 1, for null terminator.
			//__attribute__((unused)) char* data = reinterpret_cast<char*>(&(payload[3]));
			// data[str_length]                   = 0;
			//_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "%s", data);
			break;
		}
		case CS_MICROAPP_COMMAND_LOG_ARR: {
			[[maybe_unused]] microapp_log_array_cmd_t* cmd = reinterpret_cast<microapp_log_array_cmd_t*>(payload);
			uint8_t len                                    = MAX_MICROAPP_ARRAY_LENGTH;
			if (cmd->length < len) {
				len = cmd->length;
			}
			if (len == 0) {
				break;
			}
			for (uint8_t i = 0; i < len; ++i) {
				// Print hexadecimal values
				_log(LOCAL_MICROAPP_LOG_LEVEL, false, "0x%x ", (int)cmd->arr[i]);
			}
			_log(LOCAL_MICROAPP_LOG_LEVEL, newLine, "");
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

/*
 * We will register the handler in the class for first empty slot
 */
bool MicroappProtocol::registerBleCallback(uint8_t id) {
	for (int i = 0; i < MAX_BLE_ISR_COUNT; ++i) {
		if (!_bleIsr[i].registered) {
			_bleIsr[i].registered = true;
			_bleIsr[i].type       = BleEventDeviceScanned;
			_bleIsr[i].id         = id;
			LOGi("Registered callback %i", id);
			return true;
		}
	}
	// no empty slot available
	LOGw("Could not register BLE callback %i", id);
	return false;
}

cs_ret_code_t MicroappProtocol::handleMicroappBleCommand(microapp_ble_cmd_t* ble_cmd) {
	switch (ble_cmd->opcode) {
		case CS_MICROAPP_COMMAND_BLE_SCAN_SET_HANDLER: {
			bool success = registerBleCallback(ble_cmd->id);
			return success ? ERR_SUCCESS : ERR_NO_SPACE;
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

cs_ret_code_t MicroappProtocol::handleMicroappMeshCommand(microapp_mesh_cmd_t* command) {
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

			if (broadcast) {
				eventData.flags.flags.broadcast = true;
				eventData.flags.flags.reliable  = false;
			}
			else {
				eventData.flags.flags.broadcast = true;
				eventData.flags.flags.reliable  = true;
				eventData.idCount               = 1;
				eventData.targetIds             = &(cmd->stoneId);
			}
			eventData.flags.flags.useKnownIds = false;
			eventData.flags.flags.noHops      = false;
			eventData.type                    = CS_MESH_MODEL_TYPE_MICROAPP;
			eventData.payload                 = cmd->data;
			eventData.size                    = cmd->dlen;
			event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &eventData, sizeof(eventData));
			event.dispatch();
			if (event.result.returnCode != ERR_SUCCESS) {
				LOGi("Failed to send message, return code: %u", event.result.returnCode);
				return event.result.returnCode;
			}
			break;
		}
		case CS_MICROAPP_COMMAND_MESH_READ_AVAILABLE: {
			auto cmd = reinterpret_cast<microapp_mesh_read_available_cmd_t*>(command);

			if (!_meshMessageBuffer.isInitialized()) {
				_meshMessageBuffer.init();
			}

			// TODO: This assumes nothing will overwrite the buffer
			cmd->available = !_meshMessageBuffer.empty();

			// TODO: One might want to call callMicroapp here (at least once).
			// That would benefit from an ack "the other way around" (so microapp knows "available" is updated).
			break;
		}
		case CS_MICROAPP_COMMAND_MESH_READ: {
			auto cmd = reinterpret_cast<microapp_mesh_read_cmd_t*>(command);

			if (!_meshMessageBuffer.isInitialized()) {
				_meshMessageBuffer.init();
			}
			if (_meshMessageBuffer.empty()) {
				LOGi("No message in buffer");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}

			auto message     = _meshMessageBuffer.pop();
			
			// TODO: This assumes nothing will overwrite the buffer
			cmd->stoneId     = message.stoneId;
			cmd->messageSize = message.messageSize;
			if (message.messageSize > MICROAPP_MAX_MESH_MESSAGE_SIZE) {
				LOGi("Message with wrong size in buffer");
				return ERR_WRONG_PAYLOAD_LENGTH;
			}
			memcpy(cmd->message, message.message, message.messageSize);

			// TODO: One might want to call callMicroapp here (at least once).
			// That would benefit from an ack "the other way around" (so microapp knows "available" is updated).
			break;
		}

		default: {
			LOGi("Unknown microapp mesh command opcode: %u", meshCommand->opcode);
			return ERR_UNKNOWN_OP_CODE;
		}
	}
	return ERR_SUCCESS;
}
