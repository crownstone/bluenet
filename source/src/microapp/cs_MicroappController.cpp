/**
 *
 * Microapp protocol.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: April 4, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

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

#include <algorithm>

#include "nrf_fstorage_sd.h"

/**
 * A developer option that calls the microapp through a function call (rather than a jump). A properly compiled
 * microapp can be started from the very first address after the header. The ResetHandler is written to that position.
 * This is done in linker scripts. If something goes off in the microapp in the startup code it might be convenient to
 * immediately call a particular function in the microapp. This address can be also the very first address but it can
 * just as well be in the middle of the microapp binary. Default is 0.
 */
#define DEVELOPER_OPTION_USE_FUNCTION_CALL 0

/*
 * Overwrite the address that is read from the microapp header (the offset field) by the following hardcoded address.
 * The default behaviour has this macro undefined.
 */
#define DEVELOPER_OPTION_OVERWRITE_FUNCTION_CALL_ADDRESS 0x69014
#undef DEVELOPER_OPTION_OVERWRITE_FUNCTION_CALL_ADDRESS

/*
 * Use a dummy callback in bluenet itself (no microapp is required in that case) by setting this to 1. Default is 0.
 */
#define DEVELOPER_OPTION_USE_DUMMY_CALLBACK_IN_BLUENET 0

/*
 * Disable coroutines and use direct calls by setting this macro to 1. Default is 0.
 */
#define DEVELOPER_OPTION_DISABLE_COROUTINE 0

extern "C" {

/*
 * Set arguments for coroutine.
 */
void setCoroutineContext(uint8_t coroutineIndex, bluenet_io_buffer_t* io_buffer) {
	stack_params_t* stackParams = getStackParams();
	coroutine_args_t* args      = (coroutine_args_t*)stackParams->arg;
	switch (coroutineIndex) {
		case COROUTINE_MICROAPP0: args->io_buffer = io_buffer; break;
		// potentially more microapps here
		default: break;
	}
}

/*
 * This function is the only function called by the microapp. It is called from the coroutine context and just yields.
 * The argument is placed outside of the stack so it can be obtained by bluenet after the coroutine context switch.
 *
 * @param[in] payload                            pointer to buffer with command for bluenet
 */
cs_ret_code_t microappCallback(uint8_t opcode, bluenet_io_buffer_t* io_buffer) {
	cs_ret_code_t retCode = ERR_SUCCESS;
	switch (opcode) {
		case CS_MICROAPP_CALLBACK_UPDATE_IO_BUFFER: {
			setCoroutineContext(COROUTINE_MICROAPP0, io_buffer);
			[[fallthrough]];
		}
		case CS_MICROAPP_CALLBACK_SIGNAL: {
#if DEVELOPER_OPTION_DISABLE_COROUTINE == 1
			LOGi("Do not yield");
#else
			yieldCoroutine();
#endif
			break;
		}
		default: {
			LOGi("Unknown opcode");
		}
	}
	return retCode;
}

#ifdef DEVELOPER_OPTION_USE_DUMMY_CALLBACK_IN_BLUENET
/*
 * Helper function that does not go back and forth to the microapp and can be used to test if the coroutines and
 * callbacks are working properly without a microapp present.
 */
void microappCallbackDummy() {
	static int first_time = true;
	while (1) {
		// just some random data array
		bluenet_io_buffer_t io_buffer;
		// fake setup and loop commands
		if (first_time) {
			io_buffer.microapp2bluenet.payload[0] = CS_MICROAPP_COMMAND_SETUP_END;
			first_time                            = false;
		}
		else {
			io_buffer.microapp2bluenet.payload[0] = CS_MICROAPP_COMMAND_LOOP_END;
			// data[0] = CS_MICROAPP_COMMAND_LOOP_END;
		}
		// Get the ram data of ourselves (IPC_INDEX_CROWNSTONE_APP).
		uint8_t rd_size = 0;
		bluenet2microapp_ipcdata_t ipc_data;
		getRamData(IPC_INDEX_CROWNSTONE_APP, (uint8_t*)&ipc_data, sizeof(bluenet2microapp_ipcdata_t), &rd_size);

		// Perform the actual callback. Should call microappCallback and yield.
		ipc_data.microappCallback(CS_MICROAPP_CALLBACK_UPDATE_IO_BUFFER, &io_buffer);
	}
}
#endif

void jumpToAddress(uintptr_t address) {
#ifdef DEVELOPER_OPTION_OVERWRITE_FUNCTION_CALL_ADDRESS
	address = DEVELOPER_OPTION_OVERWRITE_FUNCTION_CALL_ADDRESS;
#endif
#if DEVELOPER_OPTION_USE_FUNCTION_CALL == 1
	// You have to know which function you jump to, here we assume a function returning void and no arguments.
	// We also have to add a 1 for thumb mode
	void (*func)() = (void (*)())(address + 1);
	LOGi("Call function at address: 0x%p", func);
	(*func)();
#else
	void* ptr = (void*)address;
	LOGi("Jump to address: %p", ptr);
	goto* ptr;
	LOGi("Shouldn't end up here");
#endif
}

/*
 * Jump into the microapp (this function should be called as a coroutine). It obtains the very first instruction from
 * the coroutine arguments. It then considers that instruction to be a method without arguments and calls it. An
 * incorrectly written microapp might crash the firmware here. Henceforth, before this moment it must be written to
 * flash that we try to start the microapp. If we get a reboot and see the "try to start" state, we can disable the
 * microapp forever. Alternatively, if the function never yields, it will trip the watchdog. If the watchdog is
 * triggered, we might presume that a microapp was the reason for it and disable it.
 *
 * @param[in] void *p                            pointer to a coroutine_args_t struct
 */
void goIntoMicroapp(void* p) {

#if DEVELOPER_OPTION_USE_DUMMY_CALLBACK_IN_BLUENET == 1
	LOGi("Call main: %p", &microappCallbackDummy);
	microappCallbackDummy();
#else
	coroutine_args_t* coargs = (coroutine_args_t*)p;
	LOGi("Within coroutine, call main: %p", coargs->entry);
	jumpToAddress(coargs->entry);
#endif
	// The coroutine should never return. Incorrectly written microapp!
	LOGe("Coroutine should never return. We should not come here!");
}

}  // extern C

