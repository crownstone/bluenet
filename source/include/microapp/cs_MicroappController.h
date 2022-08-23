#pragma once

#include <ble/cs_BleConstants.h>
#include <cs_MicroappStructs.h>
#include <events/cs_EventListener.h>
#include <protocol/cs_Typedefs.h>
#include <protocol/mesh/cs_MeshModelPackets.h>

extern "C" {
#include <util/cs_DoubleStackCoroutine.h>
}

static_assert(sizeof(bluenet2microapp_ipcdata_t) <= BLUENET_IPC_RAM_DATA_ITEM_SIZE);

// Do some asserts on the redefinitions in the shared header files
// These asserts cannot be done where declared since the shared files do not depend on the rest of bluenet
static_assert(MAC_ADDRESS_LENGTH == MAC_ADDRESS_LEN);
static_assert(MAX_BLE_ADV_DATA_LENGTH == ADVERTISEMENT_DATA_MAX_SIZE);
static_assert(MAX_MICROAPP_MESH_PAYLOAD_SIZE == MAX_MESH_MSG_PAYLOAD_SIZE);

/**
 * The IPC buffers can be used to bootstrap communication between microapp and bluenet. However, when in the microapp
 * coroutine context it is convenient if we can immediately yield towards the other context. For this we reserve a bit
 * of space on the stack (apart from stack pointer etc.).
 */
struct coroutine_args_t {
	uintptr_t entry;
	bluenet_io_buffers_t* io_buffers;
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
 * The class MicroappController has functionality to store a second app (and perhaps in the future even more apps) on
 * another part of the flash memory.
 */
class MicroappController : public EventListener {
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
	static const int8_t MICROAPP_MAX_SOFT_INTERRUPTS_WITHIN_A_TICK  = 10;

	/**
	 * The maximum number of consecutive calls to a microapp
	 */
	static const uint8_t MICROAPP_MAX_NUMBER_CONSECUTIVE_CALLS = 8;

	/**
	 * The maximum number of registered interrupts
	 */
	static const uint8_t MICROAPP_MAX_SOFT_INTERRUPT_REGISTRATIONS  = 10;

	/**
	 * Buffer for keeping track of registered interrupts
	 */
	microapp_soft_interrupt_registration_t _softInterruptRegistrations[MICROAPP_MAX_SOFT_INTERRUPT_REGISTRATIONS];

	/**
	 * Coroutine for microapp.
	 */
	coroutine_t _coroutine;

	/*
	 * Shared state to both the microapp and the bluenet code. This is used as an argument to the coroutine. It can
	 * later be used to get information back and forth between microapp and bluenet.
	 */
	coroutine_args_t sharedState;

	/**
	 * To throttle the ticks themselves
	 */
	uint8_t _tickCounter;

	/**
	 * Counter for interrupts within a tick. Limited to MICROAPP_MAX_SOFT_INTERRUPTS_WITHIN_A_TICK
	 */
	int8_t _softInterruptCounter;

	/**
	 * Counter for consecutive microapp calls. Limited to MICROAPP_MAX_NUMBER_CONSECUTIVE_CALLS
	 */
	uint8_t _consecutiveMicroappCallCounter;

	/**
	 * Flag to indicate whether to forward scanned devices to microapp.
	 */
	bool _microappIsScanning;

	/**
	 * Keeps track of how many empty interrupt slots are available on the microapp side
	 */
	uint8_t _emptySoftInterruptSlots = 1;

	/**
	 * Maps digital pins to interrupts. See also MicroappRequestHandler::interruptToDigitalPin()
	 */
	int digitalPinToInterrupt(int pin);

	/**
	 * Checks whether an interrupt registration already exists
	 */
	bool softInterruptRegistered(MicroappSdkMessageType type, uint8_t id);

	/**
	 * Checks whether the microapp has empty interrupt slots to deal with a new softInterrupt
	 */
	bool allowSoftInterrupts();

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
	 * Call the microapp in an interrupt context
	 */
	void generateSoftInterrupt();

	/**
	 * Check if start address of the microapp is within the flash boundaries assigned to the microapps.
	 */
	cs_ret_code_t checkFlashBoundaries(uint8_t appIndex, uintptr_t address);

	/**
	 * Get incoming microapp buffer (from coargs).
	 */
	uint8_t* getInputMicroappBuffer();

	/**
	 * Get outgoing microapp buffer (from coargs).
	 */
	uint8_t* getOutputMicroappBuffer();

	/**
	 * Handle a GPIO event
	 */
	void onGpioUpdate(uint8_t pinIndex);

	/**
	 * Handle a scanned BLE device.
	 */
	void onDeviceScanned(scanned_device_t* dev);

	/**
	 * Handle a received mesh message and determine whether to forward it to the microapp.
	 *
	 * @param event the EVT_RECV_MESH_MSG event data
	 */
	void onReceivedMeshMessage(MeshMsgEvent* event);

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
	void callApp(uint8_t appIndex);

	/**
	 * Tick microapp
	 */
	void tickMicroapp(uint8_t appIndex);

	/**
	 * Register interrupts that allow generation of interrupts to the microapp
	 */
	cs_ret_code_t registerSoftInterrupt(MicroappSdkMessageType type, uint8_t id);

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
	 * Enable or disable BLE scanned device interrupt calls
	 */
	void setScanning(bool scanning);

	/**
	 * Receive events
	 */
	void handleEvent(event_t& event);
};
