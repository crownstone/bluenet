#pragma once

#include <ble/cs_BleConstants.h>
#include <cs_MicroappStructs.h>
#include <events/cs_EventListener.h>
#include <ipc/cs_IpcRamData.h>
#include <protocol/cs_Packets.h>
#include <protocol/cs_Typedefs.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <services/cs_MicroappService.h>

extern "C" {
#include <util/cs_DoubleStackCoroutine.h>
}

static_assert(sizeof(bluenet2microapp_ipcdata_t) <= BLUENET_IPC_RAM_DATA_ITEM_SIZE);

// Do some asserts on the redefinitions in the shared header files
// These asserts cannot be done where declared since the shared files do not depend on the rest of bluenet
static_assert(MAC_ADDRESS_LENGTH == MAC_ADDRESS_LEN);
static_assert(MAX_BLE_ADV_DATA_LENGTH == ADVERTISEMENT_DATA_MAX_SIZE);
static_assert(MAX_MICROAPP_MESH_PAYLOAD_SIZE == MAX_MESH_MSG_PAYLOAD_SIZE);

static_assert(CS_MICROAPP_SDK_SWITCH_OFF == CS_SWITCH_CMD_VAL_OFF);
static_assert(CS_MICROAPP_SDK_SWITCH_ON == CS_SWITCH_CMD_VAL_FULLY_ON);
static_assert(CS_MICROAPP_SDK_SWITCH_TOGGLE == CS_SWITCH_CMD_VAL_TOGGLE);
static_assert(CS_MICROAPP_SDK_SWITCH_BEHAVIOUR == CS_SWITCH_CMD_VAL_BEHAVIOUR);
static_assert(CS_MICROAPP_SDK_SWITCH_SMART_ON == CS_SWITCH_CMD_VAL_SMART_ON);

/**
 * The IPC buffers can be used to bootstrap communication between microapp and bluenet. However, when in the microapp
 * coroutine context it is convenient if we can immediately yield towards the other context. For this we reserve a bit
 * of space on the stack (apart from stack pointer etc.).
 */
struct microapp_coroutine_args_t {
	uintptr_t entry;
	bluenet_io_buffers_t* ioBuffers;
};

/**
 * @struct microapp_soft_interrupt_registration_t
 * Struct for keeping track of registered interrupts from the microapp
 *
 * @var microapp_soft_interrupt_registration_t::registered indicates whether a registration slot is filled.
 * @var microapp_soft_interrupt_registration_t::type       indicates the message type for the interrupt
 * @var microapp_soft_interrupt_registration_t::id         unique identifier for interrupt registrations
 */
struct microapp_soft_interrupt_registration_t {
	bool registered             = false;
	MicroappSdkMessageType type = CS_MICROAPP_SDK_TYPE_NONE;
	uint8_t id                  = 0;
};

/**
 * Operating state of a microapp. For now binary, either running or not running.
 */
enum class MicroappOperatingState {
	CS_MICROAPP_NOT_RUNNING,
	CS_MICROAPP_RUNNING,
};

/**
 * Keeps up data for a microapp.
 */
struct microapp_data_t {
	//! Keeps up whether the microapp is scanning, thus whether BLE scans should generate interrupts.
	bool isScanning  = false;

	//! Keeps up the microapp service.
	Service* service = nullptr;
};

/**
 * The class MicroappController has functionality to store a second app (and perhaps in the future even more apps) on
 * another part of the flash memory.
 */
class MicroappController {
private:
	/**
	 * Singleton, constructor, also copy constructor, is private.
	 */
	MicroappController();
	MicroappController(MicroappController const&);
	void operator=(MicroappController const&);

	/**
	 * Limit the number of interrupts in a tick. (if -1 there is no limit)
	 */
	static const int8_t MICROAPP_MAX_SOFT_INTERRUPTS_WITHIN_A_TICK = 10;

	/**
	 * The maximum number of consecutive calls to a microapp
	 */
	static const uint8_t MICROAPP_MAX_NUMBER_CONSECUTIVE_CALLS     = 8;

	/**
	 * The maximum number of registered interrupts
	 */
	static const uint8_t MICROAPP_MAX_SOFT_INTERRUPT_REGISTRATIONS = 10;

	/**
	 * Buffer for keeping track of registered interrupts
	 */
	microapp_soft_interrupt_registration_t _softInterruptRegistrations[MICROAPP_MAX_SOFT_INTERRUPT_REGISTRATIONS];

	/*
	 * Shared state to both the microapp and the bluenet code. This is used as an argument to the coroutine. It can
	 * later be used to get information back and forth between microapp and bluenet.
	 */

	/**
	 * To throttle the ticks themselves
	 */
	uint8_t _tickCounter                    = 0;

