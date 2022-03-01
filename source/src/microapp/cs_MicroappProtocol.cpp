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
 * Shared state to both the microapp and the bluenet code. This is used as an argument to the coroutine. It can later
 * be used to get information back and forth between microapp and bluenet.
 */
coargs_t sharedState;

/*
 * Set arguments for coroutine.
 */
void setCoroutineContext(uint8_t coroutineIndex, bluenet_io_buffer_t* io_buffer) {
	stack_params_t* stack_params = getStackParams();
	coargs_t* args               = (coargs_t*)stack_params->arg;
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
cs_ret_code_t microapp_callback(bluenet_io_buffer_t* io_buffer) {
	setCoroutineContext(COROUTINE_MICROAPP0, io_buffer);
#if DEVELOPER_OPTION_DISABLE_COROUTINE == 1
	LOGi("Do not yield");
#else
	yieldCoroutine();
#endif

	return ERR_SUCCESS;
}

#ifdef DEVELOPER_OPTION_USE_DUMMY_CALLBACK_IN_BLUENET
/*
 * Helper function that does not go back and forth to the microapp and can be used to test if the coroutines and
 * callbacks are working properly without a microapp present.
 */
void microapp_callback_dummy() {
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

		// Perform the actual callback. Should call microapp_callback and in the end yield.
		// microappCallbackFunc callback_func = reinterpret_cast<microappCallbackFunc>(ipc_data.microapp_calback);
		ipc_data.microapp_callback(&io_buffer);
		// callback_func(data);
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
 * @param[in] void *p                            pointer to a coargs_t struct
 */
void goIntoMicroapp(void* p) {

#if DEVELOPER_OPTION_USE_DUMMY_CALLBACK_IN_BLUENET == 1
	LOGi("Call main: %p", &microapp_callback_dummy);
	microapp_callback_dummy();
#else
	coargs_t* coargs = (coargs_t*)p;
	LOGi("Call main: %p", coargs->entry);
	jumpToAddress(coargs->entry);
#endif
	// The coroutine should never return. Incorrectly written microapp!
	LOGe("Coroutine should never return. We should not come here!");
}

}  // extern C

/*
 * Helper functions.
 *
 * TODO: Move to appropriate files.
 */

/*
 * Get from interrupt to digital pin.
 *
 * We have one virtual device at location 0, so we just decrement the pin number by one to map to the array in the
 * GPIO module.
 */
int interruptToDigitalPin(int interrupt) {
	return interrupt - 1;
}

/*
 * Get from digital pin to interrupt. See also interruptToDigitalPin.
 */
int digitalPinToInterrupt(int pin) {
	return pin + 1;
}

/*
 * Checks flash boundaries (for single microapp).
 */
cs_ret_code_t checkFlashBoundaries(uintptr_t address) {
	if (address < g_FLASH_MICROAPP_BASE) {
		return ERR_UNSAFE;
	}
	if (address > (g_FLASH_MICROAPP_BASE + g_FLASH_MICROAPP_PAGES * 0x1000)) {
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
		, _callCounter(0)
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
	bluenet2microapp_ipcdata_t bluenet2microapp;
	bluenet2microapp.protocol          = 1;
	bluenet2microapp.length            = sizeof(bluenet2microapp_ipcdata_t);
	bluenet2microapp.microapp_callback = microapp_callback;

	LOGi("Set callback to %p", bluenet2microapp.microapp_callback);

	[[maybe_unused]] uint32_t retCode =
			setRamData(IPC_INDEX_CROWNSTONE_APP, (uint8_t*)&bluenet2microapp, bluenet2microapp.length);
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

	initMemory();

	uintptr_t address = MicroappStorage::getInstance().getStartInstructionAddress(appIndex);
	LOGi("Microapp: start at %p", address);

	if (checkFlashBoundaries(address) != ERR_SUCCESS) {
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
 * This should not be done. Call the microapp before main where all copying towards the right sections takes place.
 * That will be fine for both .bss (uninitialized) and .data (initialized) data. If this is done before the very
 * first call to the microapp, it won't hurt though.
 *
 * TODO: Check and remove this function.
 */
uint16_t MicroappProtocol::initMemory() {
	LOGi("Init memory: clear %p to %p", g_RAM_MICROAPP_BASE, g_RAM_MICROAPP_BASE + g_RAM_MICROAPP_AMOUNT);
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
	_tickCounter++;
	if (_tickCounter % MICROAPP_LOOP_FREQUENCY != 0) {
		return;
	}
	_tickCounter = 0;
	LOGv("Tick microapp");
	_callbackExecCounter = 0;
	do {
		callMicroapp();
		call_again = retrieveCommand();
	} while (call_again);
}

/*
 * Retrieve command from the microapp. This is within the crownstone coroutine context. It gets the payload from the
 * global `sharedState` variable and calls `handleMicroappCommand` afterwards. The `sharedState.microapp2bluenet buffer`
 * only stores a single command. This is fine because after each command the microapp should transfer control to
 * bluenet. For each command is decided in `stopAfterMicroappCommand` if the microapp should be called again. For
 * example, for the end of setup, loop, or delay, it is **not** immediately called again but normal bluenet execution
 * continues. In that case the microapp is called again on the next tick.
 */
bool MicroappProtocol::retrieveCommand() {
	uint8_t* payload       = sharedState.io_buffer->microapp2bluenet.payload;
	microapp_cmd_t* cmd_in = reinterpret_cast<microapp_cmd_t*>(payload);

	[[maybe_unused]] static int retrieveCounter = 0;
	if (cmd_in->cmd != CS_MICROAPP_COMMAND_LOOP_END) {
		LOGv("Retrieve and handle [%i] command %i (callback %i)", ++retrieveCounter, cmd_in->cmd, cmd_in->callbackCmd);
	}
	handleMicroappCommand(cmd_in);
	bool call_again = !stopAfterMicroappCommand(cmd_in);
	if (!call_again) {
		LOGv("Command %i, do not call again", cmd_in->cmd);
	}
	return call_again;
}

/*
 * A GPIO callback towards the microapp.
 */
void MicroappProtocol::performCallbackGpio(uint8_t pin) {
	if (callbackInProgress()) {
		LOGi("Callback in progress, ignore pin %i event", digitalPinToInterrupt(pin));
		return;
	}
	// Write pin command into the buffer.
	uint8_t* payload        = sharedState.io_buffer->bluenet2microapp.payload;
	microapp_pin_cmd_t* cmd = reinterpret_cast<microapp_pin_cmd_t*>(payload);
	cmd->header.callbackCmd = CS_MICROAPP_COMMAND_PIN;
	cmd->header.ack         = false;
	cmd->header.id          = digitalPinToInterrupt(pin);
	cmd->pin                = digitalPinToInterrupt(pin);
	cmd->value              = CS_MICROAPP_COMMAND_VALUE_CHANGE;
	// Resume microapp so it can pick up this command.
	LOGi("Send callback on virtual pin %i", cmd->pin);
	writeCallback();
}

/*
 * We either get a callback received or a callback failure when we start a callback. However, there can be a failure
 * also halfway the execution which will not lead to a proper end.
 *
 * TODO: This relies too much on proper counters for callbacks received versus callbacks ended. It should at least be
 * reset so now and then (or updated with info from the microapp itself).
 */
bool MicroappProtocol::callbackInProgress() {
	uint32_t inProgress = (_callbackReceivedCounter - _callbackEndCounter);
	return ((uint8_t)inProgress >= MAX_SOFTINTERRUPTS_IN_PARALLEL);
}

/*
 * Communicate mesh message towards microapp.
 */
void MicroappProtocol::performCallbackIncomingBLEAdvertisement(scanned_device_t* dev) {

#ifdef DEVELOPER_OPTION_THROTTLE_BY_RSSI
	if (dev->rssi < -50) {
		return;
	}
#endif

	if (callbackInProgress()) {
		LOGv("Callback in progress, ignore scanned device event");
		return;
	}

	// Write bluetooth device to buffer
	uint8_t* payload           = sharedState.io_buffer->bluenet2microapp.payload;
	microapp_ble_device_t* buf = reinterpret_cast<microapp_ble_device_t*>(payload);

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

	buf->header.callbackCmd = CS_MICROAPP_COMMAND_BLE_DEVICE;

	// Allow microapp to map to handler with given id
	buf->header.id = id;
	buf->type      = type;

	// Copy address and at the same time convert from little endian to big endian
	std::reverse_copy(dev->address, dev->address + MAC_ADDRESS_LENGTH, buf->addr);
	buf->rssi = dev->rssi;

	// Copy the data itself
	buf->dlen = dev->dataSize;
	memcpy(buf->data, dev->data, dev->dataSize);

	LOGv("Incoming advertisement written to %p", buf);
	writeCallback();
}

/*
 * Write the callback to the microapp and have it execute it. We can have calls to bluenet within the soft interrupt.
 * Hence, we call retrieveCommand after this. Then a number of calls are executed until we reach the end of the loop
 * or until we have reached a maximum of consecutive calls (within retrieveCommand).
 *
 * Note that although we do throttle the number of consecutive calls, this does not throttle the callbacks themselves.
 *
 */
void MicroappProtocol::writeCallback() {
	uint8_t* data_out                       = sharedState.io_buffer->bluenet2microapp.payload;
	microapp_cmd_t* buf_out                 = reinterpret_cast<microapp_cmd_t*>(data_out);
	[[maybe_unused]] uint8_t* data_in       = sharedState.io_buffer->microapp2bluenet.payload;
	[[maybe_unused]] microapp_cmd_t* buf_in = reinterpret_cast<microapp_cmd_t*>(data_in);

	if (_callbackExecCounter == MAX_CALLBACKS_WITHIN_A_TICK - 1) {
		LOGi("Last callback (next one in next tick)");
	}

	if (_callbackExecCounter == MAX_CALLBACKS_WITHIN_A_TICK) {
		return;
	}
	_callbackExecCounter++;

	buf_out->ack                   = CS_ACK_BLUENET_MICROAPP_REQUEST;
	bool call_again                = false;
	int8_t repeatCounter           = 0;
	static int32_t callbackCounter = 0;
	do {
		LOGv("Call [%i,%i,%i], within callback %i: (cur=%i)",
			 callbackCounter,
			 repeatCounter,
			 buf_out->counter,
			 buf_out->callbackCmd,
			 buf_in->cmd);
		callMicroapp();
		bool acked = (buf_out->ack == CS_ACK_BLUENET_MICROAPP_REQ_ACK);
		if (acked) {
			// buf_out->ack = CS_ACK_NONE;
			LOGv("Acked callback");
		}
		call_again = retrieveCommand();
		if (!acked) {
			LOGv("Not acked... Give it space");
			call_again = false;
		}
		repeatCounter++;
	} while (call_again);
	callbackCounter++;
}

/*
 * We resume the previously started coroutine.
 */
void MicroappProtocol::callMicroapp() {
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
 * Register GPIO pin
 */
bool MicroappProtocol::registerGpio(uint8_t pin) {
	// We register the pin at the first empty slot
	for (int i = 0; i < MAX_PIN_ISR_COUNT; ++i) {
		if (!_pinIsr[i].registered) {
			_pinIsr[i].registered = true;
			_pinIsr[i].pin        = pin;
			LOGi("Register callback for microapp pin %i", pin);
			return true;
		}
	}
	LOGw("Could not register callback for microapp pin %i", pin);
	return false;
}

/**
 * Required if we have to pass events through to the microapp.
 */
void MicroappProtocol::handleEvent(event_t& event) {
	switch (event.type) {
		// Listen to GPIO events, will be used later to implement attachInterrupt in microapp.
		case CS_TYPE::EVT_GPIO_INIT: {
			TYPIFY(EVT_GPIO_INIT) gpio = *(TYPIFY(EVT_GPIO_INIT)*)event.data;
			if (gpio.direction == INPUT || gpio.direction == SENSE) {
				LOGd("Register GPIO event handler for microapp");
				registerGpio(gpio.pin_index);
			}
			break;
		}
		case CS_TYPE::EVT_GPIO_UPDATE: {
			TYPIFY(EVT_GPIO_UPDATE) gpio = *(TYPIFY(EVT_GPIO_UPDATE)*)event.data;
			LOGi("GPIO callback for microapp pin %i", gpio.pin_index);
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

	performCallbackIncomingBLEAdvertisement(dev);
}

/*
 * Forwards commands from the microapp to the relevant handler
 */
cs_ret_code_t MicroappProtocol::handleMicroappCommand(microapp_cmd_t* cmd) {
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
		case CS_MICROAPP_COMMAND_CALLBACK_RECEIVED: {
			_callbackReceivedCounter++;
			LOGv("Callback received [%i] for %i", _callbackReceivedCounter, (int)cmd->id);
			break;
		}
		case CS_MICROAPP_COMMAND_CALLBACK_FAILURE: {
			_callbackFailCounter++;
			LOGv("Callback fail [%i] for %i", _callbackFailCounter, (int)cmd->id);
			break;
		}
		case CS_MICROAPP_COMMAND_CALLBACK_END: {
			_callbackEndCounter++;
			LOGv("Callback end [%i] for %i", _callbackEndCounter, (int)cmd->id);
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

bool MicroappProtocol::stopAfterMicroappCommand(microapp_cmd_t* cmd) {
	uint8_t command = cmd->cmd;
	bool stop       = true;
	switch (command) {
		case CS_MICROAPP_COMMAND_PIN:
		case CS_MICROAPP_COMMAND_LOG:
		case CS_MICROAPP_COMMAND_SERVICE_DATA:
		case CS_MICROAPP_COMMAND_TWI:
		case CS_MICROAPP_COMMAND_BLE:
		case CS_MICROAPP_COMMAND_POWER_USAGE:
		case CS_MICROAPP_COMMAND_PRESENCE:
		case CS_MICROAPP_COMMAND_CALLBACK_RECEIVED:
		case CS_MICROAPP_COMMAND_MESH: {
			stop = false;
			break;
		}
		case CS_MICROAPP_COMMAND_DELAY:
		case CS_MICROAPP_COMMAND_SETUP_END:
		case CS_MICROAPP_COMMAND_LOOP_END:
		case CS_MICROAPP_COMMAND_CALLBACK_END:
		case CS_MICROAPP_COMMAND_CALLBACK_FAILURE:
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
		if (++_callCounter % MAX_CONSECUTIVE_MESSAGES == 0) {
			_callCounter = 0;
			LOGv("Stop because we've reached a max # of calls");
			return true;
		}
	}
	return stop;
}

// TODO: establish a proper default log level for microapps
#define LOCAL_MICROAPP_LOG_LEVEL SERIAL_INFO

cs_ret_code_t MicroappProtocol::handleMicroappLogCommand(microapp_log_cmd_t* command) {

	if (command->length == 0) {
		LOGi("Incorrect length for log message");
		return ERR_WRONG_PAYLOAD_LENGTH;
	}
	__attribute__((unused)) bool newLine = false;
	if (command->option == CS_MICROAPP_COMMAND_LOG_NEWLINE) {
		newLine = true;
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
			pin_cmd->pin                      = interruptToDigitalPin(pin_cmd->pin);
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
		case CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP: gpio.pull = 1; [[fallthrough]];
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

cs_ret_code_t MicroappProtocol::handleMicroappServiceDataCommand(microapp_service_data_cmd_t* cmd) {

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
			LOGi("Set scan event callback handler");
#if BUILD_MESHING == 0
			LOGi("Scanning is done within the mesh code. No scans will be received because mesh is disabled");
#endif
			bool success        = registerBleCallback(ble_cmd->id);
			ble_cmd->header.ack = success;
			return success ? ERR_SUCCESS : ERR_NO_SPACE;
		}
		case CS_MICROAPP_COMMAND_BLE_SCAN_START: {
			LOGi("Start scanning");
			_microappIsScanning = true;
			ble_cmd->header.ack = true;
			break;
		}
		case CS_MICROAPP_COMMAND_BLE_SCAN_STOP: {
			LOGi("Stop scanning");
			_microappIsScanning = false;
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

cs_ret_code_t MicroappProtocol::handleMicroappPowerUsageCommand(microapp_power_usage_cmd_t* cmd) {
	microapp_power_usage_t* commandPayload = &cmd->powerUsage;

	TYPIFY(STATE_POWER_USAGE) powerUsage;
	State::getInstance().get(CS_TYPE::STATE_POWER_USAGE, &powerUsage, sizeof(powerUsage));

	commandPayload->powerUsage = powerUsage;

	return ERR_SUCCESS;
}

/*
 * This queries for a presence event.
 */
cs_ret_code_t MicroappProtocol::handleMicroappPresenceCommand(microapp_presence_cmd_t* cmd) {

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
			if (!broadcast) {
				eventData.idCount   = 1;
				eventData.targetIds = &(cmd->stoneId);
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

			auto message = _meshMessageBuffer.pop();

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
			LOGi("Unknown microapp mesh command opcode: %u", command->opcode);
			return ERR_UNKNOWN_OP_CODE;
		}
	}
	return ERR_SUCCESS;
}