/*
 * MicroappController constructor zero-initializes most fields and makes sure the instance can receive messages through
 * deriving from EventListener and adding itself to the EventDispatcher as listener.
 */
MicroappController::MicroappController() : EventListener(), _callCounter(0), _microappIsScanning(false) {

	EventDispatcher::getInstance().addListener(this);

	for (int i = 0; i < MICROAPP_MAX_PIN_ISR_COUNT; ++i) {
		_pinIsr[i].pin = 0;
	}
	for (int i = 0; i < MICROAPP_MAX_BLE_ISR_COUNT; ++i) {
		_bleIsr[i].id = 0;
	}

	LOGi("Microapp end is at %p", g_RAM_MICROAPP_END);
}

/*
 * Set the microappCallback in the IPC ram data bank. At a later time it can be used by the microapp to find the
 * address of microappCallback to call back into the bluenet code.
 */
void MicroappController::setIpcRam() {
	LOGi("Set IPC info for microapp");
	bluenet2microapp_ipcdata_t bluenet2microapp;
	bluenet2microapp.protocol         = 1;
	bluenet2microapp.length           = sizeof(bluenet2microapp_ipcdata_t);
	bluenet2microapp.microappCallback = microappCallback;

	LOGi("Set callback to %p", bluenet2microapp.microappCallback);

	[[maybe_unused]] uint32_t retCode =
			setRamData(IPC_INDEX_CROWNSTONE_APP, (uint8_t*)&bluenet2microapp, bluenet2microapp.length);
	LOGi("Set ram data for microapp, retCode=%u", retCode);
}

/*
 * Checks flash boundaries (for single microapp).
 */
cs_ret_code_t MicroappController::checkFlashBoundaries(uint8_t appIndex, uintptr_t address) {
	uintptr_t memoryMicroappOffset = (g_FLASH_MICROAPP_PAGES * 0x1000) * appIndex;
	uintptr_t addressLow           = g_FLASH_MICROAPP_BASE + memoryMicroappOffset;
	uintptr_t addressHigh          = addressLow + g_FLASH_MICROAPP_PAGES * 0x1000;
	if (address < addressLow) {
		return ERR_UNSAFE;
	}
	if (address > addressHigh) {
		return ERR_UNSAFE;
	}
	return ERR_SUCCESS;
}

/**
 * Clear memory. Should be done in ResetHandler, but we don't want to rely on it.
 *
 * TODO: If this is actually necessary, also check if .data is actually properly copied.
 * TODO: This should be initialized per microapp.
 */
uint16_t MicroappController::initMemory(uint8_t appIndex) {
	LOGi("Init memory: clear 0x%p to 0x%p", g_RAM_MICROAPP_BASE, g_RAM_MICROAPP_BASE + g_RAM_MICROAPP_AMOUNT);
	for (uint32_t i = 0; i < g_RAM_MICROAPP_AMOUNT; ++i) {
		uint32_t* const val = (uint32_t*)(uintptr_t)(g_RAM_MICROAPP_BASE + i);
		*val                = 0;
	}
	return ERR_SUCCESS;
}