	/**
	 * Counter for interrupts within a tick. Limited to MICROAPP_MAX_SOFT_INTERRUPTS_WITHIN_A_TICK
	 */
	int8_t _softInterruptCounter            = 0;

	/**
	 * Counter for consecutive microapp calls. Limited to MICROAPP_MAX_NUMBER_CONSECUTIVE_CALLS
	 */
	uint8_t _consecutiveMicroappCallCounter = 0;

	/**
	 * Keeps track of how many empty interrupt slots are available on the microapp side.
	 * Start with 1, but will be set to the correct value in the request handler.
	 */
	uint8_t _emptySoftInterruptSlots        = 1;

	/**
	 * Set operating state in IPC ram. This can be used after (an accidental) boot to decide if a microapp has been
	 * the reason for that reboot. In that case the microapp can be disabled so not to cause more havoc.
	 *
	 * @param[in] appIndex   Currently, only appIndex 0 is supported.
	 * @param[in] state      The state (running or not running) of this microapp.
	 */
	void setOperatingState(uint8_t appIndex, MicroappOperatingState state);

protected:
	/**
	 * Resume the previously started coroutine
	 */
	void callMicroapp();

	/**
	 * Retrieve ack from the outgoing buffer that the microapp may have overwritten.
	 * Particularly check if the microapp exit was on finishing an interrupt.
	 * In that case, ignore the request made by the microapp
	 *
	 * @return true     if the request in the incoming buffer should be handled
	 * @return false    if the request in the incoming buffer should be ignored
	 */
	bool handleAck();

	/**
	 * Retrieve request from the microapp and let MicroappRequestHandler handle it.
	 * Return whether the microapp should be called again immediately or not.
	 * This depends on both the type of request (i.e. for YIELDs do not call again)
	 * and on whether the max number of consecutive calls has been reached
	 *
	 * @return true     if the microapp should be called again
	 * @return false    if the microapp should not be called again
	 */
	bool handleRequest();

	/**
	 * After particular microapp requests we want to stop the microapp (end of loop etc.) and continue with bluenet.
	 * This function returns true for such requests.
	 *
	 * @param[in] header     Header of the microapp request
	 *
	 * @return true          if the microapp is yielding
	 * @return false         if the microapp is not yielding
	 */
	bool stopAfterMicroappRequest(microapp_sdk_header_t* header);

	/**
	 * Check if start address of the microapp is within the flash boundaries assigned to the microapps.
	 */
	cs_ret_code_t checkFlashBoundaries(uint8_t appIndex, uintptr_t address);

public:
	static MicroappController& getInstance() {
		static MicroappController instance;
		return instance;
	}

	/**
	 * Set IPC ram data.
	 */
	void setIpcRam();

	/**
	 * Initialize memory.
	 */
	uint16_t initMemory(uint8_t appIndex);

	/**
	 * Actually run the app.
	 *
	 * @param[in] appIndex (currently ignored)
	 */
	void startMicroapp(uint8_t appIndex);

	/**
	 * Tick microapp
	 */
	void tickMicroapp(uint8_t appIndex);

	/**
	 * Get incoming microapp buffer (from coargs).
	 */
	uint8_t* getInputMicroappBuffer();

	/**
	 * Get outgoing microapp buffer (from coargs).
	 */
	uint8_t* getOutputMicroappBuffer();

	/**
	 * Register interrupts that allow generation of interrupts to the microapp
	 */
	cs_ret_code_t registerSoftInterrupt(MicroappSdkMessageType type, uint8_t id);

	/**
	 * Checks whether an interrupt registration already exists
	 */
	bool isSoftInterruptRegistered(MicroappSdkMessageType type, uint8_t id);

	/**
	 * Checks whether the microapp has empty interrupt slots to deal with a new softInterrupt
	 */
	bool allowSoftInterrupts();

	/**
	 * Set the number of empty interrupt slots
	 *
	 * @param emptySlots The new value
	 */
	void setEmptySoftInterruptSlots(uint8_t emptySlots);

	/**
	 * Increment the number of empty interrupt slots
	 */
	void incrementEmptySoftInterruptSlots();

	/**
	 * Call the microapp in an interrupt context
	 */
	void generateSoftInterrupt();

	/**
	 * Get operating state from IPC ram.
	 * @param[in] appIndex   Currently, only appIndex 0 is supported.
	 */
	MicroappOperatingState getOperatingState(uint8_t appIndex);

	/**
	 * Clear a microapp state:
	 * - Remove all registered interrupts.
	 */
	void clear(uint8_t appIndex);

	/**
	 * Some runtime data we have to store for a microapp.
	 */
	microapp_data_t microappData;
};
