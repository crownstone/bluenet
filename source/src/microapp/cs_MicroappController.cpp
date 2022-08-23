/**
 *
 * Microapp protocol.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: April 4, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_AutoConfig.h>
#include <common/cs_Types.h>
#include <cs_MemoryLayout.h>
#include <events/cs_EventDispatcher.h>
#include <ipc/cs_IpcRamData.h>
#include <logging/cs_Logger.h>
#include <microapp/cs_MicroappRequestHandler.h>
#include <microapp/cs_MicroappController.h>
#include <microapp/cs_MicroappStorage.h>
#include <protocol/cs_ErrorCodes.h>

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

/*
 * Enable throttling of BLE scanned devices by rssi.
 */
#define DEVELOPER_OPTION_THROTTLE_BY_RSSI

extern "C" {

/*
 * Set arguments for coroutine.
 */
void setCoroutineContext(uint8_t coroutineIndex, bluenet_io_buffers_t* io_buffers) {
	stack_params_t* stackParams = getStackParams();
	coroutine_args_t* args      = (coroutine_args_t*)stackParams->arg;
	switch (coroutineIndex) {
		case COROUTINE_MICROAPP0: args->io_buffers = io_buffers; break;
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
microapp_sdk_result_t microappCallback(uint8_t opcode, bluenet_io_buffers_t* io_buffers) {
	switch (opcode) {
		case CS_MICROAPP_CALLBACK_UPDATE_IO_BUFFER: {
			setCoroutineContext(COROUTINE_MICROAPP0, io_buffers);
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
	return CS_MICROAPP_SDK_ACK_SUCCESS;
}

#ifdef DEVELOPER_OPTION_USE_DUMMY_CALLBACK_IN_BLUENET
/*
 * Helper function that does not go back and forth to the microapp and can be used to test if the coroutines and
 * callbacks are working properly without a microapp present.
 */
void microappCallbackDummy() {
	while (1) {
		// just some random data array
		bluenet_io_buffers_t io_buffers;
		// fake setup or loop yields
		io_buffers.microapp2bluenet.payload[0] = CS_MICROAPP_SDK_TYPE_YIELD;
		// Get the ram data of ourselves (IPC_INDEX_CROWNSTONE_APP).
		uint8_t rd_size                        = 0;
		bluenet2microapp_ipcdata_t ipc_data;
		getRamData(IPC_INDEX_CROWNSTONE_APP, (uint8_t*)&ipc_data, sizeof(bluenet2microapp_ipcdata_t), &rd_size);

		// Perform the actual callback. Should call microappCallback and yield.
		ipc_data.microappCallback(CS_MICROAPP_CALLBACK_UPDATE_IO_BUFFER, &io_buffers);
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
MicroappController::MicroappController()
		: EventListener()
		, _tickCounter(0)
		, _softInterruptCounter(0)
		, _consecutiveMicroappCallCounter(0)
		, _microappIsScanning(false) {

	EventDispatcher::getInstance().addListener(this);

	LOGi("Microapp end is at %p", microappRamSection._end);
}

/*
 * Get from digital pin to interrupt.
 */
int MicroappController::digitalPinToInterrupt(int pin) {
	return pin;
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
	if (appIndex >= g_MICROAPP_COUNT) {
		return ERR_UNSAFE;
	}
	uint32_t microappSize          = microappFlashSection._size / g_MICROAPP_COUNT;
	uintptr_t memoryMicroappOffset = microappSize * appIndex;
	uintptr_t addressLow           = microappFlashSection._start + memoryMicroappOffset;
	uintptr_t addressHigh          = addressLow + microappSize;
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
	LOGi("Init memory: clear 0x%p to 0x%p", microappRamSection._start, microappRamSection._end);
	for (uint32_t i = 0; i < microappRamSection._size; ++i) {
		uint32_t* const val = (uint32_t*)(uintptr_t)(microappRamSection._start + i);
		*val                = 0;
	}
	return ERR_SUCCESS;
}

/*
 * Gets the first instruction for the microapp (this is written in its header). We correct for thumb and check its
 * boundaries. Then we call it from a coroutine context and expect it to yield.
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

uint8_t* MicroappController::getInputMicroappBuffer() {
	uint8_t* payload = sharedState.io_buffers->microapp2bluenet.payload;
	return payload;
}

uint8_t* MicroappController::getOutputMicroappBuffer() {
	uint8_t* payload = sharedState.io_buffers->bluenet2microapp.payload;
	return payload;
}

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

bool MicroappController::handleAck() {
	uint8_t* outputBuffer                 = getOutputMicroappBuffer();
	microapp_sdk_header_t* outgoingHeader = reinterpret_cast<microapp_sdk_header_t*>(outputBuffer);
	LOGv("handleAck: [ack %i]", outgoingHeader->ack);
	bool inInterruptContext               = (outgoingHeader->ack != CS_MICROAPP_SDK_ACK_NO_REQUEST);
	if (inInterruptContext) {
		bool interruptDone = (outgoingHeader->ack != CS_MICROAPP_SDK_ACK_IN_PROGRESS);
		if (interruptDone) {
			bool interruptDropped = (outgoingHeader->ack == CS_MICROAPP_SDK_ACK_ERR_BUSY);
			if (interruptDropped) {
				LOGi("Microapp is busy, drop interrupt");
				// Also prevent new interrupts since apparently the microapp has no more space
				setEmptySoftInterruptSlots(0);
			}
			else {
				LOGv("Finished interrupt with return code %i", outgoingHeader->ack);
				// Increment number of empty interrupt slots since we just finished one
				incrementEmptySoftInterruptSlots();
			}
			// If interrupt finished, we do not call again and we also don't handle the microapp request
			_consecutiveMicroappCallCounter = 0;
			return false;
		}
	}
	return true;
}

bool MicroappController::handleRequest() {
	uint8_t* inputBuffer                        = getInputMicroappBuffer();
	microapp_sdk_header_t* incomingHeader       = reinterpret_cast<microapp_sdk_header_t*>(inputBuffer);
	[[maybe_unused]] static int retrieveCounter = 0;
	LOGv("Retrieve and handle [%i] request %i", ++retrieveCounter, incomingHeader->messageType);
	MicroappRequestHandler& microappRequestHandler = MicroappRequestHandler::getInstance();
	cs_ret_code_t result                           = microappRequestHandler.handleMicroappRequest(incomingHeader);
	if (result != ERR_SUCCESS) {
		LOGi("Handling request of type %i failed with return code %i", result);
	}
	bool callAgain = !stopAfterMicroappRequest(incomingHeader);
	if (!callAgain) {
		LOGv("Do not call again");
		_consecutiveMicroappCallCounter = 0;
	}
	// Also check if the max number of consecutive nonyielding calls is reached
	else {
		if (++_consecutiveMicroappCallCounter >= MICROAPP_MAX_NUMBER_CONSECUTIVE_CALLS) {
			_consecutiveMicroappCallCounter = 0;
			LOGi("Stop because we've reached a max # of consecutive calls");
			callAgain = false;
		}
	}
	return callAgain;
}

bool MicroappController::stopAfterMicroappRequest(microapp_sdk_header_t* incomingHeader) {
	bool stop;
	switch (incomingHeader->messageType) {
		case CS_MICROAPP_SDK_TYPE_LOG:
		case CS_MICROAPP_SDK_TYPE_PIN:
		case CS_MICROAPP_SDK_TYPE_SWITCH:
		case CS_MICROAPP_SDK_TYPE_SERVICE_DATA:
		case CS_MICROAPP_SDK_TYPE_TWI:
		case CS_MICROAPP_SDK_TYPE_BLE:
		case CS_MICROAPP_SDK_TYPE_MESH:
		case CS_MICROAPP_SDK_TYPE_POWER_USAGE:
		case CS_MICROAPP_SDK_TYPE_PRESENCE:
		case CS_MICROAPP_SDK_TYPE_CONTROL_COMMAND: {
			stop = false;
			break;
		}
		case CS_MICROAPP_SDK_TYPE_NONE:
		case CS_MICROAPP_SDK_TYPE_YIELD:
		case CS_MICROAPP_SDK_TYPE_CONTINUE: {
			stop = true;
			break;
		}
		default: {
			LOGi("Unknown request type: %i", incomingHeader->messageType);
			stop = true;
			break;
		}
	}
	return stop;
}

/*
 * Called from cs_Microapp every time tick. The microapp does not set anything in RAM but will only read from RAM and
 * call a handler.
 *
 * There's no load failure detection. When the call fails bluenet hangs and should reboot.
 */
void MicroappController::tickMicroapp(uint8_t appIndex) {
	_tickCounter++;
	if (_tickCounter < MICROAPP_LOOP_FREQUENCY) {
		return;
	}
	_tickCounter = 0;
	// Reset interrupt counter every microapp tick
	_softInterruptCounter                      = 0;
	// Indicate to the microapp that this is a tick entry by writing in outgoing message header
	uint8_t* outputBuffer                  = getOutputMicroappBuffer();
	microapp_sdk_header_t* outgoingMessage = reinterpret_cast<microapp_sdk_header_t*>(outputBuffer);

	outgoingMessage->messageType           = CS_MICROAPP_SDK_TYPE_CONTINUE;
	outgoingMessage->ack                   = CS_MICROAPP_SDK_ACK_NO_REQUEST;
	bool callAgain                         = false;
	bool ignoreRequest                     = false;
	int8_t repeatCounter                   = 0;
	do {
		LOGv("tickMicroapp [call %i]", repeatCounter);
		callMicroapp();
		ignoreRequest = !handleAck();
		if (ignoreRequest) {
			return;
		}
		callAgain = handleRequest();
		repeatCounter++;
	} while (callAgain);
	LOGv("tickMicroapp end");
}

/*
 * Write the callback to the microapp and have it execute it. We can have calls to bluenet within the interrupt.
 * Hence, we call handleRequest after this if the interrupt is not finished.
 * Note that although we do throttle the number of consecutive calls, this does not throttle the callbacks themselves.
 */
void MicroappController::generateSoftInterrupt() {
	// This is probably already checked before this function call, but let's do it anyway to be sure
	if (!allowSoftInterrupts()) {
		return;
	}
	if (_softInterruptCounter == MICROAPP_MAX_SOFT_INTERRUPTS_WITHIN_A_TICK - 1) {
		LOGv("Last callback (next one in next tick)");
	}
	_softInterruptCounter++;

	uint8_t* outputBuffer                    = getOutputMicroappBuffer();
	microapp_sdk_header_t* outgoingInterrupt = reinterpret_cast<microapp_sdk_header_t*>(outputBuffer);

	// Request an acknowledgement by the microapp indicating status of interrupt
	outgoingInterrupt->ack                   = CS_MICROAPP_SDK_ACK_REQUEST;
	bool callAgain                           = false;
	bool ignoreRequest                       = false;
	int8_t repeatCounter                     = 0;
	do {
		LOGv("generateSoftInterrupt [call %i, %i interrupts within tick]", repeatCounter, _softInterruptCounter);
		callMicroapp();
		ignoreRequest = !handleAck();
		if (ignoreRequest) {
			return;
		}
		callAgain = handleRequest();
		repeatCounter++;
	} while (callAgain);
	LOGv("generateSoftInterrupt end");
}

/*
 * Do some checks to validate if we want to generate an interrupt for the pin event
 * and if so, prepare the outgoing buffer and call generateSoftInterrupt()
 */
void MicroappController::onGpioUpdate(uint8_t pinIndex) {
	if (!allowSoftInterrupts()) {
		LOGv("New interrupts blocked, ignore pin event");
		return;
	}
	uint8_t interruptPin = digitalPinToInterrupt(pinIndex);
	if (!softInterruptRegistered(CS_MICROAPP_SDK_TYPE_PIN, interruptPin)) {
		LOGv("No interrupt registered for virtual pin %d", interruptPin);
		return;
	}
	// Write pin data into the buffer.
	uint8_t* outputBuffer   = getOutputMicroappBuffer();
	microapp_sdk_pin_t* pin = reinterpret_cast<microapp_sdk_pin_t*>(outputBuffer);
	pin->header.messageType = CS_MICROAPP_SDK_TYPE_PIN;
	pin->pin                = interruptPin;

	LOGv("Incoming GPIO interrupt for microapp on virtual pin %i", interruptPin);
	generateSoftInterrupt();
}

/*
 * Do some checks to validate if we want to generate an interrupt for the scanned device
 * and if so, prepare the outgoing buffer and call generateSoftInterrupt()
 */
void MicroappController::onDeviceScanned(scanned_device_t* dev) {
	if (!_microappIsScanning) {
		// Microapp not scanning, so not forwarding scanned device
		return;
	}

#ifdef DEVELOPER_OPTION_THROTTLE_BY_RSSI
	// Throttle by rssi by default to limit number of scanned devices forwarded
	if (dev->rssi < -50) {
		return;
	}
#endif

	if (!allowSoftInterrupts()) {
		LOGv("New interrupts blocked, ignore ble device event");
		return;
	}
	if (!softInterruptRegistered(CS_MICROAPP_SDK_TYPE_BLE, CS_MICROAPP_SDK_BLE_SCAN_SCANNED_DEVICE)) {
		LOGv("No interrupt registered");
		return;
	}

	// Write bluetooth device to buffer
	uint8_t* outputBuffer   = getOutputMicroappBuffer();
	microapp_sdk_ble_t* ble = reinterpret_cast<microapp_sdk_ble_t*>(outputBuffer);

	if (dev->dataSize > sizeof(ble->data)) {
		LOGw("BLE advertisement data too large");
		return;
	}

	ble->header.messageType = CS_MICROAPP_SDK_TYPE_BLE;
	ble->type               = CS_MICROAPP_SDK_BLE_SCAN_SCANNED_DEVICE;
	ble->address_type       = dev->addressType;
	std::reverse_copy(dev->address, dev->address + MAC_ADDRESS_LENGTH, ble->address);
	ble->rssi = dev->rssi;
	ble->size = dev->dataSize;
	memcpy(ble->data, dev->data, dev->dataSize);

	LOGv("Incoming BLE scanned device for microapp");
	generateSoftInterrupt();
}

/*
 * Do some checks to validate if we want to generate an interrupt for the received message
 * and if so, prepare the outgoing buffer and call generateSoftInterrupt()
 */
void MicroappController::onReceivedMeshMessage(MeshMsgEvent* event) {
	if (event->type != CS_MESH_MODEL_TYPE_MICROAPP) {
		// Mesh message received, but not for the microapp.
		return;
	}
	if (event->isReply) {
		// We received the empty reply.
		return;
	}
	if (event->reply != nullptr) {
		// Send an empty reply.
		event->reply->type     = CS_MESH_MODEL_TYPE_MICROAPP;
		event->reply->dataSize = 0;
	}
	if (!allowSoftInterrupts()) {
		LOGv("New interrupts blocked, ignore mesh event");
		return;
	}
	if (!softInterruptRegistered(CS_MICROAPP_SDK_TYPE_MESH, CS_MICROAPP_SDK_MESH_READ)) {
		LOGv("No interrupt registered");
		return;
	}
	// Write mesh message to buffer
	uint8_t* outputBuffer        = getOutputMicroappBuffer();
	microapp_sdk_mesh_t* meshMsg = reinterpret_cast<microapp_sdk_mesh_t*>(outputBuffer);

	meshMsg->header.messageType  = CS_MICROAPP_SDK_TYPE_MESH;
	meshMsg->type                = CS_MICROAPP_SDK_MESH_READ;
	meshMsg->stoneId             = event->srcStoneId;
	meshMsg->size                = event->msg.len;
	memcpy(meshMsg->data, event->msg.data, event->msg.len);

	LOGv("Incoming mesh message for microapp");
	generateSoftInterrupt();
}

/*
 * Attempt registration of an interrupt. An interrupt registration is uniquely identified
 * by a type (=messageType, see enum MicroappSdkMessageType) and an id which identifies the interrupt
 * registration within the scope of the type.
 */
cs_ret_code_t MicroappController::registerSoftInterrupt(MicroappSdkMessageType type, uint8_t id) {
	// Check if interrupt registration already exists
	if (softInterruptRegistered(type, id)) {
		LOGi("Interrupt [%i, %i] already registered", type, id);
		return ERR_ALREADY_EXISTS;
	}
	// Look for first empty slot, if it exists
	int emptySlotIndex = -1;
	for (int i = 0; i < MICROAPP_MAX_SOFT_INTERRUPT_REGISTRATIONS; ++i) {
		if (!_softInterruptRegistrations[i].registered) {
			emptySlotIndex = i;
			break;
		}
	}
	if (emptySlotIndex < 0) {
		LOGw("No empty interrupt registration slots left");
		return ERR_NO_SPACE;
	}
	// Register the interrupt
	_softInterruptRegistrations[emptySlotIndex].registered = true;
	_softInterruptRegistrations[emptySlotIndex].type       = type;
	_softInterruptRegistrations[emptySlotIndex].id         = id;

	LOGv("Registered soft interrupt of type %i, id %i", type, id);

	return ERR_SUCCESS;
}

/*
 * Check whether an interrupt registration already exists
 */
bool MicroappController::softInterruptRegistered(MicroappSdkMessageType type, uint8_t id) {
	for (int i = 0; i < MICROAPP_MAX_SOFT_INTERRUPT_REGISTRATIONS; ++i) {
		if (_softInterruptRegistrations[i].registered) {
			if (_softInterruptRegistrations[i].type == type && _softInterruptRegistrations[i].id == id) {
				return true;
			}
		}
	}
	return false;
}

/*
 * Check whether new interrupts can be generated
 */
bool MicroappController::allowSoftInterrupts() {
	// if the microapp dropped the last one and hasn't finished an interrupt,
	// we won't try to call it with a new interrupt
	if (_emptySoftInterruptSlots <= 0) {
		return false;
	}
	// Check if we already exceeded the max number of interrupts in this tick
	if (_softInterruptCounter >= MICROAPP_MAX_SOFT_INTERRUPTS_WITHIN_A_TICK) {
		return false;
	}
	return true;
}

/*
 * Set the number of empty interrupt slots.
 * This function can be used upon microapp yield requests, which contain an emptyInterruptSlots field
 */
void MicroappController::setEmptySoftInterruptSlots(uint8_t emptySlots) {
	_emptySoftInterruptSlots = emptySlots;
}

/*
 * Increment the number of empty interrupt slots.
 * This function can be used when the microapp finishes handling an interrupt.
 * That means a slot will have been freed at the microapp side.
 */
void MicroappController::incrementEmptySoftInterruptSlots() {
	// Make sure we don't overflow to zero in extreme cases
	if (_emptySoftInterruptSlots == 0xFF) {
		return;
	}
	_emptySoftInterruptSlots++;
}

/*
 * Set the internal scanning flag
 */
void MicroappController::setScanning(bool scanning) {
	_microappIsScanning = scanning;
}

/**
 * Listen to events from the microapp that are coupled with (possibly) registered interrupts
 */
void MicroappController::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_GPIO_UPDATE: {
			auto gpio = CS_TYPE_CAST(EVT_GPIO_UPDATE, event.data);
			onGpioUpdate(gpio->pin_index);
			break;
		}
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			auto dev = CS_TYPE_CAST(EVT_DEVICE_SCANNED, event.data);
			onDeviceScanned(dev);
			break;
		}
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto msg = CS_TYPE_CAST(EVT_RECV_MESH_MSG, event.data);
			onReceivedMeshMessage(msg);
			break;
		}
		default: break;
	}
}