/*
 * Gets the first instruction for the microapp (this is written in its header). We correct for thumb and check its
 * boundaries. Then we call it from a coroutine context and expect it to yield.
 *
 * @param[in]                                    appIndex (currently ignored)
 */
void MicroappController::callApp(uint8_t appIndex) {
	LOGi("Call microapp #%i", appIndex);

	initMemory(appIndex);

	uintptr_t address = MicroappStorage::getInstance().getStartInstructionAddress(appIndex);
	LOGi("Microapp: start at %p", address);

	if (checkFlashBoundaries(appIndex, address) != ERR_SUCCESS) {
		LOGe("Address not within microapp flash boundaries");
		return;
	}

	// The entry function is this immediate address (no correction for thumb mode)
	sharedState.entry = address;

	// Write coroutine as argument in the struct so we can yield from it in the context of the microapp stack
	LOGi("Setup coroutine and configure it");
	startCoroutine(&_coroutine, goIntoMicroapp, &sharedState);
}

/*
 * Get incoming microapp buffer (from coroutine_args).
 */
uint8_t* MicroappController::getInputMicroappBuffer() {
	uint8_t* payload = sharedState.io_buffer->microapp2bluenet.payload;
	return payload;
}

/*
 * Get outgoing microapp buffer (from coroutine_args).
 */
uint8_t* MicroappController::getOutputMicroappBuffer() {
	uint8_t* payload = sharedState.io_buffer->bluenet2microapp.payload;
	return payload;
}

/*
 * Called from cs_Microapp every time tick. The microapp does not set anything in RAM but will only read from RAM and
 * call a handler.
 *
 * There's no load failure detection. When the call fails bluenet hangs and should reboot.
 */
void MicroappController::tickMicroapp(uint8_t appIndex) {
	bool callAgain = false;
	_tickCounter++;
	if (_tickCounter % MICROAPP_LOOP_FREQUENCY != 0) {
		return;
	}
	_tickCounter = 0;
	LOGv("Tick microapp");
	_softInterruptCounter = 0;
	do {
		callMicroapp();
		callAgain = retrieveCommand();
	} while (callAgain);
}

/*
 * Retrieve command from the microapp. This is within the crownstone coroutine context. It gets the payload from the
 * global `sharedState` variable and calls `handleMicroappCommand` afterwards. The `sharedState.microapp2bluenet buffer`
 * only stores a single command. This is fine because after each command the microapp should transfer control to
 * bluenet. For each command is decided in `stopAfterMicroappCommand` if the microapp should be called again. For
 * example, for the end of setup, loop, or delay, it is **not** immediately called again but normal bluenet execution
 * continues. In that case the microapp is called again on the next tick.
 */
bool MicroappController::retrieveCommand() {
	uint8_t* inputBuffer            = getInputMicroappBuffer();
	microapp_cmd_t* incomingMessage = reinterpret_cast<microapp_cmd_t*>(inputBuffer);

	[[maybe_unused]] static int retrieveCounter = 0;
	if (incomingMessage->cmd != CS_MICROAPP_COMMAND_LOOP_END) {
		LOGv("Retrieve and handle [%i] command %i (callback %i)",
			 ++retrieveCounter,
			 incomingMessage->cmd,
			 incomingMessage->interruptCmd);
	}
	MicroappCommandHandler& microappCommandHandler = MicroappCommandHandler::getInstance();
	microappCommandHandler.handleMicroappCommand(incomingMessage);
	bool callAgain = !stopAfterMicroappCommand(incomingMessage);
	if (!callAgain) {
		LOGv("Command %i, do not call again", incomingMessage->cmd);
	}
	return callAgain;
}

/*
 * A GPIO callback towards the microapp.
 */
void MicroappController::softInterruptGpio(uint8_t pin) {
	MicroappCommandHandler& microappCommandHandler = MicroappCommandHandler::getInstance();
	if (microappCommandHandler.softInterruptInProgress()) {
		LOGi("Interrupt in progress, ignore pin %i event", pin);
		return;
	}
	// Write pin command into the buffer.
	uint8_t* outputBuffer         = getOutputMicroappBuffer();
	microapp_pin_cmd_t* cmd  = reinterpret_cast<microapp_pin_cmd_t*>(outputBuffer);
	cmd->header.interruptCmd = CS_MICROAPP_COMMAND_PIN;
	cmd->header.ack          = false;
	cmd->header.id           = pin;
	cmd->pin                 = pin;
	// Resume microapp so it can pick up this command.
	LOGi("GPIO interrupt on virtual pin %i", cmd->pin);
	softInterrupt();
}

/*
 * Communicate mesh message towards microapp.
 */
void MicroappController::softInterruptBle(scanned_device_t* bluenetBleDevice) {

#ifdef DEVELOPER_OPTION_THROTTLE_BY_RSSI
	if (bluenetBleDevice->rssi < -50) {
		return;
	}
#endif

	MicroappCommandHandler& microappCommandHandler = MicroappCommandHandler::getInstance();
	if (microappCommandHandler.softInterruptInProgress()) {
		LOGv("Callback in progress, ignore scanned device event");
		return;
	}

	// Write bluetooth device to buffer
	uint8_t* outputBuffer                    = getOutputMicroappBuffer();
	microapp_ble_device_t* microappBleDevice = reinterpret_cast<microapp_ble_device_t*>(outputBuffer);

	bool found                     = false;
	uint8_t id                     = 0;
	enum MicroappBleEventType type = BleEventDeviceScanned;
	for (int i = 0; i < MICROAPP_MAX_BLE_ISR_COUNT; ++i) {
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

	if (bluenetBleDevice->dataSize > sizeof(microappBleDevice->data)) {
		LOGw("BLE advertisement data too large");
		return;
	}

	microappBleDevice->header.interruptCmd = CS_MICROAPP_COMMAND_BLE_DEVICE;

	// Allow microapp to map to handler with given id
	microappBleDevice->header.id = id;
	microappBleDevice->type      = type;

	// Copy address and at the same time convert from little endian to big endian
	std::reverse_copy(
			bluenetBleDevice->address, bluenetBleDevice->address + MAC_ADDRESS_LENGTH, microappBleDevice->addr);
	microappBleDevice->rssi = bluenetBleDevice->rssi;

	// Copy the data itself
	microappBleDevice->dlen = bluenetBleDevice->dataSize;
	memcpy(microappBleDevice->data, bluenetBleDevice->data, bluenetBleDevice->dataSize);

	LOGv("Incoming advertisement written to %p", microappBleDevice);
	softInterrupt();
}

/*
 * Write the callback to the microapp and have it execute it. We can have calls to bluenet within the soft interrupt.
 * Hence, we call retrieveCommand after this. Then a number of calls are executed until we reach the end of the loop
 * or until we have reached a maximum of consecutive calls (within retrieveCommand).
 *
 * Note that although we do throttle the number of consecutive calls, this does not throttle the callbacks themselves.
 *
 */
void MicroappController::softInterrupt() {
	uint8_t* outputBuffer           = getOutputMicroappBuffer();
	microapp_cmd_t* outgoingCommand = reinterpret_cast<microapp_cmd_t*>(outputBuffer);

	if (_softInterruptCounter == MAX_CALLBACKS_WITHIN_A_TICK - 1) {
		LOGv("Last callback (next one in next tick)");
	}

	if (_softInterruptCounter == MAX_CALLBACKS_WITHIN_A_TICK) {
		LOGv("Not doing another callback");
		return;
	}
	_softInterruptCounter++;

	outgoingCommand->ack                = CS_ACK_BLUENET_MICROAPP_REQUEST;
	bool callAgain                      = false;
	int8_t repeatCounter                = 0;
	static int32_t softInterruptCounter = 0;
	do {
		LOGv("Call [%i,%i,%i], within interrupt %i",
			 softInterruptCounter,
			 repeatCounter,
			 outgoingCommand->counter,
			 outgoingCommand->interruptCmd);
		callMicroapp();
		bool acked = (outgoingCommand->ack == CS_ACK_BLUENET_MICROAPP_REQ_ACK);
		if (acked) {
			LOGv("Acked interrupt");
		}
		callAgain = retrieveCommand();
		if (!acked) {
			LOGv("Not acked... Next tick again");
			callAgain = false;
		}
		repeatCounter++;
	} while (callAgain);
	softInterruptCounter++;
}

/*
 * We resume the previously started coroutine.
 */
void MicroappController::callMicroapp() {
#if DEVELOPER_OPTION_DISABLE_COROUTINE == 0
	if (nextCoroutine()) {
		return;
	}
#else
	static bool start = false;
	// just do it from current coroutine context
	if (!start) {
		start = true;
		LOGi("Call address: %p", sharedState->entry);
		jumpToAddress(sharedState->entry);
		LOGi("Microapp might have run, but this statement is never reached (no coroutine used).");
	}
	return;
#endif
	// Should only happen if microapp actually ends (and does not yield anymore).
	LOGi("End of coroutine. Should not happen.")
}

/*
 * Called upon receiving scanned BLE device, generates a soft interrupt if a slot has been registered before.
 */
void MicroappController::onDeviceScanned(scanned_device_t* dev) {
	if (!_microappIsScanning) {
		LOGv("Microapp not scanning, so not forwarding scanned device");
		return;
	}

	softInterruptBle(dev);
}

bool MicroappController::stopAfterMicroappCommand(microapp_cmd_t* cmd) {
	uint8_t command = cmd->cmd;
	bool stop       = true;
	switch (command) {
		case CS_MICROAPP_COMMAND_PIN:
		case CS_MICROAPP_COMMAND_SWITCH_DIMMER:
		case CS_MICROAPP_COMMAND_LOG:
		case CS_MICROAPP_COMMAND_SERVICE_DATA:
		case CS_MICROAPP_COMMAND_TWI:
		case CS_MICROAPP_COMMAND_BLE:
		case CS_MICROAPP_COMMAND_POWER_USAGE:
		case CS_MICROAPP_COMMAND_PRESENCE:
		case CS_MICROAPP_COMMAND_SOFT_INTERRUPT_RECEIVED:
		case CS_MICROAPP_COMMAND_MESH: {
			stop = false;
			break;
		}
		case CS_MICROAPP_COMMAND_DELAY:
		case CS_MICROAPP_COMMAND_SETUP_END:
		case CS_MICROAPP_COMMAND_LOOP_END:
		case CS_MICROAPP_COMMAND_SOFT_INTERRUPT_END:
		case CS_MICROAPP_COMMAND_SOFT_INTERRUPT_ERROR:
		case CS_MICROAPP_COMMAND_NONE: {
			stop = true;
			break;
		}
		default: {
			LOGi("Unknown command: %i", command);
			stop = true;
			break;
		}
	}
	if (stop) {
		_callCounter = 0;
	}
	else {
		if (++_callCounter % MICROAPP_MAX_NUMBER_CONSECUTIVE_MESSAGES == 0) {
			_callCounter = 0;
			LOGv("Stop because we've reached a max # of calls");
			return true;
		}
	}
	return stop;
}

/*
 * Register a slot for a GPIO pin interrupt.
 */
bool MicroappController::registerSoftInterruptSlotGpio(uint8_t pin) {
	// We register the pin at the first empty slot
	for (int i = 0; i < MICROAPP_MAX_PIN_ISR_COUNT; ++i) {
		if (!_pinIsr[i].registered) {
			_pinIsr[i].registered = true;
			_pinIsr[i].pin        = pin;
			LOGi("Register interrupt slot for microapp pin %i", pin);
			return true;
		}
	}
	LOGw("Could not register interrupt slot for microapp pin %i", pin);
	return false;
}

/*
 * Register a slot for a BLE soft interrupt.
 */
bool MicroappController::registerSoftInterruptSlotBle(uint8_t id) {
	for (int i = 0; i < MICROAPP_MAX_BLE_ISR_COUNT; ++i) {
		if (!_bleIsr[i].registered) {
			_bleIsr[i].registered = true;
			_bleIsr[i].type       = BleEventDeviceScanned;
			_bleIsr[i].id         = id;
			LOGi("Registered slot %i for BLE events towards the microapp", id);
			return true;
		}
	}
	// no empty slot available
	LOGw("Could not register slot for BLE events for the microapp %i", id);
	return false;
}

void MicroappController::setScanning(bool scanning) {
	_microappIsScanning = scanning;
}

/**
 * Listen to events from the microapp with respect to initialization (registering soft interrupt service routines) and
 * get the interrupts for those routines and forward them.
 */
void MicroappController::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_GPIO_INIT: {
			auto gpio = CS_TYPE_CAST(EVT_GPIO_INIT, event.data);
			if (gpio->direction == INPUT || gpio->direction == SENSE) {
				registerSoftInterruptSlotGpio(gpio->pin_index);
			}
			break;
		}
		case CS_TYPE::EVT_GPIO_UPDATE: {
			auto gpio = CS_TYPE_CAST(EVT_GPIO_UPDATE, event.data);
			softInterruptGpio(gpio->pin_index);
			break;
		}
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			auto dev = CS_TYPE_CAST(EVT_DEVICE_SCANNED, event.data);
			onDeviceScanned(dev);
			break;
		}
		case CS_TYPE::EVT_MICROAPP_BLE_FILTER_INIT: {
			auto ble = CS_TYPE_CAST(EVT_MICROAPP_BLE_FILTER_INIT, event.data);
			registerSoftInterruptSlotBle(ble->index);
			break;
		}
		default: break;
	}
}
